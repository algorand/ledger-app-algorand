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

dongle = getDongle(debug=False)

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
  # CLA
  apdu = "\x80"

  # INS
  if intx['type'] == 'pay':
    apdu += "\x06"
  elif intx['type'] == 'keyreg':
    apdu += "\x07"
  else:
    raise Exception("Unknown transaction type %s" % intx['type'])

  # P1, P2, LC (to be filled in later)
  apdu += "\x00\x00\x00"

  # Notes cannot be signed by Ledger app at the moment
  if 'note' in intx:
    raise Exception('Cannot sign transaction with note; pass --note "" to goal clerk send')

  # Common header fields
  apdu += struct.pack("32s", intx.get('snd', ""))
  apdu += struct.pack("<Q", intx.get('fee', 0))
  apdu += struct.pack("<Q", intx.get('fv', 0))
  apdu += struct.pack("<Q", intx.get('lv', 0))
  apdu += struct.pack("32s", intx.get('gen', u"").encode('ascii'))
  apdu += struct.pack("32s", intx.get('gh', ""))

  # Payment type
  if intx['type'] == 'pay':
    apdu += struct.pack("32s", intx.get('rcv', ""))
    apdu += struct.pack("<Q", intx.get('amt', 0))
    apdu += struct.pack("32s", intx.get('close', ""))

  # Keyreg type
  if intx['type'] == 'keyreg':
    apdu += struct.pack("32s", intx.get('votekey', ""))
    apdu += struct.pack("32s", intx.get('selkey', ""))

  # Fill in the length
  apdu = apdu[:4] + struct.pack("B", len(apdu) - 5) + apdu[5:]

  signature = dongle.exchange(apdu)
  print "signature " + str(signature).encode('hex')

  txbytes = algomsgpack.encoded(intx)
  ed25519.checkvalid(str(signature), 'TX' + txbytes, str(publicKey))
  print "Verified signature"

  foundMsig = False
  msig = instx.get('msig')
  if msig is not None:
    if msig.get('v') != 1:
      print "Unknown multisig version %d, not filling in multisig" % msig['v']
    for sub in msig['subsig']:
      if sub['pk'] == publicKey:
        sub['s'] = signature
        foundMsig = True
  if not foundMsig:
    instx['sig'] = signature

  with open(outfile, 'w') as f:
    f.write(algomsgpack.encoded(instx))
    print "Wrote signed transaction to %s" % outfile

except CommException as comm:
  if comm.sw == 0x6985:
    print "Aborted by user"
  else:
    print "Invalid status " + comm.sw
