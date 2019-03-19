#!/usr/bin/env python
#*******************************************************************************
#*   Ledger Blue
#*   (c) 2016 Ledger
#*
#*  Licensed under the Apache License, Version 2.0 (the "License");
#*  you may not use this file except in compliance with the License.
#*  You may obtain a copy of the License at
#*
#*      http://www.apache.org/licenses/LICENSE-2.0
#*
#*  Unless required by applicable law or agreed to in writing, software
#*  distributed under the License is distributed on an "AS IS" BASIS,
#*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#*  See the License for the specific language governing permissions and
#*  limitations under the License.
#********************************************************************************
from ledgerblue.comm import getDongle
from ledgerblue.commException import CommException
import ed25519

textToSign = "abc"

dongle = getDongle(True)
publicKeyBytes = dongle.exchange(bytes("8004000000".decode('hex')))

publicKeyXY = publicKeyBytes[1:]
publicKeyX = publicKeyXY[0:32][::-1]
publicKeyY = publicKeyXY[32:][::-1]

## Compute the encoded point of the public key
publicKey = publicKeyY
if (publicKeyX[0] & 1) != 0:
  publicKey[31] |= 0x80

print "Public key:", str(publicKey).encode('hex')

try:
	offset = 0
	while offset <> len(textToSign):
		if (len(textToSign) - offset) > 255:
			chunk = textToSign[offset : offset + 255] 
		else:
			chunk = textToSign[offset:]
		if (offset + len(chunk)) == len(textToSign):
			p1 = 0x80
		else:
			p1 = 0x00
		apdu = bytes("8002".decode('hex')) + chr(p1) + chr(0x00) + chr(len(chunk)) + bytes(chunk)
		signature = dongle.exchange(apdu)
		offset += len(chunk)
	print "signature " + str(signature).encode('hex')

        ed25519.checkvalid(str(signature), textToSign, str(publicKey))
        print "Verified signature"
except CommException as comm:
	if comm.sw == 0x6985:
		print "Aborted by user"
	else:
		print "Invalid status " + comm.sw 

