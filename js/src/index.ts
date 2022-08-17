/** ******************************************************************************
 *  (c) 2018 - 2022 Zondax AG
 *  (c) 2016-2017 Ledger
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ******************************************************************************* */
import Transport from "@ledgerhq/hw-transport";
import {ResponseAddress, ResponseAppInfo, ResponseDeviceInfo, ResponseSign, ResponseVersion} from "./types";
import {
  CHUNK_SIZE,
  ERROR_CODE,
  errorCodeToString,
  getVersion,
  LedgerError,
  P1_VALUES,
  P2_VALUES,
  processErrorResponse,
} from "./common";
import {CLA, INS, PKLEN} from "./config";

export {LedgerError};
export * from "./types";

function processGetAddrResponse(response: Buffer) {
  const errorCodeData = response.slice(-2);
  const returnCode = (errorCodeData[0] * 256 + errorCodeData[1]);

  const publicKey = response.slice(0, PKLEN).toString('hex')
  const address = response.slice(PKLEN, response.length - 2).toString('ascii')

  return {
    // Legacy
    bech32_address: address,
    compressed_pk: publicKey,
    //
    publicKey,
    address,
    returnCode,
    errorMessage: errorCodeToString(returnCode),
// legacy
    return_code: returnCode,
    error_message: errorCodeToString(returnCode)
  };
}

export default class AlgorandApp {
  private transport: Transport;

  constructor(transport: Transport) {
    if (!transport) {
      throw new Error("Transport has not been defined");
    }
    this.transport = transport;
  }

  static prepareChunks(accountId: number, message: Buffer) {
    const chunks = [];

    // First chunk prepend accountId if != 0
    const messageBuffer = Buffer.from(message);
    let buffer : Buffer;
    if (accountId !== 0) {
      const accountIdBuffer = Buffer.alloc(4);
      accountIdBuffer.writeUInt32BE(accountId)
      buffer = Buffer.concat([accountIdBuffer, messageBuffer]);
    } else {
      buffer = Buffer.concat([messageBuffer]);
    }

    for (let i = 0; i < buffer.length; i += CHUNK_SIZE) {
      let end = i + CHUNK_SIZE;
      if (i > buffer.length) {
        end = buffer.length;
      }
      chunks.push(buffer.slice(i, end));
    }
    return chunks;
  }

  async signGetChunks(accountId: number, message: string | Buffer) {
    if (typeof message === 'string') {
      return AlgorandApp.prepareChunks(accountId, Buffer.from(message));
    }

    return AlgorandApp.prepareChunks(accountId, message);
  }

  async getVersion(): Promise<ResponseVersion> {
    return getVersion(this.transport).catch(err => processErrorResponse(err));
  }

  async getAppInfo(): Promise<ResponseAppInfo> {
    return this.transport.send(0xb0, 0x01, 0, 0).then(response => {
      const errorCodeData = response.slice(-2);
      const returnCode = errorCodeData[0] * 256 + errorCodeData[1];

      const result: { errorMessage?: string; returnCode?: LedgerError } = {};

      let appName = "err";
      let appVersion = "err";
      let flagLen = 0;
      let flagsValue = 0;

      if (response[0] !== 1) {
        // Ledger responds with format ID 1. There is no spec for any format != 1
        result.errorMessage = "response format ID not recognized";
        result.returnCode = LedgerError.DeviceIsBusy;
      } else {
        const appNameLen = response[1];
        appName = response.slice(2, 2 + appNameLen).toString("ascii");
        let idx = 2 + appNameLen;
        const appVersionLen = response[idx];
        idx += 1;
        appVersion = response.slice(idx, idx + appVersionLen).toString("ascii");
        idx += appVersionLen;
        const appFlagsLen = response[idx];
        idx += 1;
        flagLen = appFlagsLen;
        flagsValue = response[idx];
      }

      return {
        returnCode,
        errorMessage: errorCodeToString(returnCode),
        // legacy
        return_code: returnCode,
        error_message: errorCodeToString(returnCode),
        //
        appName,
        appVersion,
        flagLen,
        flagsValue,
        flagRecovery: (flagsValue & 1) !== 0,
        // eslint-disable-next-line no-bitwise
        flagSignedMcuCode: (flagsValue & 2) !== 0,
        // eslint-disable-next-line no-bitwise
        flagOnboarded: (flagsValue & 4) !== 0,
        // eslint-disable-next-line no-bitwise
        flagPINValidated: (flagsValue & 128) !== 0
      };
    }, processErrorResponse);
  }

  async deviceInfo(): Promise<ResponseDeviceInfo> {
    return this.transport.send(0xe0, 0x01, 0, 0, Buffer.from([]), [0x6e00])
      .then(response => {
        const errorCodeData = response.slice(-2);
        const returnCode = errorCodeData[0] * 256 + errorCodeData[1];

        if (returnCode === 0x6e00) {
          return {
            return_code: returnCode,
            error_message: "This command is only available in the Dashboard"
          };
        }

        const targetId = response.slice(0, 4).toString("hex");

        let pos = 4;
        const secureElementVersionLen = response[pos];
        pos += 1;
        const seVersion = response.slice(pos, pos + secureElementVersionLen).toString();
        pos += secureElementVersionLen;

        const flagsLen = response[pos];
        pos += 1;
        const flag = response.slice(pos, pos + flagsLen).toString("hex");
        pos += flagsLen;

        const mcuVersionLen = response[pos];
        pos += 1;
        // Patch issue in mcu version
        let tmp = response.slice(pos, pos + mcuVersionLen);
        if (tmp[mcuVersionLen - 1] === 0) {
          tmp = response.slice(pos, pos + mcuVersionLen - 1);
        }
        const mcuVersion = tmp.toString();

        return {
          returnCode: returnCode,
          errorMessage: errorCodeToString(returnCode),
          // legacy
          return_code: returnCode,
          error_message: errorCodeToString(returnCode),
          // //
          targetId,
          seVersion,
          flag,
          mcuVersion
        };
      }, processErrorResponse);
  }

  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  async getPubkey(accountId = 0, requireConfirmation = false): Promise<ResponseAddress> {
    const data = Buffer.alloc(4);
    data.writeUInt32BE(accountId)

    const p1_value = requireConfirmation ? P1_VALUES.SHOW_ADDRESS_IN_DEVICE : P1_VALUES.ONLY_RETRIEVE

    return this.transport
      .send(CLA, INS.GET_PUBLIC_KEY, p1_value, P2_VALUES.DEFAULT, data, [0x9000])
      .then(processGetAddrResponse, processErrorResponse);
  }

  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  async getAddressAndPubKey(accountId = 0, requireConfirmation = false): Promise<ResponseAddress> {
    const data = Buffer.alloc(4);
    data.writeUInt32BE(accountId)

    const p1_value = requireConfirmation ? P1_VALUES.SHOW_ADDRESS_IN_DEVICE : P1_VALUES.ONLY_RETRIEVE

    return this.transport
      .send(CLA, INS.GET_ADDRESS, p1_value, P2_VALUES.DEFAULT, data, [0x9000])
      .then(processGetAddrResponse, processErrorResponse);
  }

  async signSendChunk(chunkIdx: number, chunkNum: number, accountId: number, chunk: Buffer): Promise<ResponseSign> {
    let p1 = P1_VALUES.MSGPACK_ADD
    let p2 = P2_VALUES.MSGPACK_ADD

    if (chunkIdx === 1) {
      p1 = (accountId !== 0) ? P1_VALUES.MSGPACK_FIRST_ACCOUNT_ID : P1_VALUES.MSGPACK_FIRST
    }
    if (chunkIdx === chunkNum) {
      p2 = P2_VALUES.MSGPACK_LAST
    }

    return this.transport
      .send(CLA, INS.SIGN_MSGPACK, p1, p2, chunk, [
        LedgerError.NoErrors,
        LedgerError.DataIsInvalid,
        LedgerError.BadKeyHandle,
        LedgerError.SignVerifyError
      ])
      .then((response: Buffer) => {
        const errorCodeData = response.slice(-2);
        const returnCode = errorCodeData[0] * 256 + errorCodeData[1];
        let errorMessage = errorCodeToString(returnCode);

        if (returnCode === LedgerError.BadKeyHandle ||
          returnCode === LedgerError.DataIsInvalid ||
          returnCode === LedgerError.SignVerifyError) {
          errorMessage = `${errorMessage} : ${response
            .slice(0, response.length - 2)
            .toString("ascii")}`;
        }

        if (returnCode === LedgerError.NoErrors && response.length > 2) {
          const signature = response.slice(0, response.length - 2);
          return {
            signature,
            returnCode: returnCode,
            errorMessage: errorMessage,
            // legacy
            return_code: returnCode,
            error_message: errorCodeToString(returnCode),
          };
        }

        return {
          returnCode: returnCode,
          errorMessage: errorMessage,
          // legacy
          return_code: returnCode,
          error_message: errorCodeToString(returnCode),
        } as ResponseSign;

      }, processErrorResponse);
  }

  async sign(accountId = 0, message: string | Buffer) {
    return this.signGetChunks(accountId, message).then(chunks => {
      return this.signSendChunk(1, chunks.length, accountId, chunks[0]).then(async result => {
        for (let i = 1; i < chunks.length; i += 1) {
          // eslint-disable-next-line no-await-in-loop,no-param-reassign
          result = await this.signSendChunk(1 + i, chunks.length, accountId, chunks[i])
          if (result.return_code !== ERROR_CODE.NoError) {
            break
          }
        }

        return {
          return_code: result.return_code,
          error_message: result.error_message,
          signature: result.signature,
        }
      }, processErrorResponse)
    })
  }
}
