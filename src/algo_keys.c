#include "os.h"
#include "cx.h"
#include "os_io_seproxyhal.h"

#include "algo_keys.h"




static void algorand_key_derive(uint32_t accountId, cx_ecfp_private_key_t *privateKey)
{
  uint8_t  privateKeyData[64];
  uint32_t bip32Path[5];

  bip32Path[0] = 44  | 0x80000000;
  bip32Path[1] = 283 | 0x80000000;
  bip32Path[2] = accountId | 0x80000000;
  bip32Path[3] = 0;
  bip32Path[4] = 0;

  io_seproxyhal_io_heartbeat();

  os_perso_derive_node_bip32(CX_CURVE_Ed25519, bip32Path, sizeof(bip32Path) / sizeof(bip32Path[0]), privateKeyData, NULL);
  cx_ecfp_init_private_key(CX_CURVE_Ed25519, privateKeyData, 32, privateKey);

  io_seproxyhal_io_heartbeat();

  os_memset(bip32Path, 0, sizeof(bip32Path));
  os_memset(privateKeyData, 0, sizeof(privateKeyData));
}

static size_t generate_public_key(const cx_ecfp_private_key_t *privateKey, uint8_t *buf)
{
  cx_ecfp_public_key_t publicKey;

  cx_ecfp_generate_pair(CX_CURVE_Ed25519,
                        &publicKey,
                        (cx_ecfp_private_key_t *)privateKey,
                        1);

  // publicKey.W is 65 bytes: a header byte, followed by a 32-byte
  // x coordinate, followed by a 32-byte y coordinate.  The bytes
  // representing the coordinates are in reverse order.

  for (int i = 0; i < 32; i++) {
    buf[i] = publicKey.W[64-i];
  }

  if (publicKey.W[32] & 1) {
    buf[31] |= 0x80;
  }
  return 32;
}

void fetch_public_key(uint32_t accountId, uint8_t* pubkey)
{
  cx_ecfp_private_key_t privateKey;
  os_memset(&privateKey, 0, sizeof(privateKey));

  algorand_key_derive(accountId, &privateKey);
  generate_public_key(&privateKey, pubkey);
  os_memset(&privateKey, 0, sizeof(privateKey));
}


int algorand_sign_message(uint32_t account_id, const uint8_t* msg_to_sign , const uint32_t msg_len, uint8_t* out_buffer)
{
  int sign_size = 0;
  cx_ecfp_private_key_t privateKey;
  algorand_key_derive(account_id, &privateKey);

  io_seproxyhal_io_heartbeat();
  sign_size = cx_eddsa_sign(&privateKey,
                     0, CX_SHA512,
                     msg_to_sign, msg_len,
                     NULL, 0,
                     out_buffer,
                     6+2*(32+1), // Formerly from cx_compliance_141.c
                     NULL);

  
  io_seproxyhal_io_heartbeat();
  return sign_size;
}