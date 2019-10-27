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

  if (str_len >= strbuflen) {
    snprintf(decode_err, sizeof(decode_err), "%d-byte string too big for %d-byte buf", str_len, strbuflen);
    THROW(INVALID_PARAMETER);
  }

  if (*bufp + str_len > buf_end) {
    snprintf(decode_err, sizeof(decode_err), "%d-byte string overruns input", str_len);
    THROW(INVALID_PARAMETER);
  }

  os_memmove(strbuf, *bufp, str_len);
  strbuf[str_len] = 0;
  *bufp += str_len;
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
        decode_string(&buf, buf_end, key, sizeof(key));

        if (!strcmp(key, "type")) {
          char tbuf[16];
          decode_string(&buf, buf_end, tbuf, sizeof(tbuf));

          if (!strcmp(tbuf, "pay")) {
            t->type = PAYMENT;
          } else if (!strcmp(tbuf, "keyreg")) {
            t->type = KEYREG;
          } else {
            snprintf(decode_err, sizeof(decode_err), "unknown tx type %s", tbuf);
            THROW(INVALID_PARAMETER);
          }
        } else if (!strcmp(key, "snd")) {
          decode_bin_fixed(&buf, buf_end, t->sender, sizeof(t->sender));
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
        } else if (!strcmp(key, "amt")) {
          decode_uint64(&buf, buf_end, &t->amount);
        } else if (!strcmp(key, "rcv")) {
          decode_bin_fixed(&buf, buf_end, t->receiver, sizeof(t->receiver));
        } else if (!strcmp(key, "close")) {
          decode_bin_fixed(&buf, buf_end, t->close, sizeof(t->close));
        } else if (!strcmp(key, "selkey")) {
          decode_bin_fixed(&buf, buf_end, t->vrfpk, sizeof(t->vrfpk));
        } else if (!strcmp(key, "votekey")) {
          decode_bin_fixed(&buf, buf_end, t->votepk, sizeof(t->votepk));
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
