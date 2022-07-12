/*******************************************************************************
*   (c) 2019 Zondax GmbH
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

#include "gmock/gmock.h"

#include <vector>
#include <iostream>
#include <hexutils.h>
#include <parser_txdef.h>
#include <parser.h>
#include "parser_impl.h"

using namespace std;

TEST(SCALE, ReadBytes) {
    parser_context_t ctx;
    parser_error_t err;
    uint8_t buffer[100];
    auto bufferLen = parseHexString(
            buffer,
            sizeof(buffer),
            "45"
            "123456"
            "12345678901234567890"
    );

    parser_init(&ctx, buffer, bufferLen);

    uint8_t bytesArray[100] = {0};
    err = _readBytes(&ctx, bytesArray, 1);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(bytesArray[0], 0x45);

    uint8_t testArray[3] = {0x12, 0x34, 0x56};
    err = _readBytes(&ctx, bytesArray+1, 3);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    for (uint8_t i = 0; i < 3; i++) {
        EXPECT_EQ(testArray[i], bytesArray[i+1]);
    }

    uint8_t testArray2[10] = {0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90};
    err = _readBytes(&ctx, bytesArray+4, 10);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    for (uint8_t i = 0; i < 10; i++) {
        EXPECT_EQ(testArray2[i], bytesArray[i+4]);
    }
}

TEST(SCALE, ReadBytesFailed) {
    parser_context_t ctx;
    parser_error_t err;
    uint8_t buffer[100];
    auto bufferLen = parseHexString(
            buffer,
            sizeof(buffer),
            "45"
            "123456"
            "12345678901234567890"
    );

    parser_init(&ctx, buffer, bufferLen);

    uint8_t bytesArray[5] = {0};
    err = _readBytes(&ctx, bytesArray, 50);
    EXPECT_EQ(err, parser_unexpected_buffer_end) << parser_getErrorDescription(err);
}

TEST(SCALE, EncodingUint8) {
    parser_context_t ctx;
    parser_error_t err;
    uint8_t buffer[100] = {130, 169, 102, 105, 120, 101, 100, 85, 105, 110, 116, 12, 165, 117, 105, 110, 116, 56, 204, 230};
    auto bufferLen {50};

    parser_init(&ctx, buffer, bufferLen);

    size_t mapItems {0};
    err = _readMapSize(&ctx, &mapItems);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(mapItems, 2);

    uint8_t key[32] = {0};
    uint64_t value {0};

    //Read key:value pos0
    err = _readString(&ctx, key, sizeof(key));
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(strcmp((char*)key, "fixedUint"), 0);

    err = _readInteger(&ctx, &value);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(value, 12);


    //Read key:value pos1
    err = _readString(&ctx, key, sizeof(key));
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(strcmp((char*)key, "uint8"), 0);

    err = _readInteger(&ctx, &value);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(value, 230);
}

TEST(SCALE, EncodingUint16) {
    parser_context_t ctx;
    parser_error_t err;

    uint8_t buffer[100];
    auto bufferLen = parseHexString(
        buffer,
        sizeof(buffer),
        "82A875696E7431365F30CD3039A875696E7431365F31CDFE08"
    );

    parser_init(&ctx, buffer, bufferLen);

    size_t mapItems {0};
    err = _readMapSize(&ctx, &mapItems);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(mapItems, 2);

    uint8_t key[32] = {0};
    uint64_t value {0};

    //Read key:value pos0
    err = _readString(&ctx, key, sizeof(key));
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(strcmp((char*)key, "uint16_0"), 0);

    err = _readInteger(&ctx, &value);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(value, 12345);

    //Read key:value pos1
    err = _readString(&ctx, key, sizeof(key));
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(strcmp((char*)key, "uint16_1"), 0);

    err = _readInteger(&ctx, &value);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(value, 65032);
}

TEST(SCALE, EncodingUint32) {
    parser_context_t ctx;
    parser_error_t err;

    uint8_t buffer[100];
    auto bufferLen = parseHexString(
        buffer,
        sizeof(buffer),
        "82A875696E7433325F30CE00BC6079A875696E7433325F31CE19999999"
    );

    parser_init(&ctx, buffer, bufferLen);

    size_t mapItems {0};
    err = _readMapSize(&ctx, &mapItems);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(mapItems, 2);

    uint8_t key[32] = {0};
    uint64_t value {0};

    //Read key:value pos0
    err = _readString(&ctx, key, sizeof(key));
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(strcmp((char*)key, "uint32_0"), 0);

    err = _readInteger(&ctx, &value);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(value, 12345465);

    //Read key:value pos1
    err = _readString(&ctx, key, sizeof(key));
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(strcmp((char*)key, "uint32_1"), 0);

    err = _readInteger(&ctx, &value);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(value, 429496729);
}

TEST(SCALE, EncodingUint64) {
    parser_context_t ctx;
    parser_error_t err;

    uint8_t buffer[100];
    auto bufferLen = parseHexString(
        buffer,
        sizeof(buffer),
        "82A875696E7436345F30CF0000000000BC6079A875696E7436345F31CF000000000054D6B4"
    );

    parser_init(&ctx, buffer, bufferLen);

    size_t mapItems {0};
    err = _readMapSize(&ctx, &mapItems);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(mapItems, 2);

    uint8_t key[32] = {0};
    uint64_t value {0};

    //Read key:value pos0
    err = _readString(&ctx, key, sizeof(key));
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(strcmp((char*)key, "uint64_0"), 0);

    err = _readInteger(&ctx, &value);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(value, 12345465);

    //Read key:value pos1
    err = _readString(&ctx, key, sizeof(key));
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(strcmp((char*)key, "uint64_1"), 0);

    err = _readInteger(&ctx, &value);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(value, 5559988);
}

TEST(SCALE, EncodingString) {
    parser_context_t ctx;
    parser_error_t err;

    uint8_t buffer[100];
    auto bufferLen = parseHexString(
        buffer,
        sizeof(buffer),
        "82A8737472696E675F30AA6E657720737472696E67A8737472696E675F31D921736F6D65207465787420686572652074686174206D69676874206265206C6F6E67"
    );

    parser_init(&ctx, buffer, bufferLen);

    size_t mapItems {0};
    err = _readMapSize(&ctx, &mapItems);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(mapItems, 2);

    uint8_t key[50] = {0};

    //Read key:value pos0
    err = _readString(&ctx, key, sizeof(key));
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(strcmp((char*)key, "string_0"), 0);

    err = _readString(&ctx, key, sizeof(key));
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(strcmp((char*)key, "new string"), 0);

    //Read key:value pos1
    err = _readString(&ctx, key, sizeof(key));
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(strcmp((char*)key, "string_1"), 0);

    err = _readString(&ctx, key, sizeof(key));
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(strcmp((char*)key, "some text here that might be long"), 0);
}

TEST(SCALE, EncodingBoolean) {
    parser_context_t ctx;
    parser_error_t err;

    uint8_t buffer[100];
    auto bufferLen = parseHexString(
        buffer,
        sizeof(buffer),
        "82A9626F6F6C65616E5F30C3A9626F6F6C65616E5F31C2"
    );

    parser_init(&ctx, buffer, bufferLen);

    size_t mapItems {0};
    err = _readMapSize(&ctx, &mapItems);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(mapItems, 2);

    uint8_t key[50] = {0};
    uint8_t value {0};

    //Read key:value pos0
    err = _readString(&ctx, key, sizeof(key));
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(strcmp((char*)key, "boolean_0"), 0);

    value = 0xFF;
    err = _readBool(&ctx, &value);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(value, 1);

    //Read key:value pos1
    err = _readString(&ctx, key, sizeof(key));
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(strcmp((char*)key, "boolean_1"), 0);

    value = 0xFF;
    err = _readBool(&ctx, &value);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
    EXPECT_EQ(value, 0);
}


TEST(SCALE, ParseFreezeAssets) {
    parser_context_t ctx;
    parser_error_t err;
    parser_tx_t tx_item;
    parser_tx_t parser_obj;

    uint8_t buffer[] = {136,164,102,97,100,100,196,32,0,128,113,56,42,181,180,185,179,217,45,25,67,96,137,24,105,245,42,140,102,1,108,119,121,112,6,24,192,121,191,145,164,102,97,105,100,205,4,210,163,102,101,101,205,8,202,162,102,118,205,3,232,162,103,104,196,32,72,99,181,24,164,179,200,78,200,16,242,45,79,16,129,203,15,113,240,89,167,172,32,222,198,47,127,112,229,9,58,34,162,108,118,205,7,208,163,115,110,100,196,32,171,254,108,36,82,141,12,209,97,249,95,84,239,5,200,187,60,138,223,232,41,229,170,243,199,2,0,70,121,158,251,185,164,116,121,112,101,164,97,102,114,122};
    uint16_t bufferLen = sizeof(buffer);
    err = parser_parse(&ctx, buffer, bufferLen, &parser_obj);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
}

TEST(Transactions, AssetXfer) {
    parser_context_t ctx;
    parser_error_t err;
    parser_tx_t parser_obj;

    uint8_t buffer[] = {137,164,97,97,109,116,10,164,97,114,99,118,196,32,86,149,120,43,210,87,218,245,240,34,71,114,243,188,72,116,212,242,145,254,132,88,187,108,72,154,171,40,86,45,162,57,163,102,101,101,205,9,16,162,102,118,205,3,232,162,103,104,196,32,72,99,181,24,164,179,200,78,200,16,242,45,79,16,129,203,15,113,240,89,167,172,32,222,198,47,127,112,229,9,58,34,162,108,118,205,7,208,163,115,110,100,196,32,86,149,120,43,210,87,218,245,240,34,71,114,243,188,72,116,212,242,145,254,132,88,187,108,72,154,171,40,86,45,162,57,164,116,121,112,101,165,97,120,102,101,114,164,120,97,105,100,205,4,210};
    uint16_t bufferLen = sizeof(buffer)/sizeof(buffer[0]);

    parser_init(&ctx, buffer, bufferLen);
    err =_read(&ctx, &parser_obj);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
}

TEST(Transactions, AssetConfig) {
    parser_context_t ctx;
    parser_error_t err;
    parser_tx_t parser_obj;

    uint8_t buffer[] = {136,164,97,112,97,114,132,161,99,196,32,84,107,24,45,17,156,227,15,99,178,55,184,156,217,164,104,232,43,180,255,215,53,123,85,219,151,28,169,128,32,175,64,161,102,196,32,67,183,197,138,6,218,192,233,7,232,191,109,116,50,117,124,110,93,63,106,116,225,229,102,101,134,201,162,26,182,148,78,161,109,196,32,200,68,255,169,13,75,248,157,166,164,74,28,140,190,109,45,90,126,101,160,50,164,128,177,27,113,63,34,214,27,173,58,161,114,196,32,34,119,92,43,165,171,121,151,242,243,150,163,43,55,202,149,110,138,32,31,181,182,72,29,185,8,63,74,58,101,207,81,164,99,97,105,100,205,4,210,163,102,101,101,205,13,32,162,102,118,205,3,232,162,103,104,196,32,72,99,181,24,164,179,200,78,200,16,242,45,79,16,129,203,15,113,240,89,167,172,32,222,198,47,127,112,229,9,58,34,162,108,118,205,7,208,163,115,110,100,196,32,159,196,157,204,110,90,9,21,46,44,29,174,156,77,5,145,203,185,245,231,184,177,44,138,43,232,150,199,191,161,237,103,164,116,121,112,101,164,97,99,102,103};
    uint16_t bufferLen = sizeof(buffer)/sizeof(buffer[0]);

    parser_init(&ctx, buffer, bufferLen);
    err =_read(&ctx, &parser_obj);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
}

TEST(Transactions, Keyreg) {
    parser_context_t ctx;
    parser_error_t err;
    parser_tx_t parser_obj;

    uint8_t buffer[] = {140,163,102,101,101,205,14,66,162,102,118,205,3,231,162,103,104,196,32,72,99,181,24,164,179,200,78,200,16,242,45,79,16,129,203,15,113,240,89,167,172,32,222,198,47,127,112,229,9,58,34,162,108,118,205,13,128,166,115,101,108,107,101,121,196,32,246,106,245,221,24,188,172,87,169,196,221,224,132,133,43,100,219,162,232,186,170,51,152,68,204,30,57,70,184,185,150,69,163,115,110,100,196,32,187,14,182,52,21,74,24,11,106,39,77,215,117,41,92,54,211,186,122,174,107,93,176,204,16,197,70,45,176,243,48,223,167,115,112,114,102,107,101,121,196,64,153,132,116,25,81,14,108,196,210,53,219,10,51,164,112,99,44,7,96,250,149,14,168,55,19,130,69,207,22,78,173,225,253,53,79,1,250,210,179,81,169,242,99,192,16,215,142,33,17,56,18,49,126,223,93,108,35,5,209,243,232,5,164,247,164,116,121,112,101,166,107,101,121,114,101,103,167,118,111,116,101,102,115,116,1,166,118,111,116,101,107,100,10,167,118,111,116,101,107,101,121,196,32,246,106,245,221,24,188,172,87,169,196,221,224,132,133,43,100,219,162,232,186,170,51,152,68,204,30,57,70,184,185,150,69,167,118,111,116,101,108,115,116,205,7,208};
    uint16_t bufferLen = sizeof(buffer)/sizeof(buffer[0]);

    parser_init(&ctx, buffer, bufferLen);
    err =_read(&ctx, &parser_obj);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
}

TEST(Transactions, Payment) {
    parser_context_t ctx;
    parser_error_t err;
    parser_tx_t parser_obj;

    uint8_t buffer[] = {139,163,97,109,116,205,3,232,165,99,108,111,115,101,196,32,64,233,52,146,136,37,100,203,206,156,89,166,155,103,84,38,137,233,161,195,162,169,234,91,101,166,232,164,66,31,252,87,163,102,101,101,205,3,232,162,102,118,205,48,57,163,103,101,110,172,100,101,118,110,101,116,45,118,51,56,46,48,162,103,104,196,32,254,179,108,57,16,20,57,0,195,218,85,66,202,24,54,176,15,210,248,25,89,18,87,205,35,246,4,47,152,200,54,157,162,108,118,205,246,253,164,110,111,116,101,196,8,69,38,34,0,24,82,134,251,163,114,99,118,196,32,123,108,226,79,235,91,172,192,177,100,226,156,34,44,87,245,246,61,195,135,212,57,4,130,88,65,28,95,225,15,124,2,163,115,110,100,196,32,141,146,180,137,144,1,115,160,77,250,67,89,163,102,106,106,252,234,44,66,160,93,217,193,247,62,235,165,71,128,55,233,164,116,121,112,101,163,112,97,121};
    uint16_t bufferLen = sizeof(buffer)/sizeof(buffer[0]);

    parser_init(&ctx, buffer, bufferLen);
    err =_read(&ctx, &parser_obj);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
}

TEST(Transactions, Application) {
    parser_context_t ctx;
    parser_error_t err;
    parser_tx_t parser_obj;

    uint8_t buffer[] = {222,0,20,164,97,112,97,97,146,196,1,0,196,2,1,2,164,97,112,97,110,1,164,97,112,97,112,196,5,1,32,1,1,34,164,97,112,97,115,145,6,164,97,112,97,116,145,196,32,187,14,182,52,21,74,24,11,106,39,77,215,117,41,92,54,211,186,122,174,107,93,176,204,16,197,70,45,176,243,48,223,164,97,112,101,112,2,164,97,112,102,97,145,3,164,97,112,103,115,130,163,110,98,115,2,163,110,117,105,1,164,97,112,108,115,130,163,110,98,115,4,163,110,117,105,3,164,97,112,115,117,196,5,2,32,1,1,34,163,102,101,101,205,3,232,162,102,118,206,0,4,236,15,163,103,101,110,172,116,101,115,116,110,101,116,45,118,49,46,48,162,103,104,196,32,72,99,181,24,164,179,200,78,200,16,242,45,79,16,129,203,15,113,240,89,167,172,32,222,198,47,127,112,229,9,58,34,162,108,118,206,0,4,239,247,162,108,120,196,32,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,164,110,111,116,101,196,10,110,111,116,101,32,118,97,108,117,101,165,114,101,107,101,121,196,32,160,137,170,105,34,227,185,152,250,223,246,205,72,8,221,249,224,33,228,148,78,56,158,163,213,198,56,120,102,137,25,126,163,115,110,100,196,32,9,251,210,118,44,8,248,108,90,230,191,109,215,167,169,1,222,102,117,215,80,224,126,140,92,118,152,100,125,182,225,253,164,116,121,112,101,164,97,112,112,108};
    uint16_t bufferLen = sizeof(buffer)/sizeof(buffer[0]);

    parser_init(&ctx, buffer, bufferLen);
    err =_read(&ctx, &parser_obj);
    EXPECT_EQ(err, parser_ok) << parser_getErrorDescription(err);
}
