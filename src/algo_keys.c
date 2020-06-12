#include "os.h"
#include "cx.h"

#include "algo_keys.h"


uint8_t publicKey[32];

void
algorand_key_derive(uint32_t accountId, uint8_t *privateKeyData)
{
  uint32_t bip32Path[5];

  bip32Path[0] = 44  | 0x80000000;
  bip32Path[1] = 283 | 0x80000000;
  bip32Path[2] = accountId | 0x80000000;
  bip32Path[3] = 0;
  bip32Path[4] = 0;
  os_perso_derive_node_bip32(CX_CURVE_Ed25519, bip32Path, sizeof(bip32Path) / sizeof(bip32Path[0]), privateKeyData, NULL);
}

void
algorand_private_key(uint8_t *privateKeyData, cx_ecfp_private_key_t *privateKey)
{
  cx_ecfp_init_private_key(CX_CURVE_Ed25519, privateKeyData, 32, privateKey);
}

void
algorand_public_key(uint8_t *privateKeyData, uint8_t *buf)
{
  cx_ecfp_private_key_t privateKey;
  cx_ecfp_public_key_t  publicKey;

  algorand_private_key(privateKeyData, &privateKey);
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
}
