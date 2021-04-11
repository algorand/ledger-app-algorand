import pytest
import logging
import struct
import base64

import msgpack
import nacl.signing

import algosdk
from . import txn_utils
from . import ui_interaction

from . import speculos


def get_default_tnx():

    return  algosdk.transaction.AssetConfigTxn(
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
        url="https://path/to/my/asset/details", 
        decimals=0
    )

@pytest.fixture
def txn():
    return base64.b64decode(algosdk.encoding.msgpack_encode(get_default_tnx()))






def get_expected_messages(current_tnx):
    messages =  [['review', 'transaction'], 
                 ['txn type','asset config'],
                 ['sender', current_tnx.sender.lower()], 
                 ['fee (alg)', str(current_tnx.fee*0.000001)],
                 ['genesis hash', current_tnx.genesis_hash.lower()],
                 ['asset id', 'create'],
                 ['total units', '1000'],
                 ['default frozen', 'unfrozen'],
                 ['unit name', 'latinum'],
                 ['asset name', 'latinum'],
                 ['url', current_tnx.url.lower()],
                 ['manager', current_tnx.manager.lower()], 
                 ['reserve', current_tnx.reserve.lower()],
                 ['freezer', current_tnx.freeze.lower()],
                 ['clawback', current_tnx.clawback.lower()], 
                 ['sign', 'transaction']]

    return messages


txn_labels = {
    'review', 'txn type', 'sender', 'fee', 'first valid', 'last valid',
    'genesis', 'asset id', 'create' , 'default frozen', 'unit name', 'total units',
    'decimals', 'Asset name', 'url', 'metadata hash', 'manager',
    'reserve', 'freezer', 'clawback',  'transaction'
} 

conf_label = "transaction"



def test_sign_msgpack_asset_validate_display(dongle, txn):
    """
    """

    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(txn)
        _ = txn_utils.sign_algo_txn(dongle, txn)
        messages = dongle.get_messages()
    logging.info(messages)
    logging.info(get_expected_messages(get_default_tnx()))
    assert get_expected_messages(get_default_tnx()) == messages

    
    
def test_sign_msgpack_with_default_account(dongle, txn):
    """
    """
    apdu = struct.pack('>BBBBB', 0x80, 0x3, 0x0, 0x0, 0x0)
    pubKey = dongle.exchange(apdu)

    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(txn)
        txnSig = txn_utils.sign_algo_txn(dongle, txn)

    assert len(txnSig) == 64
    verify_key = nacl.signing.VerifyKey(pubKey)
    verify_key.verify(smessage=b'TX' + txn, signature=txnSig)
