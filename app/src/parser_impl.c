/*******************************************************************************
*  (c) 2018 - 2022 Zondax AG
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
********************************************************************************/

#include "parser_impl.h"
#include "msgpack.h"

static uint8_t num_items;
static uint8_t common_num_items;
static uint8_t tx_num_items;

#define MAX_ITEM_ARRAY 20
static uint8_t itemArray[MAX_ITEM_ARRAY] = {0};
static uint8_t itemIndex = 0;

DEC_READFIX_UNSIGNED(8);
DEC_READFIX_UNSIGNED(16);
DEC_READFIX_UNSIGNED(32);
DEC_READFIX_UNSIGNED(64);

#define COUNT(array) \
  (sizeof(array) / sizeof(*array))

parser_error_t parser_init_context(parser_context_t *ctx,
                                   const uint8_t *buffer,
                                   uint16_t bufferSize) {
    ctx->offset = 0;
    ctx->buffer = NULL;
    ctx->bufferLen = 0;
    num_items = 0;
    common_num_items = 0;
    tx_num_items = 0;

    if (bufferSize == 0 || buffer == NULL) {
        // Not available, use defaults
        return parser_init_context_empty;
    }

    ctx->buffer = buffer;
    ctx->bufferLen = bufferSize;
    return parser_ok;
}

parser_error_t parser_init(parser_context_t *ctx, const uint8_t *buffer, uint16_t bufferSize) {
    CHECK_ERROR(parser_init_context(ctx, buffer, bufferSize))
    return parser_ok;
}

static parser_error_t initializeItemArray()
{
    for(uint8_t i = 0; i < MAX_ITEM_ARRAY; i++) {
        itemArray[i] = 0xFF;
    }
    itemIndex = 0;
    return parser_ok;
}

static parser_error_t addItem(uint8_t displayIdx)
{
    if(itemIndex >= MAX_ITEM_ARRAY) {
        return parser_unexpected_buffer_end;
    }
    itemArray[itemIndex] = displayIdx;
    itemIndex++;

    return parser_ok;
}

parser_error_t getItem(uint8_t index, uint8_t* displayIdx)
{
    if(index >= itemIndex) {
        return parser_display_page_out_of_range;
    }
    *displayIdx = itemArray[index];
    return parser_ok;
}

static uint8_t getMsgPackType(uint8_t byte)
{
    if (byte >= FIXMAP_0 && byte <= FIXARR_15) {
        return FIXMAP_0;
    } else if (byte >= FIXSTR_0 && byte <= FIXSTR_31) {
        return FIXSTR_0;
    }
    return byte;
}

parser_error_t _readMapSize(parser_context_t *c, size_t *mapItems)
{
    uint8_t byte = 0;
    CHECK_ERROR(_readUInt8(c, &byte))

    switch (getMsgPackType(byte)) {
        case FIXMAP_0:
            *mapItems = byte - FIXMAP_0;
        break;

        case MAP16:
            CHECK_ERROR(_readUInt16(c, (uint16_t*)mapItems))
        break;

        case MAP32:
            return parser_msgpack_map_type_not_supported;
        break;

        default:
            return parser_msgpack_unexpected_type;
    }
    return parser_ok;
}

parser_error_t _readArraySize(parser_context_t *c, size_t *mapItems)
{
    uint8_t byte = 0;
    CHECK_ERROR(_readUInt8(c, &byte))

    if (byte >= FIXARR_0 && byte <= FIXARR_15) {
        *mapItems = byte - FIXARR_0;
        return parser_ok;
    }

    return parser_msgpack_unexpected_type;
}

parser_error_t _readBytes(parser_context_t *c, uint8_t *buff, uint16_t buffLen)
{
    CTX_CHECK_AVAIL(c, buffLen)
    MEMCPY(buff, (c->buffer + c->offset), buffLen);
    CTX_CHECK_AND_ADVANCE(c, buffLen)
    return parser_ok;
}

parser_error_t _readString(parser_context_t *c, uint8_t *buff, uint16_t buffLen)
{
    uint8_t byte = 0;
    uint8_t strLen = 0;
    CHECK_ERROR(_readUInt8(c, &byte))
    memset(buff, 0, buffLen);

    switch (getMsgPackType(byte)) {
    case FIXSTR_0:
        strLen = byte - FIXSTR_0;
        break;

    case STR8:
        CHECK_ERROR(_readUInt8(c, &strLen))
        break;

    case STR16:
        return parser_msgpack_str_type_not_supported;
        break;

    case STR32:
        return parser_msgpack_str_type_not_supported;
        break;

    default:
        return parser_msgpack_str_type_expected;
        break;
    }

    if (strLen >= buffLen) {
        return parser_msgpack_str_too_big;
    }
    CHECK_ERROR(_readBytes(c, buff, strLen))
    return parser_ok;
}

parser_error_t _readInteger(parser_context_t *c, uint64_t* value)
{
    uint8_t intType = 0;
    CHECK_ERROR(_readBytes(c, &intType, 1))

    if (intType >= FIXINT_0 && intType <= FIXINT_127) {
        *value = intType - FIXINT_0;
        return parser_ok;
    }

    switch (intType)
    {
    case UINT8: {
        uint8_t tmp = 0;
        CHECK_ERROR(_readUInt8(c, &tmp))
        *value = (uint64_t)tmp;
        break;
    }
    case UINT16: {
        uint16_t tmp = 0;
        CHECK_ERROR(_readUInt16(c, &tmp))
        *value = (uint64_t)tmp;
        break;
    }
    case UINT32: {
        uint32_t tmp = 0;
        CHECK_ERROR(_readUInt32(c, &tmp))
        *value = (uint64_t)tmp;
        break;
    }
    case UINT64: {
        CHECK_ERROR(_readUInt64(c, value))
        break;
    }
    default:
        return parser_msgpack_int_type_expected;
        break;
    }

    return parser_ok;
}

parser_error_t _readBinFixed(parser_context_t *c, uint8_t *buff, uint16_t bufferLen)
{
    uint8_t binType = 0;
    uint8_t binLen = 0;
    CHECK_ERROR(_readUInt8(c, &binType))
    switch (binType)
    {
        case BIN8: {
            CHECK_ERROR(_readUInt8(c, &binLen))
            break;
        }
        case BIN16:
        case BIN32: {
            return parser_msgpack_bin_type_not_supported;
            break;
        }
        default: {
            return parser_msgpack_bin_type_expected;
            break;
        }
    }

    if(binLen != bufferLen) {
        return parser_msgpack_bin_unexpected_size;
    }
    CHECK_ERROR(_readBytes(c, buff, bufferLen))
    return parser_ok;
}

static parser_error_t _readBin(parser_context_t *c, uint8_t *buff, uint16_t *bufferLen, uint16_t bufferMaxSize)
{
    uint8_t binType = 0;
    uint16_t binLen = 0;
    CHECK_ERROR(_readUInt8(c, &binType))
    switch (binType)
    {
        case BIN8: {
            uint8_t tmp = 0;
            CHECK_ERROR(_readUInt8(c, &tmp))
            binLen = (uint16_t)tmp;
            break;
        }
        case BIN16: {
            CHECK_ERROR(_readUInt16(c, &binLen))
            break;
        }
        case BIN32: {
            return parser_msgpack_bin_type_not_supported;
            break;
        }
        default: {
            return parser_msgpack_bin_type_expected;
            break;
        }
    }

    if(binLen > bufferMaxSize) {
        return parser_msgpack_bin_unexpected_size;
    }

    *bufferLen = binLen;
    CHECK_ERROR(_readBytes(c, buff, *bufferLen))
    return parser_ok;
}

parser_error_t _readBool(parser_context_t *c, uint8_t *value)
{
    uint8_t tmp = 0;
    CHECK_ERROR(_readUInt8(c, &tmp))
    switch (tmp)
    {
        case BOOL_TRUE: {
            *value = 1;
            break;
        }

        case BOOL_FALSE: {
            *value = 0;
            break;
        }
        default: {
            return parser_msgpack_bool_type_expected;
            break;
        }
    }
    return parser_ok;
}

static parser_error_t _readAssetParams(parser_context_t *c, txn_asset_config *asset_config)
{
    size_t paramsSize = 0;
    CHECK_ERROR(_readMapSize(c, &paramsSize))

    CHECK_ERROR(_findKey(c, KEY_APARAMS_MANAGER))
    CHECK_ERROR(_readBinFixed(c, asset_config->params.manager, sizeof(asset_config->params.manager)))
    addItem(1);

    CHECK_ERROR(_findKey(c, KEY_APARAMS_RESERVE))
    CHECK_ERROR(_readBinFixed(c, asset_config->params.reserve, sizeof(asset_config->params.reserve)))
    addItem(2);

    CHECK_ERROR(_findKey(c, KEY_APARAMS_FREEZE))
    CHECK_ERROR(_readBinFixed(c, asset_config->params.freeze, sizeof(asset_config->params.freeze)))
    addItem(3);

    CHECK_ERROR(_findKey(c, KEY_APARAMS_CLAWBACK))
    CHECK_ERROR(_readBinFixed(c, asset_config->params.clawback, sizeof(asset_config->params.clawback)))
    addItem(4);

    tx_num_items += 4;

    // These items won't be displayed
    if (_findKey(c, KEY_APARAMS_TOTAL) == parser_ok) {
        CHECK_ERROR(_readInteger(c, &asset_config->params.total))
    }
    if (_findKey(c, KEY_APARAMS_DECIMALS) == parser_ok) {
        CHECK_ERROR(_readInteger(c, &asset_config->params.decimals))
    }
    if (_findKey(c, KEY_APARAMS_DEF_FROZEN) == parser_ok) {
        CHECK_ERROR(_readBool(c, &asset_config->params.default_frozen))
    }
    if (_findKey(c, KEY_APARAMS_UNIT_NAME) == parser_ok) {
        CHECK_ERROR(_readString(c, (uint8_t*)asset_config->params.unitname, sizeof(asset_config->params.unitname)))
    }
    if (_findKey(c, KEY_APARAMS_ASSET_NAME) == parser_ok) {
        CHECK_ERROR(_readString(c, (uint8_t*)asset_config->params.assetname, sizeof(asset_config->params.assetname)))
    }
    if (_findKey(c, KEY_APARAMS_URL) == parser_ok) {
        CHECK_ERROR(_readString(c, (uint8_t*)asset_config->params.url, sizeof(asset_config->params.url)))
    }
    if (_findKey(c, KEY_APARAMS_METADATA_HASH) == parser_ok) {
        CHECK_ERROR(_readBinFixed(c, asset_config->params.metadata_hash, sizeof(asset_config->params.metadata_hash)))
    }

    return parser_ok;
}

parser_error_t _readAppArgs(parser_context_t *c, uint8_t args[][MAX_ARGLEN], size_t args_len[], size_t *argsSize, size_t maxArgs)
{
    CHECK_ERROR(_readArraySize(c, argsSize))

    if (*argsSize > maxArgs) {
        return parser_msgpack_array_too_big;
    }

    for (size_t i = 0; i < *argsSize; i++) {
        CHECK_ERROR(_readBin(c, args[i], (uint16_t*)&args_len[i], MAX_ARGLEN))
    }

    return parser_ok;
}

parser_error_t _readAccounts(parser_context_t *c, uint8_t accounts[][32], size_t *numAccounts, size_t maxAccounts)
{
    CHECK_ERROR(_readArraySize(c, numAccounts))

    if (*numAccounts > maxAccounts) {
        return parser_msgpack_array_too_big;
    }

    for (size_t i = 0; i < *numAccounts; i++) {
        CHECK_ERROR(_readBinFixed(c, accounts[i], sizeof(accounts[0])))
    }

    return parser_ok;
}

parser_error_t _readStateSchema(parser_context_t *c, state_schema *schema)
{
    size_t mapSize = 0;
    CHECK_ERROR(_readMapSize(c, &mapSize))
    uint8_t key[32];
    for (size_t i = 0; i < mapSize; i++) {
        CHECK_ERROR(_readString(c, key, sizeof(key)))
        if (strncmp((char*)key, KEY_SCHEMA_NUI, sizeof(KEY_SCHEMA_NUI)) == 0) {
            CHECK_ERROR(_readInteger(c, &schema->num_uint))
        } else if (strncmp((char*)key, KEY_SCHEMA_NBS, sizeof(KEY_SCHEMA_NBS)) == 0) {
            CHECK_ERROR(_readInteger(c, &schema->num_byteslice))
        } else {
            return parser_msgpack_unexpected_key;
        }
    }

    return parser_ok;
}

parser_error_t _readArrayU64(parser_context_t *c, uint64_t elements[], size_t *numElements, size_t maxElements)
{
    CHECK_ERROR(_readArraySize(c, numElements))

    if (*numElements > maxElements) {
        return parser_msgpack_array_too_big;
    }

    for (size_t i = 0; i < *numElements; i++) {
        CHECK_ERROR(_readInteger(c, &elements[i]))
    }

    return parser_ok;
}

static parser_error_t _readTxType(parser_context_t *c, parser_tx_t *v)
{
    uint8_t buff[100] = {0};
    uint8_t buffLen = sizeof(buff);
    uint8_t typeStr[50] = {0};
    uint8_t typeStrLen = sizeof(typeStr);
    uint16_t currentOffset = c->offset;
    c->offset = 0;

    for (size_t offset = 0; offset < c->bufferLen; offset++) {
        if (_readString(c, buff, buffLen) == parser_ok) {
            if (strncmp((char*)buff, KEY_COMMON_TYPE, sizeof(KEY_COMMON_TYPE)) == 0) {
                if (_readString(c, typeStr, typeStrLen) == parser_ok) {
                    if (strncmp((char *) typeStr, KEY_TX_PAY, sizeof(KEY_TX_PAY)) == 0) {
                        v->type = TX_PAYMENT;
                        break;
                    } else if (strncmp((char *) typeStr, KEY_TX_KEYREG, sizeof(KEY_TX_KEYREG)) == 0) {
                        v->type = TX_KEYREG;
                        break;
                    } else if (strncmp((char *) typeStr, KEY_TX_ASSET_XFER, sizeof(KEY_TX_ASSET_XFER)) == 0) {
                        v->type = TX_ASSET_XFER;
                        break;
                    } else if (strncmp((char *) typeStr, KEY_TX_ASSET_FREEZE, sizeof(KEY_TX_ASSET_FREEZE)) == 0) {
                        v->type = TX_ASSET_FREEZE;
                        break;
                    } else if (strncmp((char *) typeStr, KEY_TX_ASSET_CONFIG, sizeof(KEY_TX_ASSET_CONFIG)) == 0) {
                        v->type = TX_ASSET_CONFIG;
                        break;
                    } else if (strncmp((char *) typeStr, KEY_TX_APPLICATION, sizeof(KEY_TX_APPLICATION)) == 0) {
                        v->type = TX_APPLICATION;
                        break;
                    }
                }
            } else {
                c->offset = offset + 1;
            }
        }
    }

    if(v->type == TX_UNKNOWN) {
        // Return buffer to previous offset if key is not found
        c->offset = currentOffset;
        return parser_no_data;
    }

    return parser_ok;
}

static parser_error_t _readTxCommonParams(parser_context_t *c, parser_tx_t *v)
{
    common_num_items = 3;

    CHECK_ERROR(_findKey(c, KEY_COMMON_SENDER))
    CHECK_ERROR(_readBinFixed(c, v->sender, sizeof(v->sender)))
    addItem(0);

    CHECK_ERROR(_findKey(c, KEY_COMMON_FEE))
    CHECK_ERROR(_readInteger(c, &v->fee))
    addItem(1);

    if (_findKey(c, KEY_COMMON_GEN_ID) == parser_ok) {
        CHECK_ERROR(_readString(c, (uint8_t*)v->genesisID, sizeof(v->genesisID)))
        common_num_items++;
        addItem(3);
    }

    CHECK_ERROR(_findKey(c, KEY_COMMON_GEN_HASH))
    CHECK_ERROR(_readBinFixed(c, v->genesisHash, sizeof(v->genesisHash)))
    addItem(2);


    if (_findKey(c, KEY_COMMON_GROUP_ID) == parser_ok) {
        CHECK_ERROR(_readBinFixed(c, v->groupID, sizeof(v->groupID)))
        common_num_items++;
        addItem(10);
    }

    // Add lease

    if (_findKey(c, KEY_COMMON_NOTE) == parser_ok) {
        uint16_t noteLen = 0;
        CHECK_ERROR(_readBin(c, v->note, &noteLen, sizeof(v->note)))
        v->note_len = (size_t)noteLen;
        common_num_items++;
        addItem(4);
    }

    if (_findKey(c, KEY_COMMON_REKEY) == parser_ok) {
        CHECK_ERROR(_readBinFixed(c, v->rekey, sizeof(v->rekey)))
    }

    // First and Last valid won't be display --> don't count them
    CHECK_ERROR(_findKey(c, KEY_COMMON_FIRST_VALID))
    CHECK_ERROR(_readInteger(c, &v->firstValid))

    CHECK_ERROR(_findKey(c, KEY_COMMON_LAST_VALID))
    CHECK_ERROR(_readInteger(c, &v->lastValid))

    return parser_ok;
}

parser_error_t _findKey(parser_context_t *c, const char *key)
{
    uint8_t buff[100] = {0};
    uint8_t buffLen = sizeof(buff);
    uint16_t currentOffset = c->offset;
    c->offset = 0;
    for (size_t offset = 0; offset < c->bufferLen; offset++) {
        if (_readString(c, buff, buffLen) == parser_ok) {
            if (strlen((char*)buff) == strlen(key) && strncmp((char*)buff, key, strlen(key)) == 0) {
                return parser_ok;
            }
        }
        c->offset = offset + 1;
    }
    // Return buffer to previous offset if key is not found
    c->offset = currentOffset;
    return parser_no_data;
}

static parser_error_t _readTxPayment(parser_context_t *c, parser_tx_t *v)
{
    CHECK_ERROR(_findKey(c, KEY_PAY_RECEIVER))
    CHECK_ERROR(_readBinFixed(c, v->payment.receiver, sizeof(v->payment.receiver)))
    addItem(0);

    CHECK_ERROR(_findKey(c, KEY_PAY_AMOUNT))
    CHECK_ERROR(_readInteger(c, &v->payment.amount))
    addItem(1);

    tx_num_items = 2;

    if (_findKey(c, KEY_PAY_CLOSE) == parser_ok) {
        CHECK_ERROR(_readBinFixed(c, v->payment.close, sizeof(v->payment.close)))
        tx_num_items++;
        addItem(2);
    }

    return parser_ok;
}

static parser_error_t _readTxKeyreg(parser_context_t *c, parser_tx_t *v)
{
    CHECK_ERROR(_findKey(c, KEY_VOTE_PK))
    CHECK_ERROR(_readBinFixed(c, v->keyreg.votepk, sizeof(v->keyreg.votepk)))
    addItem(0);

    CHECK_ERROR(_findKey(c, KEY_VRF_PK))
    CHECK_ERROR(_readBinFixed(c, v->keyreg.vrfpk, sizeof(v->keyreg.vrfpk)))
    addItem(1);

    CHECK_ERROR(_findKey(c, KEY_SPRF_PK))
    CHECK_ERROR(_readBinFixed(c, v->keyreg.sprfkey, sizeof(v->keyreg.sprfkey)))
    addItem(2);

    CHECK_ERROR(_findKey(c, KEY_VOTE_FIRST))
    CHECK_ERROR(_readInteger(c, &v->keyreg.voteFirst))
    addItem(3);

    CHECK_ERROR(_findKey(c, KEY_VOTE_LAST))
    CHECK_ERROR(_readInteger(c, &v->keyreg.voteLast))
    addItem(4);

    CHECK_ERROR(_findKey(c, KEY_VOTE_KEY_DILUTION))
    CHECK_ERROR(_readInteger(c, &v->keyreg.keyDilution))
    addItem(5);

    if (_findKey(c, KEY_VOTE_NON_PART_FLAG) == parser_ok) {
        CHECK_ERROR(_readBool(c, &v->keyreg.nonpartFlag))
    }
    addItem(6);

    tx_num_items = 7;


    return parser_ok;
}

static parser_error_t _readTxAssetXfer(parser_context_t *c, parser_tx_t *v)
{
    CHECK_ERROR(_findKey(c, KEY_XFER_ID))
    CHECK_ERROR(_readInteger(c, &v->asset_xfer.id))
    addItem(0);

    CHECK_ERROR(_findKey(c, KEY_XFER_AMOUNT))
    CHECK_ERROR(_readInteger(c, &v->asset_xfer.amount))
    addItem(1);

    CHECK_ERROR(_findKey(c, KEY_XFER_RECEIVER))
    CHECK_ERROR(_readBinFixed(c, v->asset_xfer.receiver, sizeof(v->asset_xfer.receiver)))
    addItem(2);

    tx_num_items = 3;

    if (_findKey(c, KEY_XFER_SENDER) == parser_ok) {
        CHECK_ERROR(_readBinFixed(c, v->asset_xfer.sender, sizeof(v->asset_xfer.sender)))
    }

    if (_findKey(c, KEY_XFER_CLOSE) == parser_ok) {
        CHECK_ERROR(_readBinFixed(c, v->asset_xfer.close, sizeof(v->asset_xfer.close)))
    }

    return parser_ok;
}

static parser_error_t _readTxAssetFreeze(parser_context_t *c, parser_tx_t *v)
{
    CHECK_ERROR(_findKey(c, KEY_FREEZE_ID))
    CHECK_ERROR(_readInteger(c, &v->asset_freeze.id))
    addItem(0);

    CHECK_ERROR(_findKey(c, KEY_FREEZE_ACCOUNT))
    CHECK_ERROR(_readBinFixed(c, v->asset_freeze.account, sizeof(v->asset_freeze.account)))
    addItem(1);

    if (_findKey(c, KEY_FREEZE_FLAG) == parser_ok) {
        if (_readBool(c, &v->asset_freeze.flag) != parser_ok) {
            v->asset_freeze.flag = 0x00;
        }
    }
    addItem(2);

    tx_num_items = 3;
    return parser_ok;
}

static parser_error_t _readTxAssetConfig(parser_context_t *c, parser_tx_t *v)
{
    CHECK_ERROR(_findKey(c, KEY_CONFIG_ID))
    CHECK_ERROR(_readInteger(c, &v->asset_config.id))
    addItem(0);

    tx_num_items = 1;

    CHECK_ERROR(_findKey(c, KEY_CONFIG_PARAMS))
    CHECK_ERROR(_readAssetParams(c, &v->asset_config))

    return parser_ok;
}

static parser_error_t _readTxApplication(parser_context_t *c, txn_application *application)
{
    if (_findKey(c, KEY_APP_ID) == parser_ok) {
        CHECK_ERROR(_readInteger(c, &application->id))
    }
    addItem(0);

    CHECK_ERROR(_findKey(c, KEY_APP_ONCOMPLETION))
    CHECK_ERROR(_readInteger(c, &application->oncompletion))
    addItem(1);

    CHECK_ERROR(_findKey(c, KEY_APP_FOREIGN_APPS))
    CHECK_ERROR(_readArrayU64(c, application->foreign_apps, &application->num_foreign_apps, COUNT(application->foreign_apps)))
    addItem(2);

    CHECK_ERROR(_findKey(c, KEY_APP_FOREIGN_ASSETS))
    CHECK_ERROR(_readArrayU64(c, application->foreign_assets, &application->num_foreign_assets, COUNT(application->foreign_assets)))
    addItem(3);

    CHECK_ERROR(_findKey(c, KEY_APP_ACCOUNTS))
    CHECK_ERROR(_readAccounts(c, application->accounts, &application->num_accounts, COUNT(application->accounts)))
    addItem(4);
    addItem(5);

    CHECK_ERROR(_findKey(c, KEY_APP_ARGS))
    CHECK_ERROR(_readAppArgs(c, application->app_args, application->app_args_len, &application->num_app_args,
                             COUNT(application->app_args)))
    addItem(6);
    addItem(7);

    CHECK_ERROR(_findKey(c, KEY_APP_GLOBAL_SCHEMA))
    CHECK_ERROR(_readStateSchema(c, &application->global_schema))
    addItem(8);

    CHECK_ERROR(_findKey(c, KEY_APP_LOCAL_SCHEMA))
    CHECK_ERROR(_readStateSchema(c, &application->local_schema))
    addItem(9);

    CHECK_ERROR(_findKey(c, KEY_APP_APROG_LEN))
    CHECK_ERROR(_readBin(c, application->aprog, (uint16_t*)&application->aprog_len, sizeof(application->aprog)))
    addItem(10);

    CHECK_ERROR(_findKey(c, KEY_APP_CPROG_LEN))
    CHECK_ERROR(_readBin(c, application->cprog, (uint16_t*)&application->cprog_len, sizeof(application->cprog)))
    addItem(11);

    tx_num_items = 12;
    return parser_ok;
}

parser_error_t _readArray_args(parser_context_t *c, uint8_t args[][MAX_ARGLEN], size_t args_len[], uint8_t *argsSize, uint8_t maxArgs)
{
    uint8_t byte = 0;
    CHECK_ERROR(_readUInt8(c, &byte))
    if (byte < FIXARR_0 || byte > FIXARR_15) {
        return parser_msgpack_array_unexpected_size;
    }
    *argsSize = byte - FIXARR_0;

    if(*argsSize > maxArgs) {
        return parser_msgpack_array_too_big;
    }

    for (size_t i = 0; i < *argsSize; i++) {
        CHECK_ERROR(_readBin(c, args[i], (uint16_t*)&args_len[i], sizeof(args[0])))
    }
    return parser_ok;
}

parser_error_t _read(parser_context_t *c, parser_tx_t *v)
{
    size_t keyLen = 0;
    CHECK_ERROR(initializeItemArray())

    CHECK_ERROR(_readMapSize(c, &keyLen))
    if(keyLen > UINT8_MAX) {
        return parser_unexpected_number_items;
    }

    // Read Tx type
    CHECK_ERROR(_readTxType(c, v))

    // Read common params
    CHECK_ERROR(_readTxCommonParams(c, v));

    // Read Tx specifics params
    switch (v->type) {
    case TX_PAYMENT:
        CHECK_ERROR(_readTxPayment(c, v))
        break;
    case TX_KEYREG:
        CHECK_ERROR(_readTxKeyreg(c, v))
        break;
    case TX_ASSET_XFER:
        CHECK_ERROR(_readTxAssetXfer(c, v))
        break;
    case TX_ASSET_FREEZE:
        CHECK_ERROR(_readTxAssetFreeze(c, v))
        break;
    case TX_ASSET_CONFIG:
        CHECK_ERROR(_readTxAssetConfig(c, v))
        break;
    case TX_APPLICATION:
        CHECK_ERROR(_readTxApplication(c, &v->application))
        break;
    default:
        return paser_unknown_transaction;
        break;
    }

    num_items = common_num_items + tx_num_items + 1;
    return parser_ok;
}

uint8_t _getNumItems()
{
    return num_items;
}

uint8_t _getCommonNumItems()
{
    return common_num_items;
}

uint8_t _getTxNumItems()
{
    return tx_num_items;
}

const char *parser_getErrorDescription(parser_error_t err) {
    switch (err) {
        case parser_ok:
            return "No error";
        case parser_no_data:
            return "No more data";
        case parser_init_context_empty:
            return "Initialized empty context";
        case parser_unexpected_buffer_end:
            return "Unexpected buffer end";
        case parser_unexpected_version:
            return "Unexpected version";
        case parser_unexpected_characters:
            return "Unexpected characters";
        case parser_unexpected_field:
            return "Unexpected field";
        case parser_duplicated_field:
            return "Unexpected duplicated field";
        case parser_value_out_of_range:
            return "Value out of range";
        case parser_unexpected_chain:
            return "Unexpected chain";
        case parser_query_no_results:
            return "item query returned no results";
        case parser_missing_field:
            return "missing field";
//////
        case parser_display_idx_out_of_range:
            return "display index out of range";
        case parser_display_page_out_of_range:
            return "display page out of range";

        default:
            return "Unrecognized error code";
    }
}

const char *parser_getMsgPackTypeDescription(uint8_t type) {
    switch (type) {
        case FIXINT_0:
        case FIXINT_127:
            return "Fixed Integer";
        case FIXMAP_0:
        case FIXMAP_15:
            return "Fixed Map";
        case FIXARR_0:
        case FIXARR_15:
            return "Fixed Array";
        case FIXSTR_0:
        case FIXSTR_31:
            return "Fix String";
        case BOOL_FALSE:
        case BOOL_TRUE:
            return "Boolean";
        case BIN8:
        case BIN16:
        case BIN32:
            return "Binary";
        case UINT8:
        case UINT16:
        case UINT32:
        case UINT64:
            return "Integer";
        case STR8:
        case STR16:
        case STR32:
            return "String";
        case MAP16:
        case MAP32:
            return "Map";
        default:
            return "Unrecognized type";
    }
}