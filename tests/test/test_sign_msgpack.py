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
    return  algosdk.transaction.PaymentTxn(
        sender="YK54TGVZ37C7P76GKLXTY2LAH2522VD3U2434HRKE7NMXA65VHJVLFVOE4",
        receiver="RNZZNMS5L35EF6IQHH24ISSYQIKTUTWKGCB4Q5PBYYSTVB5EYDQRVYWMLE",
        fee=0.001,
        flat_fee=True,
        amt=1000000,
        first=5667360,
        last=5668360,
        note="Hello World".encode(),
        gen="testnet-v1.0",
        gh="SGO1GKSzyE7IEPItTxCByw9x8FmnrCDexi9/cOUJOiI="
    )

@pytest.fixture
def txn():
    return base64.b64decode(algosdk.encoding.msgpack_encode(get_default_tnx()))


def get_expected_messages(tnx):
    messages =  [['review', 'transaction'],
             ['txn type', 'payment'],
             ['sender', tnx.sender.lower()],
             ['fee (alg)', str(tnx.fee*0.000001)],
             ['genesis id', tnx.genesis_id.lower()],
             ['genesis hash', tnx.genesis_hash.lower()],
             ['note', f'{len(tnx.note)} bytes'],
             ['receiver', tnx.receiver.lower()], 
             ['amount (alg)', str(int(tnx.amt/1000000))],
             ['sign', 'transaction']]

    return messages


txn_labels = {
    'review', 'txn type', 'sender', 'fee', 'first valid', 'last valid',
    'genesis', 'note', 'receiver', 'amount', 'sign'
} 

conf_label = "sign"


def test_sign_msgpack_validate_display(dongle, txn):
    """
    """

    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(txn)
        _ = txn_utils.sign_algo_txn(dongle, txn)
        messages = dongle.get_messages()
    logging.info(messages)
    logging.info(get_expected_messages(get_default_tnx()))
    assert get_expected_messages(get_default_tnx()) == messages

    
    

@pytest.mark.parametrize('account_id', [0,1,2,10,50])
def test_sign_msgpack_with_spcific_account(dongle, txn, account_id):
    """
    """
    apdu = struct.pack('>BBBBBI', 0x80, 0x3, 0x0, 0x0,0x4, account_id)
    pubKey = dongle.exchange(apdu)

    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(txn)
        txnSig = txn_utils.sign_algo_txn_with_account(dongle, txn, account_id)

    assert len(txnSig) == 64
    verify_key = nacl.signing.VerifyKey(pubKey)
    verify_key.verify(smessage=b'TX' + txn, signature=txnSig)


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


def test_sign_msgpack_wrong_size_in_payload(dongle, txn):
    """
    """
    with pytest.raises(speculos.CommException) as excinfo:
        dongle.exchange(struct.pack('>BBBBB10s' , 0x80, 0x8, 0x0, 0x0, 20, bytes(10)))
        
    assert excinfo.value.sw == 0x6a85

@pytest.mark.parametrize('chunk_size', [10, 20 ,50, 250])
def test_sign_msgpack_differnet_chunk_size(dongle, txn,chunk_size):
    """
    """
    apdu = struct.pack('>BBBBB', 0x80, 0x3, 0x0, 0x0, 0x0)
    pubKey = dongle.exchange(apdu)

    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(txn)
        txnSig = txn_utils.sign_algo_txn(dongle, txn,chunk_size)

    assert len(txnSig) == 64
    verify_key = nacl.signing.VerifyKey(pubKey)
    verify_key.verify(smessage=b'TX' + txn, signature=txnSig)

def test_sign_tnx_larger_then_internal_buffer(dongle, txn):
    """
    """
    with pytest.raises(speculos.CommException) as excinfo:
        tnx = get_default_tnx()
        tnx.note = ("1"*800).encode()  
        
        dongle.exchange(txn_utils.sign_algo_txn(dongle, base64.b64decode(algosdk.encoding.msgpack_encode(tnx))))
        
    assert excinfo.value.sw == 0x6700


def test_sign_tnx_long_field(dongle, txn):
    """
    """
    with pytest.raises(speculos.CommException) as excinfo:
        tnx = get_default_tnx()
        tnx.note = ("1"*500).encode()  
        
        dongle.exchange(txn_utils.sign_algo_txn(dongle, base64.b64decode(algosdk.encoding.msgpack_encode(tnx))))
        
    assert excinfo.value.sw == 0x6e00


@pytest.mark.parametrize('account_id', [0, 1, 3, 7, 10, 42, 12345])
def test_sign_msgpack_with_valid_account_id(dongle, txn, account_id):
    """
    """
    apdu = struct.pack('>BBBBBI', 0x80, 0x3, 0x0, 0x0, 0x4, account_id)
    pubKey = dongle.exchange(apdu)

    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(txn)
        txnSig = txn_utils.sign_algo_txn(dongle=dongle,
                               txn=struct.pack('>I', account_id) + txn,
                               p1=0x1)

    assert len(txnSig) == 64
    verify_key = nacl.signing.VerifyKey(pubKey)
    verify_key.verify(smessage=b'TX' + txn, signature=txnSig)


def test_sign_msgpack_returns_same_signature(dongle, txn):
    """
    """
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        defaultTxnSig = txn_utils.sign_algo_txn(dongle, txn)
        

    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        txnSig = txn_utils.sign_algo_txn(dongle=dongle,
                               txn=struct.pack('>I', 0x0) + txn,
                               p1=0x1)

    assert txnSig == defaultTxnSig


