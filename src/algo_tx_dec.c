#include <string.h>
#include "os.h"

#include "algo_tx.h"
#include "msgpack.h"

static char decode_err[64];

static uint8_t
next_byte(uint8_t **bufp, uint8_t *buf_end)
{
  if (*bufp >= buf_end) {
    snprintf(decode_err, sizeof(decode_err), "decode past end");
    THROW(INVALID_PARAMETER);
  }

  uint8_t b = **bufp;
  (*bufp)++;
  return b;
}

static unsigned int
decode_fixsz(uint8_t **bufp, uint8_t *buf_end, uint8_t fix_first, uint8_t fix_last)
{
  uint8_t b = next_byte(bufp, buf_end);
  if (b < fix_first || b > fix_last) {
    snprintf(decode_err, sizeof(decode_err), "decode %d wrong type (%d-%d)", b, fix_first, fix_last);
    THROW(INVALID_PARAMETER);
  }

  return b - fix_first;
}

static void
decode_string(uint8_t **bufp, uint8_t *buf_end, char *strbuf, size_t strbuflen)
{
  uint8_t b = next_byte(bufp, buf_end);
  uint8_t str_len = 0;
  if (b >= FIXSTR_0 && b <= FIXSTR_31) {
    str_len = b - FIXSTR_0;
  } else if (b == STR8) {
    str_len = next_byte(bufp, buf_end);
  } else {
    snprintf(decode_err, sizeof(decode_err), "expected string, found %d", b);
    THROW(INVALID_PARAMETER);
  }

  if (str_len > strbuflen) {
    snprintf(decode_err, sizeof(decode_err), "%d-byte string too big for %d-byte buf", str_len, strbuflen);
    THROW(INVALID_PARAMETER);
  }

  if (*bufp + str_len > buf_end) {
    snprintf(decode_err, sizeof(decode_err), "%d-byte string overruns input", str_len);
    THROW(INVALID_PARAMETER);
  }

  os_memmove(strbuf, *bufp, str_len);
  if (str_len < strbuflen) {
    strbuf[str_len] = 0;
  }
  *bufp += str_len;
}

static void
decode_string_nullterm(uint8_t **bufp, uint8_t *buf_end, char *strbuf, size_t strbuflen)
{
  if (strbuflen == 0) {
    snprintf(decode_err, sizeof(decode_err), "decode_string_nullterm: zero strbuflen");
    THROW(INVALID_PARAMETER);
  }

  decode_string(bufp, buf_end, strbuf, strbuflen-1);
  strbuf[strbuflen-1] = '\0';
}

static void
decode_bin_fixed(uint8_t **bufp, uint8_t *buf_end, uint8_t *res, size_t reslen)
{
  uint8_t b = next_byte(bufp, buf_end);
  if (b != BIN8) {
    snprintf(decode_err, sizeof(decode_err), "expected bin, found %d", b);
    THROW(INVALID_PARAMETER);
  }

  uint8_t bin_len = next_byte(bufp, buf_end);
  if (bin_len != reslen) {
    snprintf(decode_err, sizeof(decode_err), "expected %d bin bytes, found %d", reslen, bin_len);
    THROW(INVALID_PARAMETER);
  }

  if (*bufp + bin_len > buf_end) {
    snprintf(decode_err, sizeof(decode_err), "%d-byte bin overruns input", bin_len);
    THROW(INVALID_PARAMETER);
  }

  os_memmove(res, *bufp, bin_len);
  *bufp += bin_len;
}

static void
decode_bin_var(uint8_t **bufp, uint8_t *buf_end, uint8_t *res, size_t *reslen, size_t reslenmax)
{
  uint8_t b = next_byte(bufp, buf_end);
  uint16_t bin_len = 0;

  if (b == BIN8) {
    bin_len = next_byte(bufp, buf_end);
  } else if (b == BIN16) {
    bin_len = next_byte(bufp, buf_end);
    bin_len = (bin_len << 8) | next_byte(bufp, buf_end);
  } else {
    snprintf(decode_err, sizeof(decode_err), "expected bin, found %d", b);
    THROW(INVALID_PARAMETER);
  }

  if (bin_len > reslenmax) {
    snprintf(decode_err, sizeof(decode_err), "expected <= %d bin bytes, found %d", reslenmax, bin_len);
    THROW(INVALID_PARAMETER);
  }

  if (*bufp + bin_len > buf_end) {
    snprintf(decode_err, sizeof(decode_err), "%d-byte bin overruns input", bin_len);
    THROW(INVALID_PARAMETER);
  }

  os_memmove(res, *bufp, bin_len);
  *bufp += bin_len;
  *reslen = bin_len;
}

static void
decode_uint64(uint8_t **bufp, uint8_t *buf_end, uint64_t *res)
{
  uint8_t b = next_byte(bufp, buf_end);
  if (b >= FIXINT_0 && b <= FIXINT_127) {
    *res = b - FIXINT_0;
  } else if (b == UINT8) {
    *res = next_byte(bufp, buf_end);
  } else if (b == UINT16) {
    *res = next_byte(bufp, buf_end);
    *res = (*res << 8) | next_byte(bufp, buf_end);
  } else if (b == UINT32) {
    *res = next_byte(bufp, buf_end);
    *res = (*res << 8) | next_byte(bufp, buf_end);
    *res = (*res << 8) | next_byte(bufp, buf_end);
    *res = (*res << 8) | next_byte(bufp, buf_end);
  } else if (b == UINT64) {
    *res = next_byte(bufp, buf_end);
    *res = (*res << 8) | next_byte(bufp, buf_end);
    *res = (*res << 8) | next_byte(bufp, buf_end);
    *res = (*res << 8) | next_byte(bufp, buf_end);
    *res = (*res << 8) | next_byte(bufp, buf_end);
    *res = (*res << 8) | next_byte(bufp, buf_end);
    *res = (*res << 8) | next_byte(bufp, buf_end);
    *res = (*res << 8) | next_byte(bufp, buf_end);
  } else {
    snprintf(decode_err, sizeof(decode_err), "expected u64, found %d", b);
    THROW(INVALID_PARAMETER);
  }
}

static void
decode_bool(uint8_t **bufp, uint8_t *buf_end, uint8_t *res)
{
  uint8_t b = next_byte(bufp, buf_end);
  if (b == BOOL_TRUE) {
    *res = 1;
  } else if (b == BOOL_FALSE) {
    *res = 0;
  } else {
    snprintf(decode_err, sizeof(decode_err), "expected bool, found %d", b);
    THROW(INVALID_PARAMETER);
  }
}

static void
decode_asset_params(uint8_t **bufp, uint8_t *buf_end, struct asset_params *res)
{
  uint8_t map_count = decode_fixsz(bufp, buf_end, FIXMAP_0, FIXMAP_15);
  for (int i = 0; i < map_count; i++) {
    char key[32];
    decode_string_nullterm(bufp, buf_end, key, sizeof(key));

    if (!strcmp(key, "t")) {
      decode_uint64(bufp, buf_end, &res->total);
    } else if (!strcmp(key, "dc")) {
      decode_uint64(bufp, buf_end, &res->decimals);
    } else if (!strcmp(key, "df")) {
      decode_bool(bufp, buf_end, &res->default_frozen);
    } else if (!strcmp(key, "un")) {
      decode_string(bufp, buf_end, res->unitname, sizeof(res->unitname));
    } else if (!strcmp(key, "an")) {
      decode_string(bufp, buf_end, res->assetname, sizeof(res->assetname));
    } else if (!strcmp(key, "au")) {
      decode_string(bufp, buf_end, res->url, sizeof(res->url));
    } else if (!strcmp(key, "am")) {
      decode_bin_fixed(bufp, buf_end, res->metadata_hash, sizeof(res->metadata_hash));
    } else if (!strcmp(key, "m")) {
      decode_bin_fixed(bufp, buf_end, res->manager, sizeof(res->manager));
    } else if (!strcmp(key, "r")) {
      decode_bin_fixed(bufp, buf_end, res->reserve, sizeof(res->reserve));
    } else if (!strcmp(key, "f")) {
      decode_bin_fixed(bufp, buf_end, res->freeze, sizeof(res->freeze));
    } else if (!strcmp(key, "c")) {
      decode_bin_fixed(bufp, buf_end, res->clawback, sizeof(res->clawback));
    } else {
      snprintf(decode_err, sizeof(decode_err), "unknown params field %s", key);
      THROW(INVALID_PARAMETER);
    }
  }
}

static void
decode_state_schema(uint8_t **bufp, uint8_t *buf_end, struct state_schema *res)
{
  uint8_t map_count = decode_fixsz(bufp, buf_end, FIXMAP_0, FIXMAP_15);
  for (int i = 0; i < map_count; i++) {
    char key[32];
    decode_string_nullterm(bufp, buf_end, key, sizeof(key));
    if (!strcmp(key, "nui")) {
      decode_uint64(bufp, buf_end, &res->num_uint);
    } else if (!strcmp(key, "nbs")) {
      decode_uint64(bufp, buf_end, &res->num_byteslice);
    } else {
      snprintf(decode_err, sizeof(decode_err), "unknown schema field %s", key);
      THROW(INVALID_PARAMETER);
    }
  }
}

#define COUNT(array) \
  (sizeof(array) / sizeof(*array))

static void
decode_app_args(uint8_t **bufp, uint8_t *buf_end, app_args_t *app_args, app_args_len_t *app_arg_len, size_t *num_app_args)
{
  uint8_t arr_count = decode_fixsz(bufp, buf_end, FIXARR_0, FIXARR_15);
  if (arr_count > COUNT(*app_args)) {
    snprintf(decode_err, sizeof(decode_err), "too many app args. max %u", COUNT(*app_args));
    THROW(INVALID_PARAMETER);
  }

  for (int i = 0; i < arr_count; i++) {
    decode_bin_var(bufp, buf_end, (*app_args)[i], &((*app_arg_len)[i]), sizeof((*app_args)[0]));
  }

  *num_app_args = arr_count;
}

static void
decode_accounts(uint8_t **bufp, uint8_t *buf_end, accounts_t *accounts, size_t *num_accounts)
{
  uint8_t arr_count = decode_fixsz(bufp, buf_end, FIXARR_0, FIXARR_15);
  if (arr_count > COUNT(*accounts)) {
    snprintf(decode_err, sizeof(decode_err), "too many accounts. max %u", COUNT(*accounts));
    THROW(INVALID_PARAMETER);
  }

  for (int i = 0; i < arr_count; i++) {
    decode_bin_fixed(bufp, buf_end, (*accounts)[i], sizeof((*accounts)[0]));
  }

  *num_accounts = arr_count;
}

char*
tx_decode(uint8_t *buf, int buflen, struct txn *t)
{
  char* ret = NULL;
  uint8_t* buf_end = buf + buflen;

  os_memset(t, 0, sizeof(*t));

  BEGIN_TRY {
    TRY {
      uint8_t map_count = decode_fixsz(&buf, buf_end, FIXMAP_0, FIXMAP_15);
      for (int i = 0; i < map_count; i++) {
        char key[32];
        decode_string_nullterm(&buf, buf_end, key, sizeof(key));

        // We decode type-specific fields into their union location
        // on the assumption that the caller (host) passed in a valid
        // transaction.  If the transaction contains fields from more
        // than one type of transaction, the in-memory value might be
        // corrupted, but any in-memory representation for the union
        // fields is valid.
        if (!strcmp(key, "type")) {
          char tbuf[16];
          decode_string_nullterm(&buf, buf_end, tbuf, sizeof(tbuf));

          if (!strcmp(tbuf, "pay")) {
            t->type = PAYMENT;
          } else if (!strcmp(tbuf, "keyreg")) {
            t->type = KEYREG;
          } else if (!strcmp(tbuf, "axfer")) {
            t->type = ASSET_XFER;
          } else if (!strcmp(tbuf, "afrz")) {
            t->type = ASSET_FREEZE;
          } else if (!strcmp(tbuf, "acfg")) {
            t->type = ASSET_CONFIG;
          } else if (!strcmp(tbuf, "appl")) {
            t->type = APPLICATION;
          } else {
            snprintf(decode_err, sizeof(decode_err), "unknown tx type %s", tbuf);
            THROW(INVALID_PARAMETER);
          }
        } else if (!strcmp(key, "snd")) {
          decode_bin_fixed(&buf, buf_end, t->sender, sizeof(t->sender));
        } else if (!strcmp(key, "rekey")) {
          decode_bin_fixed(&buf, buf_end, t->rekey, sizeof(t->rekey));
        } else if (!strcmp(key, "fee")) {
          decode_uint64(&buf, buf_end, &t->fee);
        } else if (!strcmp(key, "fv")) {
          decode_uint64(&buf, buf_end, &t->firstValid);
        } else if (!strcmp(key, "lv")) {
          decode_uint64(&buf, buf_end, &t->lastValid);
        } else if (!strcmp(key, "gen")) {
          decode_string(&buf, buf_end, t->genesisID, sizeof(t->genesisID));
        } else if (!strcmp(key, "gh")) {
          decode_bin_fixed(&buf, buf_end, t->genesisHash, sizeof(t->genesisHash));
        } else if (!strcmp(key, "note")) {
          decode_bin_var(&buf, buf_end, t->note, &t->note_len, sizeof(t->note));
        } else if (!strcmp(key, "amt")) {
          decode_uint64(&buf, buf_end, &t->payment.amount);
        } else if (!strcmp(key, "rcv")) {
          decode_bin_fixed(&buf, buf_end, t->payment.receiver, sizeof(t->payment.receiver));
        } else if (!strcmp(key, "close")) {
          decode_bin_fixed(&buf, buf_end, t->payment.close, sizeof(t->payment.close));
        } else if (!strcmp(key, "selkey")) {
          decode_bin_fixed(&buf, buf_end, t->keyreg.vrfpk, sizeof(t->keyreg.vrfpk));
        } else if (!strcmp(key, "votekey")) {
          decode_bin_fixed(&buf, buf_end, t->keyreg.votepk, sizeof(t->keyreg.votepk));
        } else if (!strcmp(key, "votefst")) {
          decode_uint64(&buf, buf_end, &t->keyreg.voteFirst);
        } else if (!strcmp(key, "votelst")) {
          decode_uint64(&buf, buf_end, &t->keyreg.voteLast);
        } else if (!strcmp(key, "votekd")) {
          decode_uint64(&buf, buf_end, &t->keyreg.keyDilution);
        } else if (!strcmp(key, "nonpart")) {
          decode_bool(&buf, buf_end, &t->keyreg.nonpartFlag);
        } else if (!strcmp(key, "aamt")) {
          decode_uint64(&buf, buf_end, &t->asset_xfer.amount);
        } else if (!strcmp(key, "aclose")) {
          decode_bin_fixed(&buf, buf_end, t->asset_xfer.close, sizeof(t->asset_xfer.close));
        } else if (!strcmp(key, "arcv")) {
          decode_bin_fixed(&buf, buf_end, t->asset_xfer.receiver, sizeof(t->asset_xfer.receiver));
        } else if (!strcmp(key, "asnd")) {
          decode_bin_fixed(&buf, buf_end, t->asset_xfer.sender, sizeof(t->asset_xfer.sender));
        } else if (!strcmp(key, "xaid")) {
          decode_uint64(&buf, buf_end, &t->asset_xfer.id);
        } else if (!strcmp(key, "faid")) {
          decode_uint64(&buf, buf_end, &t->asset_freeze.id);
        } else if (!strcmp(key, "fadd")) {
          decode_bin_fixed(&buf, buf_end, t->asset_freeze.account, sizeof(t->asset_freeze.account));
        } else if (!strcmp(key, "afrz")) {
          decode_bool(&buf, buf_end, &t->asset_freeze.flag);
        } else if (!strcmp(key, "caid")) {
          decode_uint64(&buf, buf_end, &t->asset_config.id);
        } else if (!strcmp(key, "apar")) {
          decode_asset_params(&buf, buf_end, &t->asset_config.params);
        } else if (!strcmp(key, "apls")) {
          decode_state_schema(&buf, buf_end, &t->application.local_schema);
        } else if (!strcmp(key, "apgs")) {
          decode_state_schema(&buf, buf_end, &t->application.global_schema);
        } else if (!strcmp(key, "apaa")) {
          decode_app_args(&buf, buf_end, &t->application.app_args, &t->application.app_arg_len, &t->application.num_app_args);
        } else if (!strcmp(key, "apat")) {
          decode_accounts(&buf, buf_end, &t->application.accounts, &t->application.num_accounts);
        } else if (!strcmp(key, "apid")) {
          decode_uint64(&buf, buf_end, &t->application.id);
        } else if (!strcmp(key, "apap")) {
          decode_bin_var(&buf, buf_end, t->application.approv_prog, &t->application.approv_prog_len, sizeof(t->application.approv_prog));
        } else if (!strcmp(key, "apsu")) {
          decode_bin_var(&buf, buf_end, t->application.clear_prog, &t->application.clear_prog_len, sizeof(t->application.clear_prog));
        } else {
          snprintf(decode_err, sizeof(decode_err), "unknown field %s", key);
          THROW(INVALID_PARAMETER);
        }
      }
    }
    CATCH_OTHER(e) {
      ret = &decode_err[0];
    }
    FINALLY {
    }
  }
  END_TRY;

  return ret;
}
