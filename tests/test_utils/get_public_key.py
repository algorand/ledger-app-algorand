
from ledgerblue.comm import getDongle
import struct


if __name__ == '__main__':
    dongle = getDongle(True)
    apdu = struct.pack('>BBBBBI', 0x80, 0x3, 0x80, 0x0, 0x0, 0x0)
    dongle.exchange(apdu)

