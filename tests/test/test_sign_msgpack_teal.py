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
from algosdk.future import transaction
from Cryptodome.Hash import SHA256



OPERATION_UI_TEXT = ['NoOp', 'OptIn', 'CloseOut', 'ClearState', 'UpdateApp', 'DeleteApp']
UNKOWN_UI_TEXT = 'Unknown'


@pytest.fixture
def app_create_txn():
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



@pytest.fixture
def app_call_txn():
    local_sp = transaction.SuggestedParams(fee= 2100, first=6002000, last=6003000,
                                    gen="testnet-v1.0",
                                    gh="SGO1GKSzyE7IEPItTxCByw9x8FmnrCDexi9/cOUJOiI=",flat_fee=True)
    txn = algosdk.future.transaction.ApplicationCallTxn(sender="YK54TGVZ37C7P76GKLXTY2LAH2522VD3U2434HRKE7NMXA65VHJVLFVOE4", sp=local_sp, index=68,
                                                        foreign_apps=[55], foreign_assets=[31566704], accounts=["7PKXMJB2577SQ6R6IGYRAZQ27TOOOTIGTOQGJB3L5SGZFBVVI4AHMKLCEI","NWBZBIROXZQEETCDKX6IZVVBV4EY637KCIX56LE5EHIQERCTSDYGXWG6PU"],
                                                        app_args=[b'\x00\x00\0x00\x00',b'\x02\x00\0x00\x00'],
                                                        on_complete=transaction.OnComplete.NoOpOC.real )
    return txn


def hash_bytes(bytes_array):
    h = SHA256.new()
    h.update(bytes_array)
    return base64.b64encode(h.digest()).decode('ascii')


def get_expected_messages_for_call_pgm(current_txn):
    if current_txn.on_complete < len(OPERATION_UI_TEXT):
        operation_text = OPERATION_UI_TEXT[current_txn.on_complete]
    else:
        operation_text = UNKOWN_UI_TEXT
    
    
    messages =  [['review', 'transaction'],
                 ['txn type', 'application'], 
                 ['sender', current_txn.sender.lower()],
                 ['fee (alg)', str(current_txn.fee*0.000001)],
                 ['genesis id', current_txn.genesis_id.lower()], 
                 ['genesis hash', current_txn.genesis_hash.lower()],
                 ['app id', str(current_txn.index)],
                 ['on completion', operation_text.lower()],
                 ['foreign app 0', str(current_txn.foreign_apps[0])],
                 ['foreign asset 0', str(current_txn.foreign_assets[0])],
                 ['app account 0', str(current_txn.accounts[0]).lower()],
                 ['app account 1', str(current_txn.accounts[1]).lower()],
                 ['app arg 0 (sha256)', hash_bytes(current_txn.app_args[0]).lower()],
                 ['app arg 1 (sha256)', hash_bytes(current_txn.app_args[1]).lower()],
                 ['sign', 'transaction']]
                 
    return messages

def get_expected_messages_for_create_pgm(current_txn):
    messages = get_expected_messages_for_call_pgm(current_txn)
    messages = messages[:len(messages)-1]
    

    create_messages =  [['global schema', f'uint: {current_txn.global_schema.num_uints}, byte: {current_txn.global_schema.num_byte_slices}'],
                 ['local schema', f'uint: {current_txn.local_schema.num_uints}, byte: {current_txn.local_schema.num_byte_slices}'],
                 ['apprv (sha256)', hash_bytes(current_txn.approval_program).lower()],
                 ['clear (sha256)', hash_bytes(current_txn.clear_program).lower()],
                 ['sign', 'transaction']]
                 
    return messages + create_messages



txn_labels = {
    'review', 'txn type','sender','fee (alg)', 'genesis id', 'genesis hash', 'group id', 'app id', 'on completion', 
    'foreign app 0', 'foreign asset 0','app account 0', 'app account 1', 'app arg 0 (sha256)', 'app arg 1 (sha256)',
    'global schema', 'local schema',  'apprv (sha256)', 'clear (sha256)','sign'
} 

conf_label = "sign"



def test_sign_msgpack_call_app_validate_display(dongle, app_call_txn):
    """
    """
    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(app_call_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        _ = txn_utils.sign_algo_txn(dongle, decoded_txn)
        messages = dongle.get_messages()
    logging.info(messages)
    logging.info(get_expected_messages_for_call_pgm(app_call_txn))
    assert get_expected_messages_for_call_pgm(app_call_txn) == messages


def test_sign_msgpack_call_app_long_approval_app(dongle, app_create_txn):
    """
    """
    app_create_txn.approval_program += b'0'
    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(app_create_txn))
    with pytest.raises(speculos.CommException) as excinfo:
        dongle.exchange(txn_utils.sign_algo_txn(dongle, decoded_txn))
        
    assert excinfo.value.sw == 0x6e00


def test_sign_msgpack_call_app_long_clear_app(dongle, app_create_txn):
    """
    """
    app_create_txn.clear_program = app_create_txn.clear_program + (32 -len(app_create_txn.clear_program)+1)*b'0'
    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(app_create_txn))
    with pytest.raises(speculos.CommException) as excinfo:
        dongle.exchange(txn_utils.sign_algo_txn(dongle, decoded_txn))
        
    assert excinfo.value.sw == 0x6e00

def test_sign_msgpack_call_app_more_than_one_foreign_app(dongle, app_call_txn):
    """
    """
    app_call_txn.foreign_apps.append(81)
    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(app_call_txn))
    with pytest.raises(speculos.CommException) as excinfo:
        dongle.exchange(txn_utils.sign_algo_txn(dongle, decoded_txn))
        
    assert excinfo.value.sw == 0x6e00


def test_sign_msgpack_call_app_more_than_one_foreign_asset(dongle, app_call_txn):
    """
    """
    app_call_txn.foreign_assets.append(6547014)
    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(app_call_txn))
    with pytest.raises(speculos.CommException) as excinfo:
        dongle.exchange(txn_utils.sign_algo_txn(dongle, decoded_txn))
        
    assert excinfo.value.sw == 0x6e00

def test_sign_msgpack_call_app_more_than_two_accounts(dongle, app_call_txn):
    """
    """
    app_call_txn.accounts.append("R4DCCBODM4L7C6CKVOV5NYDPEYS2G5L7KC7LUYPLUCKBCOIZMYJPFUDTKE")
    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(app_call_txn))
    with pytest.raises(speculos.CommException) as excinfo:
        dongle.exchange(txn_utils.sign_algo_txn(dongle, decoded_txn))
        
    assert excinfo.value.sw == 0x6e00

def test_sign_msgpack_call_app_more_than_two_args(dongle, app_call_txn):
    """
    """
    app_call_txn.app_args.append(b'\0x02\0x00\0x00\0x00')
    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(app_call_txn))
    with pytest.raises(speculos.CommException) as excinfo:
        dongle.exchange(txn_utils.sign_algo_txn(dongle, decoded_txn))
        
    assert excinfo.value.sw == 0x6e00


def test_sign_msgpack_create_app_validate_display(dongle, app_create_txn):
    """
    """
    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(app_create_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        _ = txn_utils.sign_algo_txn(dongle, decoded_txn)
        messages = dongle.get_messages()
    logging.info(messages)
    logging.info(get_expected_messages_for_create_pgm(app_create_txn))
    assert get_expected_messages_for_create_pgm(app_create_txn) == messages


@pytest.mark.parametrize('on_compelete_type', [transaction.OnComplete.NoOpOC.real, transaction.OnComplete.OptInOC.real,
                                                transaction.OnComplete.CloseOutOC.real, transaction.OnComplete.ClearStateOC.real, 
                                                transaction.OnComplete.UpdateApplicationOC.real, transaction.OnComplete.DeleteApplicationOC.real,6])
def test_sign_msgpack_call_app_differnet_on_complete_validate_display(dongle, app_call_txn, app_create_txn, on_compelete_type):
    """
    """
    app_call_txn.on_complete = on_compelete_type
    if app_call_txn.on_complete == transaction.OnComplete.UpdateApplicationOC.real :
        app_call_txn.approval_program = app_create_txn.approval_program
        app_call_txn.clear_program = app_create_txn.clear_program

    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(app_call_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        _ = txn_utils.sign_algo_txn(dongle, decoded_txn)
        messages = dongle.get_messages()

    expected_messages = get_expected_messages_for_call_pgm(app_call_txn)
    if app_call_txn.on_complete == transaction.OnComplete.UpdateApplicationOC.real :
        pgm_messages = [['apprv (sha256)', hash_bytes(app_call_txn.approval_program).lower()],
                        ['clear (sha256)', hash_bytes(app_call_txn.clear_program).lower()],
                        ['sign', 'transaction']]
        expected_messages = expected_messages[:len(expected_messages)-1] + pgm_messages
       
    logging.info(messages)
    logging.info(expected_messages)
    assert expected_messages == messages


def test_sign_msgpack_with_default_account(dongle, app_create_txn):
    """
    """
    apdu = struct.pack('>BBBBB', 0x80, 0x3, 0x0, 0x0, 0x0)
    pubKey = dongle.exchange(apdu)
    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(app_create_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        txnSig = txn_utils.sign_algo_txn(dongle, decoded_txn)

    assert len(txnSig) == 64
    verify_key = nacl.signing.VerifyKey(pubKey)
    verify_key.verify(smessage=b'TX' + decoded_txn, signature=txnSig)



def test_sign_msgpack_group_id_validate_display(dongle, app_call_txn, app_create_txn):
    """
    """

    apdu = struct.pack('>BBBBB', 0x80, 0x3, 0x0, 0x0, 0x0)
    pubKey = dongle.exchange(apdu)

    gid = algosdk.transaction.calculate_group_id([app_call_txn, app_create_txn])
    app_call_txn.group = gid
    app_create_txn.group = gid


    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(app_call_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        txnSig = txn_utils.sign_algo_txn(dongle, decoded_txn)
        messages = dongle.get_messages()
        logging.info(messages)

    exp_messages = get_expected_messages_for_call_pgm(app_call_txn)
    exp_messages.insert(6,['group id', base64.b64encode(app_call_txn.group).decode('ascii').lower()])
    logging.info(exp_messages)

    assert exp_messages == messages

    verify_key = nacl.signing.VerifyKey(pubKey)
    verify_key.verify(smessage=b'TX' + decoded_txn, signature=txnSig)


    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(app_create_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        txnSig = txn_utils.sign_algo_txn(dongle, decoded_txn)
        messages = dongle.get_messages()
        logging.info(messages)

    exp_messages = get_expected_messages_for_create_pgm(app_create_txn)
    exp_messages.insert(6,['group id', base64.b64encode(app_create_txn.group).decode('ascii').lower()])
    logging.info(exp_messages)

    assert exp_messages == messages

    verify_key = nacl.signing.VerifyKey(pubKey)
    verify_key.verify(smessage=b'TX' + decoded_txn, signature=txnSig)
