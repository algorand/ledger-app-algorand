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
UNKOWN_UI_TEXT = 'Unkown'


@pytest.fixture
def app_call_txn():
   # demo = b'\x02 \x05\x00\x01\x02\x04\x05&\x02 \xed\xed\x8fD-\xc1\xc5,h\xea\xe9\xc57{C\xa2S\xc7\xcff\xe2\xff\xf2\xc0\x81\xa2\x89\xec\xe2&\x80\x0b\x07counter1\x19"\x12@\x00\x1e1\x19#\x12@\x0061\x19$\x12@\x0011\x19%\x12@\x0011\x19!\x04\x12@\x00$\x00(1\x00\x12@\x00\x18)Id#\x08I5\x00g")b#\x085\x01")4\x01f4\x00C#C#C(1\x00\x12C(1\x00\x12C'
    #approve_app= b'\x02 \x05\x00\x01\x02\x04\x05&\x02 \xed\xed\x8fD-\xc1\xc5,h\xea\xe9\xc57{C\xa2S\xc7\xcff\xe2\xff\xf2\xc0\x81\xa2\x89\xec\xe2&\x80\x0b\x07counter1\x19"\x12@\x00\x1e1\x19#\x12@\x0061\x19$\x12@\x0011\x19%\x12@\x0011\x19!'
    approve_app = b'\x02 \x05\x00\x05\x04\x02\x01&\x07\x04vote\tVoteBegin\x07VoteEnd\x05voted\x08RegBegin\x06RegEnd\x07Creator1\x18"\x12@\x00\x951\x19\x12@\x00\x871\x19$\x12@\x00y1\x19%\x12@\x00R1\x19!\x04\x12@\x00<6\x1a\x00(\x12@\x00\x01\x002\x06)d\x0f2\x06*d\x0e\x10@\x00\x01\x00"2\x08+c5\x005\x014\x00A\x00\x02"C6\x1a\x016\x1a\x01d!\x04\x08g"+6\x1a\x01f!\x04C2\x06\'\x04d\x0f2\x06\'\x05d\x0e\x10C"2\x08+c5\x005\x012\x06*d\x0e4\x00\x10A\x00\t4\x014\x01d!\x04\tg!\x04C1\x00\'\x06d\x12C1\x00\'\x06d\x12C\'\x061\x00g1\x1b$\x12@\x00\x01\x00\'\x046\x1a\x00\x17g\'\x056\x1a\x01\x17g)6\x1a\x02\x17g*6\x1a\x03\x17g!\x04C'
    clear_pgm = b'\x02 \x01\x01'

    # the approve_app is comipled Tealscript taken from https://pyteal.readthedocs.io/en/stable/examples.html#periodic-payment
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
                                                         sp=local_sp, approval_program=approve_app, on_complete=transaction.OnComplete.NoOpOC.real,clear_program= clear_pgm, global_schema=global_schema, local_schema=local_schema )
    return txn


def get_hash_of_pgm(program_bytes):
    h = SHA256.new()
    h.update(program_bytes)
    return base64.b64encode(h.digest()) 

def get_expected_messages(current_txn):
    if current_txn.on_complete in OPERATION_UI_TEXT:
        operation_text = OPERATION_UI_TEXT[current_txn.on_complete]
    else:
        operation_text = UNKOWN_UI_TEXT
    
    
    messages =  [['review', 'transaction'],
                 ['txn type', 'application'], 
                 ['sender', current_txn.sender.lower()],
                 ['fee (alg)', str(current_txn.fee*0.000001)],
                 ['genesis id', current_txn.genesis_id.lower()], 
                 ['genesis hash', current_txn.genesis_hash.lower()],
                 ['app id', current_txn.index],
                 ['on completion', operation_text],
                 ['global schema', f'uint: {current_txn.global_schema.num_uints}, byte: {current_txn.global_schema.num_byte_slices}'],
                 ['local schema', f'uint: {current_txn.local_schema.num_uints}, byte: {current_txn.local_schema.num_byte_slices}'],
                 ['apprv (sha256)', get_hash_of_pgm(current_txn.approval_program).lower()],
                 ['clear (sha256)', get_hash_of_pgm(current_txn.clear_program).lower()],
                 ['sign', 'transaction']]
                 
    return messages



txn_labels = {
    'review', 'txn type','sender','fee (alg)', 'genesis id', 'genesis hash', 'app id', 'on completion','global schema', 'local schema',  'apprv (sha256)', 'clear (sha256)','sign'
} 

conf_label = "sign"



def test_sign_msgpack_asset_validate_display(dongle, app_call_txn):
    """
    """
    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(app_call_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        _ = txn_utils.sign_algo_txn(dongle, decoded_txn)
        messages = dongle.get_messages()
    logging.info(messages)
    logging.info(get_expected_messages(app_call_txn))
    assert get_expected_messages(app_call_txn) == messages

    
    
def test_sign_msgpack_with_default_account(dongle, app_call_txn):
    """
    """
    apdu = struct.pack('>BBBBB', 0x80, 0x3, 0x0, 0x0, 0x0)
    pubKey = dongle.exchange(apdu)
    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(app_call_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        txnSig = txn_utils.sign_algo_txn(dongle, decoded_txn)

    assert len(txnSig) == 64
    verify_key = nacl.signing.VerifyKey(pubKey)
    verify_key.verify(smessage=b'TX' + decoded_txn, signature=txnSig)
