#ifndef _ADPU_PROTOCOL_DEFS_
#define _ADPU_PROTOCOL_DEFS_

#define OFFSET_CLA    0
#define OFFSET_INS    1
#define OFFSET_P1     2
#define OFFSET_P2     3
#define OFFSET_LC     4
#define OFFSET_CDATA  5

#define CLA           0x80

#define P1_FIRST 0x00
#define P1_MORE  0x80
#define P1_WITH_ACCOUNT_ID  0x01
#define P1_WITH_REQUEST_USER_APPROVAL  0x80

#define P2_LAST  0x00
#define P2_MORE  0x80

#define INS_SIGN_PAYMENT    0x01    // Deprecated, unused
#define INS_SIGN_KEYREG     0x02    // Deprecated, unused
#define INS_GET_PUBLIC_KEY  0x03
#define INS_SIGN_PAYMENT_V2 0x04
#define INS_SIGN_KEYREG_V2  0x05
#define INS_SIGN_PAYMENT_V3 0x06
#define INS_SIGN_KEYREG_V3  0x07
#define INS_SIGN_MSGPACK    0x08

#endif