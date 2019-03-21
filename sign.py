#!/usr/bin/env python

from ledgerblue.comm import getDongle
from ledgerblue.commException import CommException
import ed25519
import msgpack
import base64
import sha512_256
import sys
import os
import struct
import algomsgpack

def checksummed(pk):
  sum = sha512_256.new(str(pk)).digest()
  return base64.b32encode(pk + sum[28:32]).replace("=", "")

dongle = getDongle(True)

publicKey = dongle.exchange(bytes("8003000000".decode('hex')))
print "Ledger app address:", checksummed(publicKey)

if len(sys.argv) != 3:
  print "Usage: %s infile outfile" % sys.argv[0]
  sys.exit(0)

(_, infile, outfile) = sys.argv

with open(infile) as f:
  buf = f.read()
  instx = msgpack.unpackb(buf, raw=False)
  intx = instx['txn']

try:
  apdu = "\x80"

  if intx['type'] == 'pay':
    apdu += "\x01"
  elif intx['type'] == 'keyreg':
    apdu += "\x02"
  else:
    raise Exception("Unknown transaction type %s" % intx['type'])

  # Common header fields
  apdu += struct.pack("32s", intx.get('snd', ""))
  apdu += struct.pack("<Q", intx.get('fee', 0))
  apdu += struct.pack("<Q", intx.get('fv', 0))
  apdu += struct.pack("<Q", intx.get('lv', 0))
  apdu += struct.pack("32s", intx.get('gen', u"").encode('ascii'))

  # Payment type
  if intx['type'] == 'pay':
    apdu += struct.pack("32s", intx.get('rcv', ""))
    apdu += struct.pack("<Q", intx.get('amt', 0))
    apdu += struct.pack("32s", intx.get('close', ""))

  # Keyreg type
  if intx['type'] == 'keyreg':
    apdu += struct.pack("32s", intx.get('votekey', ""))
    apdu += struct.pack("32s", intx.get('selkey', ""))

  signature = dongle.exchange(apdu)
  print "signature " + str(signature).encode('hex')

  txbytes = algomsgpack.encoded(intx)
  ed25519.checkvalid(str(signature), 'TX' + txbytes, str(publicKey))
  print "Verified signature"
except CommException as comm:
  if comm.sw == 0x6985:
    print "Aborted by user"
  else:
    print "Invalid status " + comm.sw
