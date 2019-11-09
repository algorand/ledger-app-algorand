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
  txbytes = algomsgpack.encoded(intx)

  tosend = txbytes

  p1 = 0
  p2 = 0x80
  while p2 == 0x80:
    thischunk = tosend[:250]
    if len(thischunk) == len(tosend):
      p2 = 0

    # CLA
    apdu = "\x80"

    # INS_SIGN_MSGPACK
    apdu += "\x08"

    # P1, P2, LC
    apdu += struct.pack("B", p1)
    apdu += struct.pack("B", p2)
    apdu += struct.pack("B", len(thischunk))

    apdu += thischunk

    signature = dongle.exchange(apdu)

    tosend = tosend[len(thischunk):]
    p1 = 0x80

  if len(signature) > 64:
    raise Exception("Error: %s" % signature[65:])

  print "signature " + str(signature).encode('hex')

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
    print comm
