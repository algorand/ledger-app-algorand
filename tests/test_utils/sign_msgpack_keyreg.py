
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
    sp_var = algosdk.future.transaction.SuggestedParams(2000, 6002000, 6002000, "SGO1GKSzyE7IEPItTxCByw9x8FmnrCDexi9/cOUJOiI=", "testnet-v1.0", True)

    b64votekey = "1V2BE2lbFvS937H7pJebN0zxkqe1Nrv+aVHDTPbYRlw="
    b64selkey = "87iBW46PP4BpTDz6+IEGvxY6JqEaOtV0g+VWcJqoqtc="
    stateproofkey = "f0CYOA4yXovNBFMFX+1I/tYVBaAl7VN6e0Ki5yZA3H6jGqsU/LYHNaBkMQ/rN4M4F3UmNcpaTmbVbq+GgDsrhQ=="

    txn = algosdk.future.transaction.KeyregTxn(
        sender="YTOO52XR6UWNM6OUUDOGWVTNJYBWR5NJ3VCJTZUSR42JERFJFAG3NFD47U",
        sp=sp_var,
        votekey=b64votekey,
        selkey=b64selkey,
        votefst= 6200000,
        votelst=9500000,
        votekd= 1730,
        sprfkey=stateproofkey,
    )
    return txn

if __name__ == '__main__':
    dongle = getDongle(True)
    decoded_txn = base64.b64decode(algosdk.encoding.msgpack_encode(get_payment_txn()))
    txn_utils.sign_algo_txn(dongle, decoded_txn)
    

