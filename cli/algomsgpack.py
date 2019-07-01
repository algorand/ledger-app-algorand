## Canonical msgpack encoding used by Algorand

import struct

FIXINT_0    = 0x00
FIXINT_127  = 0x7f
FIXMAP_0    = 0x80
FIXMAP_15   = 0x8f
FIXARR_0    = 0x90
FIXARR_15   = 0x9f
FIXSTR_0    = 0xa0
FIXSTR_31   = 0xbf
BIN8        = 0xc4
UINT8       = 0xcc
UINT16      = 0xcd
UINT32      = 0xce
UINT64      = 0xcf
STR8        = 0xd9
ARR16       = 0xdc
ARR32       = 0xdd

def encode_str(buf, s):
  l = len(s)
  if l <= FIXSTR_31 - FIXSTR_0:
    buf.append(chr(FIXSTR_0 + l))
    buf.extend(s)
    return

  if l < (1 << 8):
    buf.append(chr(STR8))
    buf.append(chr(l))
    buf.extend(s)
    return

  raise Exception("String %s too long" % s)

def encode_uint(buf, i):
  if i <= FIXINT_127 - FIXINT_0:
    buf.append(chr(FIXINT_0 + i))
    return

  if i < (1 << 8):
    buf.append(chr(UINT8))
    buf.extend(struct.pack(">B", i))
    return

  if i < (1 << 16):
    buf.append(chr(UINT16))
    buf.extend(struct.pack(">H", i))
    return

  if i < (1 << 32):
    buf.append(chr(UINT32))
    buf.extend(struct.pack(">L", i))
    return

  if i < (1 << 64):
    buf.append(chr(UINT64))
    buf.extend(struct.pack(">Q", i))
    return

  raise Exception("Integer %d too big" % i)

def encode_bin(buf, b):
  l = len(b)
  if l < (1 << 8):
    buf.append(chr(BIN8))
    buf.append(chr(l))
    buf.extend(b)
    return

  raise Exception("Binary %s too long" % b)

def is_zero(v):
  if type(v) == int:
    return v == 0

  if type(v) == str or type(v) == unicode:
    return v == ""

  if type(v) == dict:
    return all([is_zero(vv) for k, vv in v.items()])

  if type(v) == list:
    return len(v) == 0

  raise Exception("is_zero: unknown type %s for %s" % (type(v), v))

def encode(buf, x):
  if type(x) == int and x >= 0:
    encode_uint(buf, x)
  elif type(x) == str:
    encode_bin(buf, x)
  elif type(x) == unicode:
    encode_str(buf, x.encode('ascii'))
  elif type(x) == dict:
    tmpbuf = []
    count = 0
    for k, v in sorted(x.items()):
      if is_zero(v):
        continue
      count += 1
      encode(tmpbuf, k)
      encode(tmpbuf, v)
    if count <= FIXMAP_15 - FIXMAP_0:
      buf.append(chr(FIXMAP_0 + count))
      buf.extend(tmpbuf)
    else:
      raise Exception("Too many map entries (%d) in %s" % (count, x))
  elif type(x) == list:
    count = len(x)
    if count <= FIXARR_15 - FIXARR_0:
      buf.append(chr(FIXARR_0 + count))
    elif count < 2**16:
      buf.append(chr(ARR16))
      buf.extend(struct.pack(">H", count))
    elif count < 2**32:
      buf.append(chr(ARR32))
      buf.extend(struct.pack(">L", count))
    else:
      raise Exception("Too many list entries (%d) in %s" % (count, x))
    for v in x:
      encode(buf, v)
  else:
    raise Exception("encode: unknown type %s" % type(x))

def encoded(x):
  buf = []
  encode(buf, x)
  return ''.join(buf)
