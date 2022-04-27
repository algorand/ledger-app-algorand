#include <stdbool.h>
#include <string.h>
#include "os.h"

#include "algo_tx.h"
#include "msgpack.h"

#define CHECK_ERROR(FUNC_CALL) do { \
    if (!FUNC_CALL) {               \
      return false;                 \
    }                               \
  } while (0)

static char decode_err[64];

static bool
next_byte(uint8_t **bufp, uint8_t *buf_end, uint8_t *b)
{
  if (*bufp >= buf_end) {
    snprintf(decode_err, sizeof(decode_err), "decode past end");
    return false;
  }

  *b = **bufp;
  (*bufp)++;
  return true;
}

static bool
decode_fixsz(uint8_t **bufp, uint8_t *buf_end, uint8_t fix_first, uint8_t fix_last, size_t *arr_count)
{
  uint8_t b;
  CHECK_ERROR(next_byte(bufp, buf_end, &b));

  if (b < fix_first || b > fix_last) {
    snprintf(decode_err, sizeof(decode_err), "decode %d wrong type (%d-%d)", b, fix_first, fix_last);
    return false;
  }

  *arr_count = b - fix_first;
  return true;
}

static bool
decode_mapsz(uint8_t **bufp, uint8_t *buf_end, size_t *map_count)
{
  uint8_t a, b;
  CHECK_ERROR(next_byte(bufp, buf_end, &b));

  if (b >= FIXMAP_0 && b <= FIXMAP_15) {
    *map_count = b - FIXMAP_0;
  } else if (b == MAP16) {
    CHECK_ERROR(next_byte(bufp, buf_end, &a));
    CHECK_ERROR(next_byte(bufp, buf_end, &b));
    *map_count = (a << 8) | b;
  } else {
    snprintf(decode_err, sizeof(decode_err), "expected map, found %d", b);
    return false;
  }

  return true;
}

static bool
decode_string(uint8_t **bufp, uint8_t *buf_end, char *strbuf, size_t strbuflen)
{
  uint8_t b;
  CHECK_ERROR(next_byte(bufp, buf_end, &b));

  uint8_t str_len = 0;
  if (b >= FIXSTR_0 && b <= FIXSTR_31) {
    str_len = b - FIXSTR_0;
  } else if (b == STR8) {
    CHECK_ERROR(next_byte(bufp, buf_end, &str_len));
  } else {
    snprintf(decode_err, sizeof(decode_err), "expected string, found %d", b);
    return false;
  }

  if (str_len >= strbuflen) {
    snprintf(decode_err, sizeof(decode_err), "%d-byte string too big for %d-byte buf", str_len, strbuflen);
    return false;
  }

  if (*bufp + str_len > buf_end) {
    snprintf(decode_err, sizeof(decode_err), "%d-byte string overruns input", str_len);
    return false;
  }

  memmove(strbuf, *bufp, str_len);
  strbuf[str_len] = 0;
  *bufp += str_len;

  return true;
}

static bool
decode_string_nullterm(uint8_t **bufp, uint8_t *buf_end, char *strbuf, size_t strbuflen)
{
  if (strbuflen == 0) {
    snprintf(decode_err, sizeof(decode_err), "decode_string_nullterm: zero strbuflen");
    return false;
  }

  CHECK_ERROR(decode_string(bufp, buf_end, strbuf, strbuflen-1));
  strbuf[strbuflen-1] = '\0';

  return true;
}

static bool
decode_bin_fixed(uint8_t **bufp, uint8_t *buf_end, uint8_t *res, size_t reslen)
{
  uint8_t b;

  CHECK_ERROR(next_byte(bufp, buf_end, &b));
  if (b != BIN8) {
    snprintf(decode_err, sizeof(decode_err), "expected bin, found %d", b);
    return false;
  }

  uint8_t bin_len;

  CHECK_ERROR(next_byte(bufp, buf_end, &bin_len));
  if (bin_len != reslen) {
    snprintf(decode_err, sizeof(decode_err), "expected %d bin bytes, found %d", reslen, bin_len);
    return false;
  }

  if (*bufp + bin_len > buf_end) {
    snprintf(decode_err, sizeof(decode_err), "%d-byte bin overruns input", bin_len);
    return false;
  }

  memmove(res, *bufp, bin_len);
  *bufp += bin_len;

  return true;
}

static bool
decode_bin_var(uint8_t **bufp, uint8_t *buf_end, uint8_t *res, size_t *reslen, size_t reslenmax)
{
  uint8_t a, b;
  uint16_t bin_len = 0;

  CHECK_ERROR(next_byte(bufp, buf_end, &b));
  if (b == BIN8) {
    CHECK_ERROR(next_byte(bufp, buf_end, &a));
    bin_len = a;
  } else if (b == BIN16) {
    CHECK_ERROR(next_byte(bufp, buf_end, &a));
    CHECK_ERROR(next_byte(bufp, buf_end, &b));
    bin_len = (a << 8) | b;
  } else {
    snprintf(decode_err, sizeof(decode_err), "expected bin, found %d", b);
    return false;
  }

  if (bin_len > reslenmax) {
    snprintf(decode_err, sizeof(decode_err), "expected <= %d bin bytes, found %d", reslenmax, bin_len);
    return false;
  }

  if (*bufp + bin_len > buf_end) {
    snprintf(decode_err, sizeof(decode_err), "%d-byte bin overruns input", bin_len);
    return false;
  }

  memmove(res, *bufp, bin_len);
  *bufp += bin_len;
  *reslen = bin_len;

  return true;
}

static bool
decode_uint64(uint8_t **bufp, uint8_t *buf_end, uint64_t *res)
{
  uint8_t b;

  CHECK_ERROR(next_byte(bufp, buf_end, &b));
  if (b >= FIXINT_0 && b <= FIXINT_127) {
    *res = b - FIXINT_0;
    return true;
  }

  size_t size;
  if (b == UINT8) {
    size = 1;
  } else if (b == UINT16) {
    size = 2;
  } else if (b == UINT32) {
    size = 4;
  } else if (b == UINT64) {
    size = 8;
  } else {
    snprintf(decode_err, sizeof(decode_err), "expected u64, found %d", b);
    return false;
  }

  CHECK_ERROR(next_byte(bufp, buf_end, &b));
  *res = b;

  for (size_t i = 1; i < size; i++) {
    CHECK_ERROR(next_byte(bufp, buf_end, &b));
    *res = (*res << 8) | b;
  }

  return true;
}

static bool
decode_bool(uint8_t **bufp, uint8_t *buf_end, uint8_t *res)
{
  uint8_t b;
  CHECK_ERROR(next_byte(bufp, buf_end, &b));
  if (b == BOOL_TRUE) {
    *res = 1;
  } else if (b == BOOL_FALSE) {
    *res = 0;
  } else {
    snprintf(decode_err, sizeof(decode_err), "expected bool, found %d", b);
    return false;
  }
  return true;
}

static bool
decode_asset_params(uint8_t **bufp, uint8_t *buf_end, struct asset_params *res)
{
  size_t map_count;
  CHECK_ERROR(decode_fixsz(bufp, buf_end, FIXMAP_0, FIXMAP_15, &map_count));

  for (size_t i = 0; i < map_count; i++) {
    char key[32];
    CHECK_ERROR(decode_string_nullterm(bufp, buf_end, key, sizeof(key)));

    if (!strcmp(key, "t")) {
      CHECK_ERROR(decode_uint64(bufp, buf_end, &res->total));
    } else if (!strcmp(key, "dc")) {
      CHECK_ERROR(decode_uint64(bufp, buf_end, &res->decimals));
    } else if (!strcmp(key, "df")) {
      CHECK_ERROR(decode_bool(bufp, buf_end, &res->default_frozen));
    } else if (!strcmp(key, "un")) {
      CHECK_ERROR(decode_string(bufp, buf_end, res->unitname, sizeof(res->unitname)));
    } else if (!strcmp(key, "an")) {
      CHECK_ERROR(decode_string(bufp, buf_end, res->assetname, sizeof(res->assetname)));
    } else if (!strcmp(key, "au")) {
      CHECK_ERROR(decode_string(bufp, buf_end, res->url, sizeof(res->url)));
    } else if (!strcmp(key, "am")) {
      CHECK_ERROR(decode_bin_fixed(bufp, buf_end, res->metadata_hash, sizeof(res->metadata_hash)));
    } else if (!strcmp(key, "m")) {
      CHECK_ERROR(decode_bin_fixed(bufp, buf_end, res->manager, sizeof(res->manager)));
    } else if (!strcmp(key, "r")) {
      CHECK_ERROR(decode_bin_fixed(bufp, buf_end, res->reserve, sizeof(res->reserve)));
    } else if (!strcmp(key, "f")) {
      CHECK_ERROR(decode_bin_fixed(bufp, buf_end, res->freeze, sizeof(res->freeze)));
    } else if (!strcmp(key, "c")) {
      CHECK_ERROR(decode_bin_fixed(bufp, buf_end, res->clawback, sizeof(res->clawback)));
    } else {
      snprintf(decode_err, sizeof(decode_err), "unknown params field %s", key);
      return false;
    }
  }

  return true;
}

#define COUNT(array) \
  (sizeof(array) / sizeof(*array))

static bool
decode_accounts(uint8_t **bufp, uint8_t *buf_end, uint8_t accounts[][32], size_t *num_accounts, size_t max_accounts)
{
  size_t arr_count;

  CHECK_ERROR(decode_fixsz(bufp, buf_end, FIXARR_0, FIXARR_15, &arr_count));
  if (arr_count > max_accounts) {
    snprintf(decode_err, sizeof(decode_err), "too many accounts. max %u", max_accounts);
    return false;
  }

  for (size_t i = 0; i < arr_count; i++) {
    CHECK_ERROR(decode_bin_fixed(bufp, buf_end, accounts[i], sizeof(accounts[0])));
  }

  *num_accounts = arr_count;
  return true;
}

static bool
decode_app_args(uint8_t **bufp, uint8_t *buf_end, uint8_t app_args[][MAX_ARGLEN], size_t app_args_len[], size_t *num_args, size_t max_args) {
  size_t arr_count;

  CHECK_ERROR(decode_fixsz(bufp, buf_end, FIXARR_0, FIXARR_15, &arr_count));
  if (arr_count > max_args) {
    snprintf(decode_err, sizeof(decode_err), "too many args. max %u", max_args);
    return false;
  }

  for (size_t i = 0; i < arr_count; i++) {
    CHECK_ERROR(decode_bin_var(bufp, buf_end, app_args[i], &app_args_len[i], sizeof(app_args[0])));
  }

  *num_args = arr_count;
  return true;
}

static bool
decode_u64_array(uint8_t **bufp, uint8_t *buf_end, uint64_t elems[], size_t *num_elems, size_t max_elems, const char *elem_name) {
  size_t arr_count;

  CHECK_ERROR(decode_fixsz(bufp, buf_end, FIXARR_0, FIXARR_15, &arr_count));
  if (arr_count > max_elems) {
    snprintf(decode_err, sizeof(decode_err), "too many %s. max %u", elem_name, max_elems);
    return false;
  }

  for (size_t i = 0; i < arr_count; i++) {
    CHECK_ERROR(decode_uint64(bufp, buf_end, &elems[i]));
  }

  *num_elems = arr_count;
  return true;
}

static bool
decode_state_schema(uint8_t **bufp, uint8_t *buf_end, struct state_schema *res)
{
  size_t map_count;

  CHECK_ERROR(decode_fixsz(bufp, buf_end, FIXMAP_0, FIXMAP_15, &map_count));
  for (size_t i = 0; i < map_count; i++) {
    char key[32];
    CHECK_ERROR(decode_string_nullterm(bufp, buf_end, key, sizeof(key)));
    if (!strcmp(key, "nui")) {
      CHECK_ERROR(decode_uint64(bufp, buf_end, &res->num_uint));
    } else if (!strcmp(key, "nbs")) {
      CHECK_ERROR(decode_uint64(bufp, buf_end, &res->num_byteslice));
    } else {
      snprintf(decode_err, sizeof(decode_err), "unknown schema field %s", key);
      return false;
    }
  }
  return true;
}

static bool tx_decode_helper(uint8_t *buf, uint8_t *buf_end, txn_t *t)
{
  size_t map_count;

  CHECK_ERROR(decode_mapsz(&buf, buf_end, &map_count));

  for (size_t i = 0; i < map_count; i++) {
    char key[32];
    CHECK_ERROR(decode_string_nullterm(&buf, buf_end, key, sizeof(key)));

    // We decode type-specific fields into their union location
    // on the assumption that the caller (host) passed in a valid
    // transaction.  If the transaction contains fields from more
    // than one type of transaction, the in-memory value might be
    // corrupted, but any in-memory representation for the union
    // fields is valid (though lengths must be checked before use).
    if (!strcmp(key, "type")) {
      char tbuf[16];
      CHECK_ERROR(decode_string_nullterm(&buf, buf_end, tbuf, sizeof(tbuf)));

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
        return false;
      }
    } else if (!strcmp(key, "snd")) {
      CHECK_ERROR(decode_bin_fixed(&buf, buf_end, t->sender, sizeof(t->sender)));
    } else if (!strcmp(key, "rekey")) {
      CHECK_ERROR(decode_bin_fixed(&buf, buf_end, t->rekey, sizeof(t->rekey)));
    } else if (!strcmp(key, "fee")) {
      CHECK_ERROR(decode_uint64(&buf, buf_end, &t->fee));
    } else if (!strcmp(key, "fv")) {
      CHECK_ERROR(decode_uint64(&buf, buf_end, &t->firstValid));
    } else if (!strcmp(key, "lv")) {
      CHECK_ERROR(decode_uint64(&buf, buf_end, &t->lastValid));
    } else if (!strcmp(key, "gen")) {
      CHECK_ERROR(decode_string(&buf, buf_end, t->genesisID, sizeof(t->genesisID)));
    } else if (!strcmp(key, "gh")) {
      CHECK_ERROR(decode_bin_fixed(&buf, buf_end, t->genesisHash, sizeof(t->genesisHash)));
    } else if (!strcmp(key, "grp")) {
      CHECK_ERROR(decode_bin_fixed(&buf, buf_end, t->groupID, sizeof(t->groupID)));
    } else if (!strcmp(key, "note")) {
      CHECK_ERROR(decode_bin_var(&buf, buf_end, t->note, &t->note_len, sizeof(t->note)));
    } else if (!strcmp(key, "amt")) {
      CHECK_ERROR(decode_uint64(&buf, buf_end, &t->payment.amount));
    } else if (!strcmp(key, "rcv")) {
      CHECK_ERROR(decode_bin_fixed(&buf, buf_end, t->payment.receiver, sizeof(t->payment.receiver)));
    } else if (!strcmp(key, "close")) {
      CHECK_ERROR(decode_bin_fixed(&buf, buf_end, t->payment.close, sizeof(t->payment.close)));
    } else if (!strcmp(key, "selkey")) {
      CHECK_ERROR(decode_bin_fixed(&buf, buf_end, t->keyreg.vrfpk, sizeof(t->keyreg.vrfpk)));
    } else if (!strcmp(key, "sprfkey")) {
      CHECK_ERROR(decode_bin_fixed(&buf, buf_end, t->keyreg.sprfkey, sizeof(t->keyreg.sprfkey)));
    } else if (!strcmp(key, "votekey")) {
      CHECK_ERROR(decode_bin_fixed(&buf, buf_end, t->keyreg.votepk, sizeof(t->keyreg.votepk)));
    } else if (!strcmp(key, "votefst")) {
      CHECK_ERROR(decode_uint64(&buf, buf_end, &t->keyreg.voteFirst));
    } else if (!strcmp(key, "votelst")) {
      CHECK_ERROR(decode_uint64(&buf, buf_end, &t->keyreg.voteLast));
    } else if (!strcmp(key, "votekd")) {
      CHECK_ERROR(decode_uint64(&buf, buf_end, &t->keyreg.keyDilution));
    } else if (!strcmp(key, "nonpart")) {
      CHECK_ERROR(decode_bool(&buf, buf_end, &t->keyreg.nonpartFlag));
    } else if (!strcmp(key, "aamt")) {
      CHECK_ERROR(decode_uint64(&buf, buf_end, &t->asset_xfer.amount));
    } else if (!strcmp(key, "aclose")) {
      CHECK_ERROR(decode_bin_fixed(&buf, buf_end, t->asset_xfer.close, sizeof(t->asset_xfer.close)));
    } else if (!strcmp(key, "arcv")) {
      CHECK_ERROR(decode_bin_fixed(&buf, buf_end, t->asset_xfer.receiver, sizeof(t->asset_xfer.receiver)));
    } else if (!strcmp(key, "asnd")) {
      CHECK_ERROR(decode_bin_fixed(&buf, buf_end, t->asset_xfer.sender, sizeof(t->asset_xfer.sender)));
    } else if (!strcmp(key, "xaid")) {
      CHECK_ERROR(decode_uint64(&buf, buf_end, &t->asset_xfer.id));
    } else if (!strcmp(key, "faid")) {
      CHECK_ERROR(decode_uint64(&buf, buf_end, &t->asset_freeze.id));
    } else if (!strcmp(key, "fadd")) {
      CHECK_ERROR(decode_bin_fixed(&buf, buf_end, t->asset_freeze.account, sizeof(t->asset_freeze.account)));
    } else if (!strcmp(key, "afrz")) {
      CHECK_ERROR(decode_bool(&buf, buf_end, &t->asset_freeze.flag));
    } else if (!strcmp(key, "caid")) {
      CHECK_ERROR(decode_uint64(&buf, buf_end, &t->asset_config.id));
    } else if (!strcmp(key, "apar")) {
      CHECK_ERROR(decode_asset_params(&buf, buf_end, &t->asset_config.params));
    } else if (!strcmp(key, "apid")) {
      CHECK_ERROR(decode_uint64(&buf, buf_end, &t->application.id));
    } else if (!strcmp(key, "apaa")) {
      CHECK_ERROR(decode_app_args(&buf, buf_end, t->application.app_args, t->application.app_args_len, &t->application.num_app_args, COUNT(t->application.app_args)));
    } else if (!strcmp(key, "apap")) {
      CHECK_ERROR(decode_bin_var(&buf, buf_end, t->application.aprog, &t->application.aprog_len, sizeof(t->application.aprog)));
    } else if (!strcmp(key, "apsu")) {
      CHECK_ERROR(decode_bin_var(&buf, buf_end, t->application.cprog, &t->application.cprog_len, sizeof(t->application.cprog)));
    } else if (!strcmp(key, "apan")) {
      CHECK_ERROR(decode_uint64(&buf, buf_end, &t->application.oncompletion));
    } else if (!strcmp(key, "apat")) {
      CHECK_ERROR(decode_accounts(&buf, buf_end, t->application.accounts, &t->application.num_accounts, COUNT(t->application.accounts)));
    } else if (!strcmp(key, "apls")) {
      CHECK_ERROR(decode_state_schema(&buf, buf_end, &t->application.local_schema));
    } else if (!strcmp(key, "apgs")) {
      CHECK_ERROR(decode_state_schema(&buf, buf_end, &t->application.global_schema));
    } else if (!strcmp(key, "apfa")) {
      CHECK_ERROR(decode_u64_array(&buf, buf_end, t->application.foreign_apps, &t->application.num_foreign_apps, COUNT(t->application.foreign_apps), "foreign apps"));
    } else if (!strcmp(key, "apas")) {
      CHECK_ERROR(decode_u64_array(&buf, buf_end, t->application.foreign_assets, &t->application.num_foreign_assets, COUNT(t->application.foreign_assets), "foreign assets"));
    } else {
      snprintf(decode_err, sizeof(decode_err), "unknown field %s", key);
      return false;
    }
  }

  // Check that lengths are not dangerous values
  if (t->type == APPLICATION) {
    if (t->application.cprog_len > COUNT(t->application.cprog)) {
      snprintf(decode_err, sizeof(decode_err), "invalid cprog length");
      return false;
    }
    if (t->application.aprog_len > COUNT(t->application.aprog)) {
      snprintf(decode_err, sizeof(decode_err), "invalid cprog length");
      return false;
    }
    if (t->application.num_accounts > COUNT(t->application.accounts)) {
      snprintf(decode_err, sizeof(decode_err), "invalid num accounts");
      return false;
    }
    if (t->application.num_app_args > COUNT(t->application.app_args)) {
      snprintf(decode_err, sizeof(decode_err), "invalid num args");
      return false;
    }
    if (t->application.num_foreign_apps > COUNT(t->application.foreign_apps)) {
      snprintf(decode_err, sizeof(decode_err), "invalid num foreign apps");
      return false;
    }
    if (t->application.num_foreign_assets > COUNT(t->application.foreign_assets)) {
      snprintf(decode_err, sizeof(decode_err), "invalid num foreign assets");
      return false;
    }
    for (unsigned int i = 0; i < COUNT(t->application.app_args_len); i++) {
      if (t->application.app_args_len[i] > COUNT(t->application.app_args[0])) {
        snprintf(decode_err, sizeof(decode_err), "invalid arg len");
        return false;
      }
    }
  }

  return true;
}

char*
tx_decode(uint8_t *buf, int buflen, txn_t *t)
{
  uint8_t* buf_end = buf + buflen;

  if (!tx_decode_helper(buf, buf_end, t)) {
    return &decode_err[0];
  }

  return NULL;
}
