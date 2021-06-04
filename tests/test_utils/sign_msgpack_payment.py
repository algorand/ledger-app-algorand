
from ledgerblue.comm import getDongle
import struct
import algosdk
import base64
import os
import sys
import inspect
currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
parentdir = os.path.dirname(currentdir)
sys.path.insert(0, parentdir)

from test import  txn_utils

def get_payment_txn():
    return  algosdk.transaction.PaymentTxn(
        sender="YK54TGVZ37C7P76GKLXTY2LAH2522VD3U2434HRKE7NMXA65VHJVLFVOE4",
        receiver="RNZZNMS5L35EF6IQHH24ISSYQIKTUTWKGCB4Q5PBYYSTVB5EYDQRVYWMLE",
        fee=30000,
        flat_fee=True,
        amt=1000000,
        first=5667360,
        last=5668360,
        note="Hello World".encode(),
        gen="testnet-v1.0",
        close_remainder_to="NWBZBIROXZQEETCDKX6IZVVBV4EY637KCIX56LE5EHIQERCTSDYGXWG6PU",
        gh="SGO1GKSzyE7IEPItTxCByw9x8FmnrCDexi9/cOUJOiI=",
    )

if __name__ == '__main__':
    dongle = getDongle(True)
    decoded_txn = base64.b64decode(algosdk.encoding.msgpack_encode(get_payment_txn()))
    txn_utils.sign_algo_txn(dongle, decoded_txn)
    

