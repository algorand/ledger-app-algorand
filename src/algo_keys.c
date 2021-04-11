#include "os.h"
#include "cx.h"
#include "os_io_seproxyhal.h"

#include "algo_keys.h"




static void algorand_key_derive(uint32_t account_id, cx_ecfp_private_key_t *private_key)
{
  uint8_t  private_key_data[64];
  uint32_t bip32Path[5];
  cx_ecfp_private_key_t local_private_key;

  explicit_bzero(&local_private_key,sizeof(local_private_key));
  explicit_bzero(private_key_data,sizeof(private_key_data));

  bip32Path[0] = 44  | 0x80000000;
  bip32Path[1] = 283 | 0x80000000;
  bip32Path[2] = account_id | 0x80000000;
  bip32Path[3] = 0;
  bip32Path[4] = 0;

  io_seproxyhal_io_heartbeat();

  BEGIN_TRY
  {
    TRY
    {
      os_perso_derive_node_bip32(CX_CURVE_Ed25519, bip32Path, sizeof(bip32Path) / sizeof(bip32Path[0]), private_key_data, NULL);
      cx_ecfp_init_private_key(CX_CURVE_Ed25519, private_key_data, 32, &local_private_key);
      os_memcpy(private_key, &local_private_key, sizeof(local_private_key));
    }
    FINALLY
    {
      explicit_bzero(&local_private_key,sizeof(local_private_key));
      explicit_bzero(private_key_data,sizeof(private_key_data));
    }
    END_TRY
  }
  io_seproxyhal_io_heartbeat();
}

static size_t get_public_key_from_private_key(const cx_ecfp_private_key_t *privateKey, uint8_t *buf)
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

void fetch_public_key(uint32_t account_id, uint8_t* pub_key)
{
  cx_ecfp_private_key_t private_key;
  explicit_bzero(&private_key, sizeof(private_key));

  BEGIN_TRY
  {
    TRY
    {
      algorand_key_derive(account_id, &private_key);
      get_public_key_from_private_key(&private_key, pub_key);
    }
    FINALLY
    {
      explicit_bzero(&private_key, sizeof(private_key));
    }
  }
  END_TRY
}


int algorand_sign_message(uint32_t account_id, const uint8_t* msg_to_sign , const uint32_t msg_len, uint8_t* out_buffer)
{
  int sign_size = 0;
  cx_ecfp_private_key_t private_key;
  explicit_bzero(&private_key,sizeof(private_key));

  algorand_key_derive(account_id, &private_key);
  
  BEGIN_TRY
  {
    TRY
    {
      io_seproxyhal_io_heartbeat();
      sign_size = cx_eddsa_sign(&private_key,
                     0, CX_SHA512,
                     msg_to_sign, msg_len,
                     NULL, 0,
                     out_buffer,
                     6+2*(32+1), // Formerly from cx_compliance_141.c
                     NULL);
    }
    FINALLY
    {
      explicit_bzero(&private_key,sizeof(private_key));
    }
  } 
  END_TRY
  
    
  io_seproxyhal_io_heartbeat();
  return sign_size;
}