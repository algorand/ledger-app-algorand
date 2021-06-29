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





@pytest.fixture
def config_asset_txn():
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

def expected_messages_for_asset_config(current_txn):
    return [['review', 'transaction'], 
                 ['txn type','asset config'],
                 ['sender', current_txn.sender.lower()], 
                 ['fee (alg)', str(current_txn.fee*0.000001)],
                 ['genesis hash', current_txn.genesis_hash.lower()],
                 ['asset id', str(current_txn.index)],
                 ['total units', str(current_txn.total)],
                 ['unit name', current_txn.unit_name.lower()],
                 ['asset name', current_txn.asset_name.lower()],
                 ['url', current_txn.url.lower()],
                 ['metadata hash', base64.b64encode(current_txn.metadata_hash).decode('ascii').lower()],
                 ['manager', current_txn.manager.lower()], 
                 ['reserve', current_txn.reserve.lower()],
                 ['freezer', current_txn.freeze.lower()],
                 ['clawback', current_txn.clawback.lower()], 
                 ['sign', 'transaction']]



@pytest.fixture
def xfer_asset_txn():
        return  algosdk.transaction.AssetTransferTxn(
        index=343434,
        first=100000,
        last=20000,
        gh="SGO1GKSzyE7IEPItTxCByw9x8FmnrCDexi9/cOUJOiI=",
        sender="YK54TGVZ37C7P76GKLXTY2LAH2522VD3U2434HRKE7NMXA65VHJVLFVOE4",
        receiver="NWBZBIROXZQEETCDKX6IZVVBV4EY637KCIX56LE5EHIQERCTSDYGXWG6PU",
        amt=4,
        fee=10000       
    )


def expected_messages_for_asset_xfer(current_txn):
    return  [['review', 'transaction'],
                ['txn type', 'asset xfer'],
                ['sender', current_txn.sender.lower()],
                ['fee (alg)', str(current_txn.fee*0.000001)],
                ['genesis hash', current_txn.genesis_hash.lower()], 
                ['asset id', f'#{current_txn.index}'],
                ['amount (base unit)', str(current_txn.amount)], 
                ['asset dst', current_txn.receiver.lower()],
                ['sign', 'transaction']]



@pytest.fixture
def freeze_asset_txn():
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

def expected_messages_for_asset_freeze(current_txn):
    if current_txn.new_freeze_state:
        state = 'frozen'
    else:
        state = 'unfrozen'
    messages =  [['review', 'transaction'],
                ['txn type', 'asset freeze'],
                ['sender', current_txn.sender.lower()],
                ['fee (alg)', str(current_txn.fee*0.000001)],
                ['genesis hash', current_txn.genesis_hash.lower()], 
                ['asset id', f'{current_txn.index}'],
                ['asset account', current_txn.target.lower()],
                ['freeze flag', state],
                ['sign', 'transaction']]
    return messages






txn_labels = {'review', 'txn type', 'sender',
             'fee (alg)', 'genesis hash','group id', 'asset id', 'amount', 'asset src', 'asset dst',  
             'asset close', 'total units', 'unit name', 'decimals', 'asset account', 'freeze flag', 
             'sign', 'asset name', 'default frozen', 'url','metadata hash', 'manager', 'reserve',
               'freezer', 'clawback', 'sign' 
} 

conf_label = "sign"
    


def test_sign_msgpack_asset_config_validate_display(dongle, config_asset_txn):
    """
    """
    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(config_asset_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        _ = txn_utils.sign_algo_txn(dongle, decoded_txn)
        messages = dongle.get_messages()
    logging.info(messages)
    logging.info(expected_messages_for_asset_config(config_asset_txn))
    assert expected_messages_for_asset_config(config_asset_txn) == messages
    

def test_sign_msgpack_asset_config_create_validate_display(dongle, config_asset_txn):
    """
    """
    config_asset_txn.index = 0

    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(config_asset_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        _ = txn_utils.sign_algo_txn(dongle, decoded_txn)
        messages = dongle.get_messages()
    logging.info(messages)
    expected_messages = expected_messages_for_asset_config(config_asset_txn)

    expected_messages[5] = ['asset id','create']
    expected_messages.insert(7,['default frozen', 'unfrozen'])
    logging.info(expected_messages)
    assert expected_messages == messages

def test_sign_msgpack_asset_config_create_frozen_validate_display(dongle, config_asset_txn):
    """
    """
    config_asset_txn.index = 0
    config_asset_txn.default_frozen = True

    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(config_asset_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        _ = txn_utils.sign_algo_txn(dongle, decoded_txn)
        messages = dongle.get_messages()
    logging.info(messages)
    expected_messages = expected_messages_for_asset_config(config_asset_txn)

    expected_messages[5] = ['asset id','create']
    expected_messages.insert(7,['default frozen', 'frozen'])
    logging.info(expected_messages)
    assert expected_messages == messages


def test_sign_msgpack_asset_config_create_with_decimals_validate_display(dongle, config_asset_txn):
    """
    """
    config_asset_txn.index = 0
    config_asset_txn.decimals = 100

    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(config_asset_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        _ = txn_utils.sign_algo_txn(dongle, decoded_txn)
        messages = dongle.get_messages()
    logging.info(messages)
    expected_messages = expected_messages_for_asset_config(config_asset_txn)

    expected_messages[5] = ['asset id','create']
    expected_messages.insert(7,['default frozen', 'unfrozen'])
    expected_messages.insert(9,['decimals', str(config_asset_txn.decimals)])
    logging.info(expected_messages)
    assert expected_messages == messages

def test_sign_msgpack_asset_freeze_validate_display(dongle, freeze_asset_txn):
    """
    """
    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(freeze_asset_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        _ = txn_utils.sign_algo_txn(dongle, decoded_txn)
        messages = dongle.get_messages()
    logging.info(messages)
    logging.info(expected_messages_for_asset_freeze(freeze_asset_txn))
    assert expected_messages_for_asset_freeze(freeze_asset_txn) == messages


def test_sign_msgpack_asset_xfer_validate_display(dongle, xfer_asset_txn):
    """
    """
    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(xfer_asset_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        _ = txn_utils.sign_algo_txn(dongle, decoded_txn)
        messages = dongle.get_messages()
    logging.info(messages)
    logging.info(expected_messages_for_asset_xfer(xfer_asset_txn))
    assert expected_messages_for_asset_xfer(xfer_asset_txn) == messages


def test_sign_msgpack_asset_xfer_verified_asa_validate_display(dongle, xfer_asset_txn):
    """
    """
    xfer_asset_txn.index = 31566704 #the USDC index
    decoded_txn = base64.b64decode(algosdk.encoding.msgpack_encode(xfer_asset_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        _ = txn_utils.sign_algo_txn(dongle, decoded_txn)
        messages = dongle.get_messages()
    logging.info(messages)
    
    expected_messages = expected_messages_for_asset_xfer(xfer_asset_txn)
    expected_messages[5] = ['asset id','usdc (#31566704)']
    expected_messages[6] = ['amount (usdc)', f'{xfer_asset_txn.amount*0.000001:.6f}']

    logging.info(expected_messages)
    assert expected_messages == messages

def test_sign_msgpack_asset_optin_validate_display(dongle, xfer_asset_txn):
    """
    """
    optin_asset_txn = xfer_asset_txn
    optin_asset_txn.amount = 0
    #optin_asset_txn.receiver = optin_asset_txn.sender
    optin_asset_txn.revocation_target = optin_asset_txn.receiver
    
    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(optin_asset_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        _ = txn_utils.sign_algo_txn(dongle, decoded_txn)
        messages = dongle.get_messages()
    expected_messages = expected_messages_for_asset_xfer(optin_asset_txn)
    expected_messages[1] =  ['txn type', 'opt-in']
    expected_messages[6] =  ['asset src', optin_asset_txn.receiver.lower()]
    del expected_messages[7]
    
    logging.info(messages)
    logging.info(expected_messages)
    assert expected_messages == messages


def test_sign_msgpack_clawback_validate_display(dongle, xfer_asset_txn):
    """
    """
    clawback_txn = xfer_asset_txn
    clawback_txn.revocation_target = "7PKXMJB2577SQ6R6IGYRAZQ27TOOOTIGTOQGJB3L5SGZFBVVI4AHMKLCEI"
    
    
    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(clawback_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        _ = txn_utils.sign_algo_txn(dongle, decoded_txn)
        messages = dongle.get_messages()
    expected_messages = expected_messages_for_asset_xfer(clawback_txn)
    expected_messages.insert(7,['asset src',clawback_txn.revocation_target.lower()])
    logging.info(messages)
    logging.info(expected_messages)
    assert expected_messages == messages

def test_sign_msgpack_closeto_validate_display(dongle, xfer_asset_txn):
    """
    """
    close_txn = xfer_asset_txn
    close_txn.close_assets_to = "7PKXMJB2577SQ6R6IGYRAZQ27TOOOTIGTOQGJB3L5SGZFBVVI4AHMKLCEI"
    
    
    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(close_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        _ = txn_utils.sign_algo_txn(dongle, decoded_txn)
        messages = dongle.get_messages()
    expected_messages = expected_messages_for_asset_xfer(close_txn)
    expected_messages.insert(8,['asset close',close_txn.close_assets_to.lower()])
    logging.info(messages)
    logging.info(expected_messages)
    assert expected_messages == messages



    
def test_sign_msgpack_with_default_account(dongle, config_asset_txn):
    """
    """
    apdu = struct.pack('>BBBBB', 0x80, 0x3, 0x0, 0x0, 0x0)
    pubKey = dongle.exchange(apdu)

    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(config_asset_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        txnSig = txn_utils.sign_algo_txn(dongle, decoded_txn)

    assert len(txnSig) == 64
    verify_key = nacl.signing.VerifyKey(pubKey)
    verify_key.verify(smessage=b'TX' + decoded_txn, signature=txnSig)


def test_sign_msgpack_asset_xfer_group_validate_display(dongle, xfer_asset_txn, config_asset_txn, freeze_asset_txn ):
    """
    """
    apdu = struct.pack('>BBBBB', 0x80, 0x3, 0x0, 0x0, 0x0)
    pubKey = dongle.exchange(apdu)


    gid = algosdk.transaction.calculate_group_id([xfer_asset_txn, config_asset_txn, freeze_asset_txn])
    xfer_asset_txn.group = gid
    config_asset_txn.group = gid
    freeze_asset_txn.group = gid

    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(xfer_asset_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        txnSig = txn_utils.sign_algo_txn(dongle, decoded_txn)
        messages = dongle.get_messages()
    logging.info(messages)
    exp_messages = expected_messages_for_asset_xfer(xfer_asset_txn)
    exp_messages.insert(5,['group id', base64.b64encode(xfer_asset_txn.group).decode('ascii').lower()])
    logging.info(exp_messages)

    assert exp_messages == messages

    verify_key = nacl.signing.VerifyKey(pubKey)
    verify_key.verify(smessage=b'TX' + decoded_txn, signature=txnSig)


    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(config_asset_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        txnSig = txn_utils.sign_algo_txn(dongle, decoded_txn)
        messages = dongle.get_messages()
    logging.info(messages)
    exp_messages = expected_messages_for_asset_config(config_asset_txn)
    exp_messages.insert(5,['group id', base64.b64encode(config_asset_txn.group).decode('ascii').lower()])
    logging.info(exp_messages)

    assert exp_messages == messages

    verify_key = nacl.signing.VerifyKey(pubKey)
    verify_key.verify(smessage=b'TX' + decoded_txn, signature=txnSig)


    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(freeze_asset_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        txnSig = txn_utils.sign_algo_txn(dongle, decoded_txn)
        messages = dongle.get_messages()
    logging.info(messages)
    exp_messages = expected_messages_for_asset_freeze(freeze_asset_txn)
    exp_messages.insert(5,['group id', base64.b64encode(freeze_asset_txn.group).decode('ascii').lower()])
    logging.info(exp_messages)

    assert exp_messages == messages

    verify_key = nacl.signing.VerifyKey(pubKey)
    verify_key.verify(smessage=b'TX' + decoded_txn, signature=txnSig)