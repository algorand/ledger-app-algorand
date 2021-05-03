
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
def get_config_asset_txn():
    return  algosdk.transaction.AssetConfigTxn(
        index=343434,
        first=100000,
        last=20000,
        gh="SGO1GKSzyE7IEPItTxCByw9x8FmnrCDexi9/cOUJOiI=",
        sender="YK54TGVZ37C7P76GKLXTY2LAH2522VD3U2434HRKE7NMXA65VHJVLFVOE4",
        fee=10000,
        flat_fee=True,
        total=1000,
        default_frozen=False,
        unit_name="LATINUM",
        asset_name="latinum",
        manager="YTOO52XR6UWNM6OUUDOGWVTNJYBWR5NJ3VCJTZUSR42JERFJFAG3NFD47U",
        reserve="R4DCCBODM4L7C6CKVOV5NYDPEYS2G5L7KC7LUYPLUCKBCOIZMYJPFUDTKE",
        freeze="NWBZBIROXZQEETCDKX6IZVVBV4EY637KCIX56LE5EHIQERCTSDYGXWG6PU",
        clawback="7PKXMJB2577SQ6R6IGYRAZQ27TOOOTIGTOQGJB3L5SGZFBVVI4AHMKLCEI",
        url="https://path/to/my/asset/detail", 
        metadata_hash=bytes.fromhex("0e07cf830957701d43c183f1515f63e6b68027e528f43ef52b1527a520ddec82"),
        decimals=0
    )

if __name__ == '__main__':
    dongle = getDongle(True)
    decoded_txn = base64.b64decode(algosdk.encoding.msgpack_encode(get_config_asset_txn()))
    txn_utils.sign_algo_txn(dongle, decoded_txn)
    

