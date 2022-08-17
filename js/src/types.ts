import {LedgerError} from "./common";

export interface ResponseBase {
  // @deprecated: Please use errorMessage instead
  error_message?: string;
  // @deprecated: Please use returnCode instead
  return_code: LedgerError;

  errorMessage?: string;
  returnCode: LedgerError;
}

export interface ResponseAddress extends ResponseBase {
  // @deprecated: Please use address instead
  bech32_address: Buffer;
  // @deprecated: Please use publicKey instead
  compressed_pk: Buffer;

  publicKey: Buffer;
  address: Buffer;
}

export interface ResponseVersion extends ResponseBase {
  // @deprecated: Please use testMode instead
  test_mode: LedgerError;

  testMode: boolean;
  major: number;
  minor: number;
  patch: number;
  deviceLocked: boolean;
  targetId: string;
}

export interface ResponseAppInfo extends ResponseBase {
  appName: string;
  appVersion: string;
  flagLen: number;
  flagsValue: number;
  flagRecovery: boolean;
  flagSignedMcuCode: boolean;
  flagOnboarded: boolean;
  flagPINValidated: boolean;
}

export interface ResponseDeviceInfo extends ResponseBase {
  targetId: string;
  seVersion: string;
  flag: string;
  mcuVersion: string;
}

export interface ResponseSign extends ResponseBase {
  signature: Buffer
}
