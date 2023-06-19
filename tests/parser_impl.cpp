/*******************************************************************************
*   (c) 2018 - 2022 Zondax AG
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

    uint16_t mapItems {0};
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

    uint16_t mapItems {0};
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

    uint16_t mapItems {0};
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

    uint16_t mapItems {0};
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

    uint16_t mapItems {0};
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

    uint16_t mapItems {0};
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

TEST(Transactions, ApplicationLongBoxes) {
    parser_context_t ctx;
    parser_tx_t parser_obj;

    std::string blobStr = "8fa46170616192c50420ce1eb981afa49dde3d96f723a21eb3d8282755cd679fd55352bf010a49866c38d44f96e5ad9d674e6970ca6da1eb9707bc3333fd6789fe029eedf4303842d87049328f2789a898388cb1632680bb72216486d6f71bf946867ee36a5b1d4d0258a169157111c82af79b7566c1c606a7dc315235fe90253eedb41c264622fd9999676d305021be204cf715a02b6121a5f1db0d7d8d85ab9319d7105c786a12826a50be196c2631eb2295cbfdcb949099bf53201aee56ce06f70e9524dcd79c399f375cc1a8388d68e46a42fade2c40eb6e5ddf5ad267dd7ac7b66b120730c5c71f4b9f198141592c8b3f78ceef696fc51810b98d3178ac6af3e157f36b1eca33a54fadee1483e621e100dcddf52eafe3a3fdd636eb72a7d3cb28c9f73cfdb06eeee28215609e5d5c06266ec11c1d03439cd15af5fa13fac7fcabd48dfbded45f89fc36b2031f2acbdcc1cabbce9503572ac473fe585fb558640cc1e67d9e58b61f2284acee61cfab0b6a8237c6a06db378d0ebf3532b3243840b369f5a0fc08678c24ab38da942c6fbe5847f4779ca0be090eec0233c2d20b740a99be53c9118eda5182e7bd96973f19c359afbef8fb232dfeee26ed06aecd0ed97c2c22e98e0d53c7c5cd6eba396bec9afe7e919e04ee665b25269e02623cb8909c8172c09c7cab02281adf070e079ee379cd2f5e56410584096fc32d813b9f0595bfbfd8ee81b44925853c2654d1692d7b6bc25e4c806fec0f4f82889139e34a9c23ac662c6895079f1aeedde53094189ab72b9c3a5c78b35ae98c1275001b97f29ab8688954650f831d2dd47777fef9b063225c49fd7f4bff364238db26fd0dbee0d27861431f61da05deef420196f1d3fe189e1dc7acec265668fb8091cc1518de336090e696cb59aadb601b902505ca3679dd6343be18c9dcfe58c1180a3c95204c011aab05e724889d175f75b1ecf957884bae425786fc3cd5e6e46cf168929aafa1873dc96b875908fc33b81b330a878dc3479b8a79551db0c1d168d6d7eaa0be07957f29441ab1e0d3bc4f68b07a710ec2e8416a8a66df537542cedc1f88fa457c95a97fb4ec970834ccc4f891bc4ee28b5fe85146c49527781d6c32a1eebc87691f921a27c71312c03bfc9593c3e12f7cc62b994f865137b319caf467a771ce8b95bcfe950d4840208154d2ca07f0e5fbb4f0b78abaf000f55a3763e37a36f3075d0a79ade78fbfe7355d553dd8437358cb5131bd0d3024314edf31a2d4f4fe1454e482425144cf1473a60d3c5d1f10adae93edd8621dd4b082f83a9fd2320f60691b52e1f23c4c01a6087011239ce9faa131fef0d6615d70d23accd77cd0ce1c3a14d01dbb0afc3540da1b8b6bf8135901f10e5fbe17997e3c8f1c5e26aa80ed3bd14756e4eacaa7f8d1f2b6941b63bfec135a0d5cc1eb145d6fe48d367c6ae0878b18974af125bca886199f49ef07398266951b5df3969249a4d1126438f8cc923b5c4f51f7e83e436207dbc1556492778b60dde070c3d2932a2ca225d421b9b43c2e217d8c63ca6518fc4955da8de21dee22c1f26fcfeb356ae0ae723330d183bdc92fa3f2966aa347892053216e0c5c5ce2f95b05dc61617ce1ec46b8caa0d5c1280d28717a926aaf23242cea5a1bb8001b52fa57524c37e854d3dafe6d35a6d12ceda776c8e0c758d73771ccdd869b3a63782f0060397328ebdc1b880d07f8f52a843080db1112a3e8ab9962fa94463ee11e74418ae205887f0ae1d09708f96679882dbb73ba2e098c42ab9bf7d525884d08e65fbee450c73163cc03077b40a6f3facf58717a32630d44fd1b61e18ed7b6b82ac3292f210a46170616e01a46170617393cf00000002fb5d485fcf0000001a3d1b9e3ccf0000000fc575a8aea4617062789382a16901a16ec4413e3e3e3e3e205468697320697320612076657279206c6f6e6720626f78206e616d6520746861742063616e20626520757020746f2036342063686172732e2e2e2e82a16902a16ec408000000000000000181a16ec4080000000000000001a46170666192cf000000017448d3abcf00000012fbb6ac74a461706c7382a36e6273ce096a4b0ba36e7569ce34738bc9a3666565ce01017240a26676cd4f2aa367656eac6d61696e6e65742d76312e30a26768c420c061c4d8fc1dbdded2d7604be4568e3f6d041987ac37bde4b620b5ab39248adfa26c76ce00023a83a26c78c420df3c5ae42404363d396ed15f40bf55f658254b7b2c4d7374e7ec3d81b21a7168a46e6f7465c504007475727069732065676573746173207072657469756d2061656e65616e207068617265747261206d61676e6120616320706c61636572617420766573746962756c756d206c6563747573206d617572697320756c7472696365732065726f7320696e2063757273757320747572706973206d617373612074696e636964756e7420647569207574206f726e617265206c65637475732073697420616d65742065737420706c61636572617420696e2065676573746173206572617420696d706572646965742073656420657569736d6f64206e69736920706f727461206c6f72656d206d6f6c6c697320616c697175616d20757420706f72747469746f72206c656f2061206469616d20736f6c6c696369747564696e2074656d706f72206964206575206e69736c206e756e63206d6920697073756d20666175636962757320766974616520616c6971756574206e656320756c6c616d636f727065722073697420616d6574207269737573206e756c6c616d20656765742066656c69732065676574206e756e63206c6f626f72746973206d617474697320616c697175616d20666175636962757320707572757320696e206d617373612074656d706f72206e65632066657567696174206e69736c207072657469756d2066757363652069642076656c697420757420746f72746f72207072657469756d20766976657272612073757370656e646973736520706f74656e7469206e756c6c616d20616320746f72746f72207669746165207075727573206661756369627573206f726e6172652073757370656e646973736520736564206e697369206c616375732073656420746f72746f72207669746165207075727573206661756369627573206f726e6172652073757370656e646973736520736564206e697369206c616375732073656420746f72746f72207669746165207075727573206661756369627573206f726e6172652073757370656e646973736520736564206e697369206c616375732073656420746f72746f72207669746165207075727573206661756369627573206f726e6172652073757370656e646973736520736564206e697369206c616375732073656420746f72746f72207669746165207075727573206661756369627573206f726e6172652073757370656e646973736520736564206e697369206c616375732073656420746f72746f72207669746165207075727573206661756369627573206f726e6172652073757370656e646973736520736564206e697369206c616375732073656420746f72746f72207669746165207075727573206661756369627573206f726e6172652073757370656e646973736520736564206e697369206c616375732073656420746f72746f72207669746165207075727573a3736e64c4207c9952d72afcd20cb5c1666aa78de3adcfac081e02f1bcbc56129aab7a2c0186a474797065a46170706c";

    uint8_t buffer[20000];
    uint16_t bufferLen = parseHexString(buffer, sizeof(buffer), blobStr.c_str());

    parser_init(&ctx, buffer, bufferLen);
    const parser_error_t err =_read(&ctx, &parser_obj);
    // Try to parse transaction with Box: n = 65 bytes. It should be rejected with value out of range
    EXPECT_EQ(err, parser_value_out_of_range) << parser_getErrorDescription(err);
}
