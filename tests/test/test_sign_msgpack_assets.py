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


def get_default_asset_config_tnx():
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
        url="https://path/to/my/asset/details", 
        decimals=0
    )

@pytest.fixture
def config_asset_txn():
    return base64.b64decode(algosdk.encoding.msgpack_encode(get_default_asset_config_tnx()))


def get_expected_messages_for_asset_config(current_tnx):
    messages =  [['review', 'transaction'], 
                 ['txn type','asset config'],
                 ['sender', current_tnx.sender.lower()], 
                 ['fee (alg)', str(current_tnx.fee*0.000001)],
                 ['genesis hash', current_tnx.genesis_hash.lower()],
                 ['asset id', str(current_tnx.index)],
                 ['total units', str(current_tnx.total)],
                 ['unit name', current_tnx.unit_name.lower()],
                 ['asset name', current_tnx.asset_name.lower()],
                 ['url', current_tnx.url.lower()],
                 ['manager', current_tnx.manager.lower()], 
                 ['reserve', current_tnx.reserve.lower()],
                 ['freezer', current_tnx.freeze.lower()],
                 ['clawback', current_tnx.clawback.lower()], 
                 ['sign', 'transaction']]

    return messages


def get_default_asset_xfer_tnx():
    return  algosdk.transaction.AssetTransferTxn(
        index=343434,
        first=100000,
        last=20000,
        gh="SGO1GKSzyE7IEPItTxCByw9x8FmnrCDexi9/cOUJOiI=",
        sender="YK54TGVZ37C7P76GKLXTY2LAH2522VD3U2434HRKE7NMXA65VHJVLFVOE4",
        receiver="NWBZBIROXZQEETCDKX6IZVVBV4EY637KCIX56LE5EHIQERCTSDYGXWG6PU",
        amt=1,
        fee=10000       
    )

@pytest.fixture
def xfer_asset_txn():
    return base64.b64decode(algosdk.encoding.msgpack_encode(get_default_asset_xfer_tnx()))


def get_expected_messages_for_asset_xfer(current_tnx):
    messages =  [['review', 'transaction'],
                ['txn type', 'asset xfer'],
                ['sender', current_tnx.sender.lower()],
                ['fee (alg)', str(current_tnx.fee*0.000001)],
                ['genesis hash', current_tnx.genesis_hash.lower()], 
                ['asset id', f'#{current_tnx.index}'],
                ['amount (base unit)', str(current_tnx.amount)], 
                ['asset dst', current_tnx.receiver.lower()],
                ['sign', 'transaction']]

    return messages

def get_default_asset_freeze_tnx():
    return  algosdk.transaction.AssetFreezeTxn(
        index=343434,
        first=100000,
        last=20000,
        sender="YK54TGVZ37C7P76GKLXTY2LAH2522VD3U2434HRKE7NMXA65VHJVLFVOE4",
        gh="SGO1GKSzyE7IEPItTxCByw9x8FmnrCDexi9/cOUJOiI=",
        target="NWBZBIROXZQEETCDKX6IZVVBV4EY637KCIX56LE5EHIQERCTSDYGXWG6PU",
        new_freeze_state = True,
        fee=10  
    )

@pytest.fixture
def freeze_asset_txn():
    return base64.b64decode(algosdk.encoding.msgpack_encode(get_default_asset_freeze_tnx()))


def get_expected_messages_for_asset_freeze(current_tnx):
    if current_tnx.new_freeze_state:
        state = 'frozen'
    else:
        state = 'unfrozen'
    messages =  [['review', 'transaction'],
                ['txn type', 'asset freeze'],
                ['sender', current_tnx.sender.lower()],
                ['fee (alg)', str(current_tnx.fee*0.000001)],
                ['genesis hash', current_tnx.genesis_hash.lower()], 
                ['asset id', f'{current_tnx.index}'],
                ['asset account', current_tnx.target.lower()],
                ['freeze flag', state],
                ['sign', 'transaction']]
    return messages



txn_labels = {
    'review', 'txn type', 'sender', 'fee', 'first valid', 'last valid',
    'genesis', 'asset id', 'create' , 'default frozen', 'unit name', 'total units',
    'decimals', 'asset name', 'url', 'metadata hash', 'manager',
    'reserve', 'freezer', 'clawback', 'amount (base unit)', 'asset dst','freeze flag', 'asset account','transaction'
} 

conf_label = "transaction"
    


def test_sign_msgpack_asset_freeze_validate_display(dongle, freeze_asset_txn):
    """
    """

    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(freeze_asset_txn)
        _ = txn_utils.sign_algo_txn(dongle, freeze_asset_txn)
        messages = dongle.get_messages()
    logging.info(messages)
    logging.info(get_expected_messages_for_asset_freeze(get_default_asset_freeze_tnx()))
    assert get_expected_messages_for_asset_freeze(get_default_asset_freeze_tnx()) == messages



def test_sign_msgpack_asset_xfer_validate_display(dongle, xfer_asset_txn):
    """
    """

    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(xfer_asset_txn)
        _ = txn_utils.sign_algo_txn(dongle, xfer_asset_txn)
        messages = dongle.get_messages()
    logging.info(messages)
    logging.info(get_expected_messages_for_asset_xfer(get_default_asset_xfer_tnx()))
    assert get_expected_messages_for_asset_xfer(get_default_asset_xfer_tnx()) == messages


def test_sign_msgpack_asset_config_validate_display(dongle, config_asset_txn):
    """
    """

    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(config_asset_txn)
        _ = txn_utils.sign_algo_txn(dongle, config_asset_txn)
        messages = dongle.get_messages()
    logging.info(messages)
    logging.info(get_expected_messages_for_asset_config(get_default_asset_config_tnx()))
    assert get_expected_messages_for_asset_config(get_default_asset_config_tnx()) == messages

    
    
def test_sign_msgpack_with_default_account(dongle, config_asset_txn):
    """
    """
    apdu = struct.pack('>BBBBB', 0x80, 0x3, 0x0, 0x0, 0x0)
    pubKey = dongle.exchange(apdu)

    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(config_asset_txn)
        txnSig = txn_utils.sign_algo_txn(dongle, config_asset_txn)

    assert len(txnSig) == 64
    verify_key = nacl.signing.VerifyKey(pubKey)
    verify_key.verify(smessage=b'TX' + config_asset_txn, signature=txnSig)
