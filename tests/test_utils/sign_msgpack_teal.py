
from ledgerblue.comm import getDongle
import struct
import algosdk
from algosdk.future import transaction
import base64
import os
import sys
import inspect
currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
parentdir = os.path.dirname(currentdir)
sys.path.insert(0, parentdir)

from test import  txn_utils


def get_app_create_txn():
    approve_app = b'\x02 \x05\x00\x05\x04\x02\x01&\x07\x04vote\tVoteBegin\x07VoteEnd\x05voted\x08RegBegin\x06RegEnd\x07Creator1\x18"\x12@\x00\x951\x19\x12@\x00\x871\x19$\x12@\x00y1\x19%\x12@\x00R1\x19!\x04\x12@\x00<6\x1a\x00(\x12@\x00\x01\x002\x06)d\x0f2\x06*d\x0e\x10@\x00\x01\x00"2\x08+c5\x005\x014\x00A\x00\x02"C6\x1a\x016\x1a\x01d!\x04\x08g"+6\x1a\x01f!\x04C2\x06\'\x04d\x0f2\x06\'\x05d\x0e\x10C"2\x08+c5\x005\x012\x06*d\x0e4\x00\x10A\x00\t4\x014\x01d!\x04\tg!\x04C1\x00\'\x06d\x12C1\x00\'\x06d\x12C\'\x061\x00g1\x1b$\x12@\x00\x01\x00\'\x046\x1a\x00\x17g\'\x056\x1a\x01\x17g)6\x1a\x02\x17g*6\x1a\x03\x17g!\x04C'
    clear_pgm = b'\x02 \x01\x01'

    # the approve_app is a compiled Tealscript taken from https://pyteal.readthedocs.io/en/stable/examples.html#periodic-payment
    # we truncated the pgm because of the memory limit on the Ledeger 
    approve_app = approve_app[:128]
    local_ints = 2
    local_bytes = 5
    global_ints = 24 
    global_bytes = 1
    global_schema = transaction.StateSchema(global_ints, global_bytes)
    local_schema = transaction.StateSchema(local_ints, local_bytes)
    local_sp = transaction.SuggestedParams(fee= 2100, first=6002000, last=6003000,
                                    gen="testnet-v1.0",
                                    gh="SGO1GKSzyE7IEPItTxCByw9x8FmnrCDexi9/cOUJOiI=",flat_fee=True)
    txn = algosdk.future.transaction.ApplicationCreateTxn(sender="YK54TGVZ37C7P76GKLXTY2LAH2522VD3U2434HRKE7NMXA65VHJVLFVOE4",
                                                         sp=local_sp, approval_program=approve_app, on_complete=transaction.OnComplete.NoOpOC.real,clear_program= clear_pgm, global_schema=global_schema, foreign_apps=[55], foreign_assets=[31566704], accounts=["7PKXMJB2577SQ6R6IGYRAZQ27TOOOTIGTOQGJB3L5SGZFBVVI4AHMKLCEI", "NWBZBIROXZQEETCDKX6IZVVBV4EY637KCIX56LE5EHIQERCTSDYGXWG6PU"],
                                                         app_args=[b'\x00\x00\0x00\x00',b'\x02\x00\0x00\x00'],
                                                         local_schema=local_schema )
    return txn

if __name__ == '__main__':
    dongle = getDongle(True)
    decoded_txn = base64.b64decode(algosdk.encoding.msgpack_encode(get_app_create_txn()))
    txn_utils.sign_algo_txn(dongle, decoded_txn)
    

