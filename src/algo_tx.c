#include <string.h>
#include "os.h"

#include "algo_tx.h"
#include "msgpack.h"

static void
put_byte(uint8_t **p, uint8_t *e, uint8_t b)
{
  if (*p < e) {
    *((*p)++) = b;
  }
}

static void
encode_str(uint8_t **p, uint8_t *e, const char *s, size_t maxlen)
{
  int len = strnlen(s, maxlen);
  if (len <= FIXSTR_31 - FIXSTR_0) {
    put_byte(p, e, FIXSTR_0 + len);
    for (int i = 0; i < len; i++) {
      put_byte(p, e, s[i]);
    }
    return;
  }

  if (len < (1 << 8)) {
    put_byte(p, e, STR8);
    put_byte(p, e, len);
    for (int i = 0; i < len; i++) {
      put_byte(p, e, s[i]);
    }
    return;
  }

  // Longer strings not supported
  os_sched_exit(0);
}

static void
encode_uint64(uint8_t **p, uint8_t *e, uint64_t i)
{
  if (i <= FIXINT_127 - FIXINT_0) {
    put_byte(p, e, FIXINT_0 + i);
    return;
  }

  if (i < (1ULL << 8)) {
    put_byte(p, e, UINT8);
    put_byte(p, e, i);
    return;
  }

  if (i < (1ULL << 16)) {
    put_byte(p, e, UINT16);
    put_byte(p, e, i >> 8);
    put_byte(p, e, i & 0xff);
    return;
  }

  if (i < (1ULL << 32)) {
    put_byte(p, e, UINT32);
    put_byte(p, e, i >> 24);
    put_byte(p, e, (i >> 16) & 0xff);
    put_byte(p, e, (i >> 8) & 0xff);
    put_byte(p, e, i & 0xff);
    return;
  }

  put_byte(p, e, UINT64);
  put_byte(p, e, i >> 56);
  put_byte(p, e, (i >> 48) & 0xff);
  put_byte(p, e, (i >> 40) & 0xff);
  put_byte(p, e, (i >> 32) & 0xff);
  put_byte(p, e, (i >> 24) & 0xff);
  put_byte(p, e, (i >> 16) & 0xff);
  put_byte(p, e, (i >> 8) & 0xff);
  put_byte(p, e, i & 0xff);
}

static void
encode_bool(uint8_t **p, uint8_t *e, uint8_t v)
{
  if (v) {
    put_byte(p, e, BOOL_TRUE);
  } else {
    put_byte(p, e, BOOL_FALSE);
  }
}

static void
encode_bin(uint8_t **p, uint8_t *e, uint8_t *bytes, int len)
{
  if (len < (1 << 8)) {
    put_byte(p, e, BIN8);
    put_byte(p, e, len);
    for (int i = 0; i < len; i++) {
      put_byte(p, e, bytes[i]);
    }
    return;
  }

  // Longer binary blobs not suppported
  os_sched_exit(0);
}

static int
map_kv_bool(uint8_t **p, uint8_t *e, char *key, uint8_t val)
{
  if (val == 0) {
    return 0;
  }

  encode_str(p, e, key, SIZE_MAX);
  encode_bool(p, e, val);
  return 1;
}

static int
map_kv_uint64(uint8_t **p, uint8_t *e, char *key, uint64_t val)
{
  if (val == 0) {
    return 0;
  }

  encode_str(p, e, key, SIZE_MAX);
  encode_uint64(p, e, val);
  return 1;
}

static int
map_kv_str(uint8_t **p, uint8_t *e, char *key, char *val, size_t maxlen)
{
  if (strnlen(val, maxlen) == 0) {
    return 0;
  }

  encode_str(p, e, key, SIZE_MAX);
  encode_str(p, e, val, maxlen);
  return 1;
}

static int
map_kv_bin(uint8_t **p, uint8_t *e, char *key, uint8_t *valbuf, int vallen)
{
  int allzero = 1;

  for (int i = 0; i < vallen; i++) {
    if (valbuf[i] != 0) {
      allzero = 0;
    }
  }

  if (allzero) {
    return 0;
  }

  encode_str(p, e, key, SIZE_MAX);
  encode_bin(p, e, valbuf, vallen);
  return 1;
}

unsigned int
tx_encode(struct txn *t, uint8_t *buf, int buflen)
{
  char *typestr;
  switch (t->type) {
  case PAYMENT:
    typestr = "pay";
    break;

  case KEYREG:
    typestr = "keyreg";
    break;

  case ASSET_XFER:
    typestr = "axfer";
    break;

  case ASSET_FREEZE:
    typestr = "afrz";
    break;

  case ASSET_CONFIG:
    typestr = "acfg";
    break;

  default:
    PRINTF("Unknown transaction type %d\n", t->type);
    typestr = "unknown";
  }

  uint8_t *p = buf;
  *(p++) = FIXMAP_0;

  uint8_t *e = &buf[buflen];

#define T(expected, encoder)                        \
  ({                                                \
    int res = 0;                                    \
    if (t->type == expected) { res = encoder; }     \
    res;                                            \
  })

  // Fill in the fields in sorted key order, bumping the
  // number of map elements as we go if they are non-zero.
  // Type-specific fields are encoded only if the type matches.
  buf[0] += T(ASSET_XFER,   map_kv_uint64(&p, e, "aamt", t->asset_xfer.amount));
  buf[0] += T(ASSET_XFER,   map_kv_bin   (&p, e, "aclose", t->asset_xfer.close, sizeof(t->asset_xfer.close)));
  buf[0] += T(ASSET_FREEZE, map_kv_bool  (&p, e, "afrz", t->asset_freeze.flag));
  buf[0] += T(PAYMENT,      map_kv_uint64(&p, e, "amt", t->payment.amount));
  buf[0] += T(ASSET_XFER,   map_kv_bin   (&p, e, "arcv", t->asset_xfer.receiver, sizeof(t->asset_xfer.receiver)));
  buf[0] += T(ASSET_XFER,   map_kv_bin   (&p, e, "asnd", t->asset_xfer.sender, sizeof(t->asset_xfer.sender)));
  buf[0] += T(PAYMENT,      map_kv_bin   (&p, e, "close", t->payment.close, sizeof(t->payment.close)));
  buf[0] += T(ASSET_FREEZE, map_kv_bin   (&p, e, "fadd", t->asset_freeze.account, sizeof(t->asset_freeze.account)));
  buf[0] += T(ASSET_FREEZE, map_kv_uint64(&p, e, "faid", t->asset_freeze.id));
  buf[0] +=                 map_kv_uint64(&p, e, "fee", t->fee);
  buf[0] +=                 map_kv_uint64(&p, e, "fv", t->firstValid);
  buf[0] +=                 map_kv_str   (&p, e, "gen", t->genesisID, sizeof(t->genesisID));
  buf[0] +=                 map_kv_bin   (&p, e, "gh", t->genesisHash, sizeof(t->genesisHash));
  buf[0] +=                 map_kv_uint64(&p, e, "lv", t->lastValid);
  buf[0] +=                 map_kv_bin   (&p, e, "note", t->note, t->note_len);
  buf[0] += T(PAYMENT,      map_kv_bin   (&p, e, "rcv", t->payment.receiver, sizeof(t->payment.receiver)));
  buf[0] += T(KEYREG,       map_kv_bin   (&p, e, "selkey", t->keyreg.vrfpk, sizeof(t->keyreg.vrfpk)));
  buf[0] +=                 map_kv_bin   (&p, e, "snd", t->sender, sizeof(t->sender));
  buf[0] +=                 map_kv_str   (&p, e, "type", typestr, SIZE_MAX);
  buf[0] += T(KEYREG,       map_kv_bin   (&p, e, "votekey", t->keyreg.votepk, sizeof(t->keyreg.votepk)));
  buf[0] += T(ASSET_XFER,   map_kv_uint64(&p, e, "xaid", t->asset_xfer.id));

#undef T

  return p-buf;
}
