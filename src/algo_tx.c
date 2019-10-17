#include "algo_tx.h"
#include <string.h>
#include "msgpack.h"

//------------------------------------------------------------------------------

static uint8_t*
put_byte(uint8_t *p, uint8_t *e, uint8_t b);
static uint8_t*
encode_str(uint8_t *p, uint8_t *e, const char *s);
static uint8_t*
encode_uint64(uint8_t *p, uint8_t *e, uint64_t i);
static uint8_t*
encode_bin(uint8_t *p, uint8_t *e, uint8_t *bytes, unsigned int len);
static uint8_t
map_kv_uint64(uint8_t **p, uint8_t *e, char *key, uint64_t val);
static uint8_t
map_kv_str(uint8_t **p, uint8_t *e, char *key, char *val);
static uint8_t
map_kv_bin(uint8_t **p, uint8_t *e, char *key, uint8_t *val, unsigned int len);

//------------------------------------------------------------------------------

unsigned int
tx_encode(struct tx_t *t, uint8_t *buf, unsigned int buflen)
{
  uint8_t *p, *e;

  buf[0] = FIXMAP_0;
  p = buf + 1;
  e = buf + buflen;

  // Fill in the fields in sorted key order, bumping the
  // number of map elements as we go if they are non-zero.
  switch (t->type) {
    case PAYMENT:
      buf[0] += map_kv_uint64(&p, e, "amt", t->payload.payment.amount);
      buf[0] += map_kv_bin   (&p, e, "close", t->payload.payment.close, sizeof(t->payload.payment.close));
      buf[0] += map_kv_uint64(&p, e, "fee", t->fee);
      buf[0] += map_kv_uint64(&p, e, "fv", t->firstValid);
      buf[0] += map_kv_str   (&p, e, "gen", t->genesisID);
      buf[0] += map_kv_bin   (&p, e, "gh", t->genesisHash, sizeof(t->genesisHash));
      buf[0] += map_kv_uint64(&p, e, "lv", t->lastValid);
      buf[0] += map_kv_bin   (&p, e, "note", t->note, t->note_size);
      buf[0] += map_kv_bin   (&p, e, "rcv", t->payload.payment.receiver, sizeof(t->payload.payment.receiver));
      buf[0] += map_kv_bin   (&p, e, "snd", t->sender, sizeof(t->sender));
      buf[0] += map_kv_str   (&p, e, "type", "pay");
      break;

    case KEYREG:
      buf[0] += map_kv_uint64(&p, e, "fee", t->fee);
      buf[0] += map_kv_uint64(&p, e, "fv", t->firstValid);
      buf[0] += map_kv_str   (&p, e, "gen", t->genesisID);
      buf[0] += map_kv_bin   (&p, e, "gh", t->genesisHash, sizeof(t->genesisHash));
      buf[0] += map_kv_uint64(&p, e, "lv", t->lastValid);
      buf[0] += map_kv_bin   (&p, e, "note", t->note, t->note_size);
      buf[0] += map_kv_bin   (&p, e, "selkey", t->payload.keyreg.vrfpk, sizeof(t->payload.keyreg.vrfpk));
      buf[0] += map_kv_bin   (&p, e, "snd", t->sender, sizeof(t->sender));
      buf[0] += map_kv_str   (&p, e, "type", "keyreg");
      buf[0] += map_kv_bin   (&p, e, "votekey", t->payload.keyreg.votepk, sizeof(t->payload.keyreg.votepk));
      break;

    default:
      PRINTF("Unknown transaction type %d\n", t->type);
      break;
  }
  return p - buf;
}

//------------------------------------------------------------------------------

static uint8_t*
put_byte(uint8_t *p, uint8_t *e, uint8_t b)
{
  if (p < e) {
    *p = b;
    p++;
  }
  return p;
}

static uint8_t*
encode_str(uint8_t *p, uint8_t *e, const char *s)
{
  int len = strlen(s);
  int i;

  if (len <= FIXSTR_31 - FIXSTR_0) {
    p = put_byte(p, e, FIXSTR_0 + len);
    for (i = 0; i < len; i++) {
      p = put_byte(p, e, s[i]);
    }
  }
  else if (len < (1 << 8)) {
    p = put_byte(p, e, STR8);
    p = put_byte(p, e, len);
    for (i = 0; i < len; i++) {
      p = put_byte(p, e, s[i]);
    }
  }
  else {
    // Longer strings not supported
    os_sched_exit(0);
  }
  return p;
}

static uint8_t*
encode_uint64(uint8_t *p, uint8_t *e, uint64_t i)
{
  if (i <= FIXINT_127 - FIXINT_0) {
    p = put_byte(p, e, FIXINT_0 + i);
  }
  else if (i < (1ULL << 8)) {
    p = put_byte(p, e, UINT8);
    p = put_byte(p, e, i);
  }
  else if (i < (1ULL << 16)) {
    p = put_byte(p, e, UINT16);
    p = put_byte(p, e, i >> 8);
    p = put_byte(p, e, i & 0xff);
  }
  else if (i < (1ULL << 32)) {
    p = put_byte(p, e, UINT32);
    p = put_byte(p, e, i >> 24);
    p = put_byte(p, e, (i >> 16) & 0xff);
    p = put_byte(p, e, (i >> 8) & 0xff);
    p = put_byte(p, e, i & 0xff);
  }
  else {
    p = put_byte(p, e, UINT64);
    p = put_byte(p, e, i >> 56);
    p = put_byte(p, e, (i >> 48) & 0xff);
    p = put_byte(p, e, (i >> 40) & 0xff);
    p = put_byte(p, e, (i >> 32) & 0xff);
    p = put_byte(p, e, (i >> 24) & 0xff);
    p = put_byte(p, e, (i >> 16) & 0xff);
    p = put_byte(p, e, (i >> 8) & 0xff);
    p = put_byte(p, e, i & 0xff);
  }
  return p;
}

static uint8_t*
encode_bin(uint8_t *p, uint8_t *e, uint8_t *bytes, unsigned int len)
{
  unsigned int i;

  if (len < (1 << 8)) {
    p = put_byte(p, e, BIN8);
    p = put_byte(p, e, len);
    for (i = 0; i < len; i++) {
      p = put_byte(p, e, bytes[i]);
    }
  }
  else if (len < (1 << 16)) {
    p = put_byte(p, e, BIN16);
    p = put_byte(p, e, len >> 8);
    p = put_byte(p, e, len & 0xff);
    for (i = 0; i < len; i++) {
      p = put_byte(p, e, bytes[i]);
    }
  }
  else {
    // Longer binary blobs not suppported
    os_sched_exit(0);
  }
  return p;
}

static uint8_t
map_kv_uint64(uint8_t **p, uint8_t *e, char *key, uint64_t val)
{
  if (val == 0) {
    return 0;
  }
  *p = encode_str(*p, e, key);
  *p = encode_uint64(*p, e, val);
  return 1;
}

static uint8_t
map_kv_str(uint8_t **p, uint8_t *e, char *key, char *val)
{
  if (strlen(val) == 0) {
    return 0;
  }

  *p = encode_str(*p, e, key);
  *p = encode_str(*p, e, val);
  return 1;
}

static uint8_t
map_kv_bin(uint8_t **p, uint8_t *e, char *key, uint8_t *val, unsigned int len)
{
  unsigned int i;

  for (i = 0; i < len; i++) {
    if (val[i] != 0) {
      //got a non-zero value
      *p = encode_str(*p, e, key);
      *p = encode_bin(*p, e, val, len);
      return 1;
    }
  }
  return 0;
}
