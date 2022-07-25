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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define FIXINT_0    0x00
#define FIXINT_127  0x7f
#define FIXMAP_0    0x80
#define FIXMAP_15   0x8f
#define FIXARR_0    0x90
#define FIXARR_15   0x9f
#define FIXSTR_0    0xa0
#define FIXSTR_31   0xbf
#define BOOL_FALSE  0xc2
#define BOOL_TRUE   0xc3

#define BIN8        0xc4
#define BIN16       0xc5
#define BIN32       0xc6

#define UINT8       0xcc
#define UINT16      0xcd
#define UINT32      0xce
#define UINT64      0xcf

#define STR8        0xd9
#define STR16       0xda
#define STR32       0xdb

#define MAP16       0xde
#define MAP32       0xdf

#ifdef __cplusplus
}
#endif

