#include "algo_keys.h"

//------------------------------------------------------------------------------

void
algorand_private_key(cx_ecfp_private_key_t *privateKey)
{
  // Allocate 64 bytes for privateKeyData because os_perso_derive_node_bip32
  // appears to write more than 32 bytes to this buffer.  However, we only
  // need 32 bytes for cx_ecfp_init_private_key..
  uint8_t privateKeyData[64];
  uint32_t bip32Path[5];

  bip32Path[0] = 44  | 0x80000000;
  bip32Path[1] = 283 | 0x80000000;
  bip32Path[2] = 0   | 0x80000000;
  bip32Path[3] = 0   | 0x80000000;
  bip32Path[4] = 0   | 0x80000000;

  os_perso_derive_node_bip32_seed_key(HDW_ED25519_SLIP10, CX_CURVE_Ed25519, bip32Path, sizeof(bip32Path) / sizeof(bip32Path[0]), privateKeyData, NULL, NULL, 0);
  cx_ecfp_init_private_key(CX_CURVE_Ed25519, privateKeyData, 32, privateKey);

  return;
}

void
algorand_public_key(uint8_t *buf)
{
  cx_ecfp_private_key_t privateKey;
  cx_ecfp_public_key_t publicKey;

  algorand_private_key(&privateKey);
  cx_ecfp_generate_pair(CX_CURVE_Ed25519, &publicKey, &privateKey, 1);

  // publicKey.W is 65 bytes: a header byte, followed by a 32-byte
  // x coordinate, followed by a 32-byte y coordinate.  The bytes
  // representing the coordinates are in reverse order.

  for (int i = 0; i < 32; i++) {
    buf[i] = publicKey.W[64-i];
  }

  if (publicKey.W[32] & 1) {
    buf[31] |= 0x80;
  }

  PRINTF("Public key (raw): %.*h\n", 65, publicKey.W);
  PRINTF("Public key (compressed): %.*h\n", 32, buf);

  return;
}
