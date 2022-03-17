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
  } else if (len < (1 << 16)) {
    put_byte(p, e, BIN16);
    put_byte(p, e, len >> 8);
    put_byte(p, e, len & 0xFF);
  } else {
    // Longer binary blobs not suppported
    os_sched_exit(0);
  }

  for (int i = 0; i < len; i++) {
    put_byte(p, e, bytes[i]);
  }
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

static int
map_kv_accts(uint8_t **p, uint8_t *e, char *key, uint8_t accounts[][32], size_t num_accounts)
{
  if (num_accounts == 0) {
    return 0;
  }

  if (num_accounts > FIXARR_15 - FIXARR_0) {
    os_sched_exit(0);
  }

  encode_str(p, e, key, SIZE_MAX);

  uint8_t *arrbase = *p;
  if (*p >= e) {
    // We need to access arrbase[0] below, so if there isn't space for at least
    // one byte, bail out.
    return 0;
  }

  put_byte(p, e, FIXARR_0);
  for (size_t i = 0; i < num_accounts; i++) {
    encode_bin(p, e, accounts[i], sizeof(accounts[0]));
    arrbase[0]++;
  }

  return 1;
}

static int
map_kv_args(uint8_t **p, uint8_t *e, char *key, uint8_t app_args[][MAX_ARGLEN], size_t app_args_len[], size_t num_app_args)
{
  if (num_app_args == 0) {
    return 0;
  }

  if (num_app_args > FIXARR_15 - FIXARR_0) {
    os_sched_exit(0);
  }

  encode_str(p, e, key, SIZE_MAX);

  uint8_t *arrbase = *p;
  if (*p >= e) {
    // We need to access arrbase[0] below, so if there isn't space for at least
    // one byte, bail out.
    return 0;
  }

  put_byte(p, e, FIXARR_0);
  for (size_t i = 0; i < num_app_args; i++) {
    encode_bin(p, e, app_args[i], app_args_len[i]);
    arrbase[0]++;
  }

  return 1;
}

static int
map_kv_u64arr(uint8_t **p, uint8_t *e, char *key, uint64_t elems[], size_t num_elems)
{
  if (num_elems == 0) {
    return 0;
  }

  if (num_elems > FIXARR_15 - FIXARR_0) {
    os_sched_exit(0);
  }

  encode_str(p, e, key, SIZE_MAX);

  uint8_t *arrbase = *p;
  if (*p >= e) {
    // We need to access arrbase[0] below, so if there isn't space for at least
    // one byte, bail out.
    return 0;
  }

  put_byte(p, e, FIXARR_0);
  for (size_t i = 0; i < num_elems; i++) {
    encode_uint64(p, e, elems[i]);
    arrbase[0]++;
  }

  return 1;
}

static int
map_kv_schema(uint8_t **p, uint8_t *e, char *key, struct state_schema *schema)
{
  // Save original buffer in case we end up with a zero value
  uint8_t *psave = *p;

  encode_str(p, e, key, SIZE_MAX);

  uint8_t *mapbase = *p;
  if (*p >= e) {
    // We need to access mapbase[0] below, so if there isn't space for at least
    // one byte, bail out.
    return 0;
  }

  put_byte(p, e, FIXMAP_0);

  mapbase[0] += map_kv_uint64(p, e, "nbs", schema->num_byteslice);
  mapbase[0] += map_kv_uint64(p, e, "nui", schema->num_uint);

  if (mapbase[0] == FIXMAP_0) {
    // No keys is a zero value; roll back any changes
    *p = psave;
    return 0;
  }

  return 1;
}

static int
map_kv_params(uint8_t **p, uint8_t *e, char *key, struct asset_params *params)
{
  // Save original buffer in case we end up with a zero value
  uint8_t *psave = *p;

  encode_str(p, e, key, SIZE_MAX);

  uint8_t *mapbase = *p;
  if (*p >= e) {
    // We need to access mapbase[0] below, so if there isn't space for at least
    // one byte, bail out.
    return 0;
  }

  put_byte(p, e, FIXMAP_0);

  mapbase[0] += map_kv_bin   (p, e, "am", params->metadata_hash, sizeof(params->metadata_hash));
  mapbase[0] += map_kv_str   (p, e, "an", params->assetname, sizeof(params->assetname));
  mapbase[0] += map_kv_str   (p, e, "au", params->url, sizeof(params->url));
  mapbase[0] += map_kv_bin   (p, e, "c",  params->clawback, sizeof(params->clawback));
  mapbase[0] += map_kv_uint64(p, e, "dc", params->decimals);
  mapbase[0] += map_kv_bool  (p, e, "df", params->default_frozen);
  mapbase[0] += map_kv_bin   (p, e, "f",  params->freeze, sizeof(params->freeze));
  mapbase[0] += map_kv_bin   (p, e, "m",  params->manager, sizeof(params->manager));
  mapbase[0] += map_kv_bin   (p, e, "r",  params->reserve, sizeof(params->reserve));
  mapbase[0] += map_kv_uint64(p, e, "t",  params->total);
  mapbase[0] += map_kv_str   (p, e, "un", params->unitname, sizeof(params->unitname));

  if (mapbase[0] == FIXMAP_0) {
    // No keys is a zero value; roll back any changes
    *p = psave;
    return 0;
  }

  return 1;
}

unsigned int
tx_encode(txn_t *t, uint8_t *buf, int buflen)
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

  case APPLICATION:
    typestr = "appl";
    break;

  default:
    PRINTF("Unknown transaction type %d\n", t->type);
    typestr = "unknown";
  }

  // Must be able to at least store map header in the worst case
  if (buflen < 3) {
    os_sched_exit(0);
  }

  uint8_t *p = buf;
  uint8_t *e = &buf[buflen];

  // Start off assuming we will need the larger map type (3 bytes) versus the
  // smaller map type (1 byte). This way we can shift everything back two bytes
  // at the end without having to keep track of how much space we have left.
  put_byte(&p, e, 0);
  put_byte(&p, e, 0);
  put_byte(&p, e, 0);

#define T(expected, encoder)                        \
  ({                                                \
    int res = 0;                                    \
    if (t->type == expected) { res = encoder; }     \
    res;                                            \
  })

  // Fill in the fields in sorted key order, bumping the
  // number of map elements as we go if they are non-zero.
  // Type-specific fields are encoded only if the type matches.
  uint8_t fields = 0;
  fields += T(ASSET_XFER,   map_kv_uint64(&p, e, "aamt",    t->asset_xfer.amount));
  fields += T(ASSET_XFER,   map_kv_bin   (&p, e, "aclose",  t->asset_xfer.close, sizeof(t->asset_xfer.close)));
  fields += T(ASSET_FREEZE, map_kv_bool  (&p, e, "afrz",    t->asset_freeze.flag));
  fields += T(PAYMENT,      map_kv_uint64(&p, e, "amt",     t->payment.amount));
  fields += T(APPLICATION,  map_kv_args  (&p, e, "apaa",    t->application.app_args, t->application.app_args_len, t->application.num_app_args));
  fields += T(APPLICATION,  map_kv_uint64(&p, e, "apan",    t->application.oncompletion));
  fields += T(APPLICATION,  map_kv_bin   (&p, e, "apap",    t->application.aprog, t->application.aprog_len));
  fields += T(ASSET_CONFIG, map_kv_params(&p, e, "apar",    &t->asset_config.params));
  fields += T(APPLICATION,  map_kv_u64arr(&p, e, "apas",    t->application.foreign_assets, t->application.num_foreign_assets));
  fields += T(APPLICATION,  map_kv_accts (&p, e, "apat",    t->application.accounts, t->application.num_accounts));
  fields += T(APPLICATION,  map_kv_u64arr(&p, e, "apfa",    t->application.foreign_apps, t->application.num_foreign_apps));
  fields += T(APPLICATION,  map_kv_schema(&p, e, "apgs",    &t->application.global_schema));
  fields += T(APPLICATION,  map_kv_uint64(&p, e, "apid",    t->application.id));
  fields += T(APPLICATION,  map_kv_schema(&p, e, "apls",    &t->application.local_schema));
  fields += T(APPLICATION,  map_kv_bin   (&p, e, "apsu",    t->application.cprog, t->application.cprog_len));
  fields += T(ASSET_XFER,   map_kv_bin   (&p, e, "arcv",    t->asset_xfer.receiver, sizeof(t->asset_xfer.receiver)));
  fields += T(ASSET_XFER,   map_kv_bin   (&p, e, "asnd",    t->asset_xfer.sender, sizeof(t->asset_xfer.sender)));
  fields += T(ASSET_CONFIG, map_kv_uint64(&p, e, "caid",    t->asset_config.id));
  fields += T(PAYMENT,      map_kv_bin   (&p, e, "close",   t->payment.close, sizeof(t->payment.close)));
  fields += T(ASSET_FREEZE, map_kv_bin   (&p, e, "fadd",    t->asset_freeze.account, sizeof(t->asset_freeze.account)));
  fields += T(ASSET_FREEZE, map_kv_uint64(&p, e, "faid",    t->asset_freeze.id));
  fields +=                 map_kv_uint64(&p, e, "fee",     t->fee);
  fields +=                 map_kv_uint64(&p, e, "fv",      t->firstValid);
  fields +=                 map_kv_str   (&p, e, "gen",     t->genesisID, sizeof(t->genesisID));
  fields +=                 map_kv_bin   (&p, e, "gh",      t->genesisHash, sizeof(t->genesisHash));
  fields +=                 map_kv_bin   (&p, e, "grp",     t->groupID, sizeof(t->groupID));
  fields +=                 map_kv_uint64(&p, e, "lv",      t->lastValid);
  fields += T(KEYREG,       map_kv_bool  (&p, e, "nonpart", t->keyreg.nonpartFlag));
  fields +=                 map_kv_bin   (&p, e, "note",    t->note, t->note_len);
  fields += T(PAYMENT,      map_kv_bin   (&p, e, "rcv",     t->payment.receiver, sizeof(t->payment.receiver)));
  fields +=                 map_kv_bin   (&p, e, "rekey",   t->rekey, sizeof(t->rekey));
  fields += T(KEYREG,       map_kv_bin   (&p, e, "selkey",  t->keyreg.vrfpk, sizeof(t->keyreg.vrfpk)));
  fields +=                 map_kv_bin   (&p, e, "snd",     t->sender, sizeof(t->sender));
  fields += T(KEYREG,       map_kv_bin   (&p, e, "sprfkey", t->keyreg.sprfkey, sizeof(t->keyreg.sprfkey)));
  fields +=                 map_kv_str   (&p, e, "type",    typestr, SIZE_MAX);
  fields += T(KEYREG,       map_kv_uint64(&p, e, "votefst", t->keyreg.voteFirst));
  fields += T(KEYREG,       map_kv_uint64(&p, e, "votekd",  t->keyreg.keyDilution));
  fields += T(KEYREG,       map_kv_bin   (&p, e, "votekey", t->keyreg.votepk, sizeof(t->keyreg.votepk)));
  fields += T(KEYREG,       map_kv_uint64(&p, e, "votelst", t->keyreg.voteLast));
  fields += T(ASSET_XFER,   map_kv_uint64(&p, e, "xaid",    t->asset_xfer.id));
#undef T

  // If there are more fields than we can fit in the one-byte map encoding,
  // use the three-byte map encoding
  if (fields > FIXMAP_15 - FIXMAP_0) {
    buf[0] = MAP16;
    buf[1] = 0;
    buf[2] = fields;
    return p - buf;
  }

  // Otherwise, we fit in a FIXMAP
  buf[0] = FIXMAP_0 + fields;

  // Shift map contents back by two bytes, leaving first header byte in place
  // p - buf - 3 == bytes written - header length
  memmove(buf + 1, buf + 3, p - buf - 3);
  return p - buf - 2;
}
