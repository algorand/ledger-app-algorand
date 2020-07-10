import pytest
import logging
import struct
import base64

import msgpack
import nacl.signing

import algosdk

from . import speculos


labels = {
    'review', 'txn type', 'sender', 'fee', 'first valid', 'last valid',
    'genesis', 'note', 'receiver', 'amount', 'sign'
}


@pytest.fixture
def txn():
    txn = algosdk.transaction.PaymentTxn(
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
    # txn = {"txn": txn.dictify()}
    return base64.b64decode(algosdk.encoding.msgpack_encode(txn))


def test_sign_msgpack_with_default_account(dongle, txn):
    """
    """
    apdu = struct.pack('>BBBBB', 0x80, 0x3, 0x0, 0x0, 0x0)
    pubKey = dongle.exchange(apdu)

    with dongle.screen_event_handler(txn_ui_handler):
        logging.info(txn)
        txnSig = sign_algo_txn(dongle, txn)

    assert len(txnSig) == 64
    verify_key = nacl.signing.VerifyKey(pubKey)
    verify_key.verify(smessage=b'TX' + txn, signature=txnSig)


@pytest.mark.parametrize('account_id', [0, 1, 3, 7, 10, 42, 12345])
def test_sign_msgpack_with_valid_account_id(dongle, txn, account_id):
    """
    """
    apdu = struct.pack('>BBBBBI', 0x80, 0x3, 0x0, 0x0, 0x4, account_id)
    pubKey = dongle.exchange(apdu)

    with dongle.screen_event_handler(txn_ui_handler):
        logging.info(txn)
        txnSig = sign_algo_txn(dongle=dongle,
                               txn=struct.pack('>I', account_id) + txn,
                               p1=0x1)

    assert len(txnSig) == 64
    verify_key = nacl.signing.VerifyKey(pubKey)
    verify_key.verify(smessage=b'TX' + txn, signature=txnSig)


def test_sign_msgpack_returns_same_signature(dongle, txn):
    """
    """
    with dongle.screen_event_handler(txn_ui_handler):
        defaultTxnSig = sign_algo_txn(dongle, txn)

    with dongle.screen_event_handler(txn_ui_handler):
        txnSig = sign_algo_txn(dongle=dongle,
                               txn=struct.pack('>I', 0x0) + txn,
                               p1=0x1)

    assert txnSig == defaultTxnSig


def txn_ui_handler(event, buttons):
    logging.warning(event)
    label = sorted(event, key=lambda e: e['y'])[0]['text'].lower()
    logging.warning('label => %s' % label)
    if len(list(filter(lambda l: l in label, labels))) > 0:
        if label == "sign":
            buttons.press(buttons.RIGHT, buttons.LEFT, buttons.RIGHT_RELEASE, buttons.LEFT_RELEASE)
        else:
            buttons.press(buttons.RIGHT, buttons.RIGHT_RELEASE)


def chunks(txn, chunk_size=250, first_chunk_size=250):
    size = first_chunk_size
    last = False
    while not last:
        chunk = txn[:size]
        txn = txn[len(chunk):]
        last = not txn
        size = chunk_size
        yield chunk, last


def apdus(chunks, p1=0x00, p2=0x80):
    for chunk, last in chunks:
        if last:
            p2 &= ~0x80
        size = len(chunk)
        yield struct.pack('>BBBBB%ds' % size, 0x80, 0x08, p1, p2, size, chunk)
        p1 |= 0x80


def sign_algo_txn(dongle, txn, p1=0x00):
    for apdu in apdus(chunks(txn), p1=p1 & 0x7f):
        sig = dongle.exchange(apdu)
    return sig

