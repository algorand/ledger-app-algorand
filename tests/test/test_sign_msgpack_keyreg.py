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

    b64votekey = "eXq34wzh2UIxCZaI1leALKyAvSz/+XOe0wqdHagM+bw="
    votekey_addr = algosdk.encoding.encode_address(base64.b64decode(b64votekey))
    b64selkey = "X84ReKTmp+yfgmMCbbokVqeFFFrKQeFZKEXG89SXwm4="
    selkey_addr = algosdk.encoding.encode_address(base64.b64decode(b64selkey))
    txn = algosdk.transaction.KeyregTxn(
        sender="YTOO52XR6UWNM6OUUDOGWVTNJYBWR5NJ3VCJTZUSR42JERFJFAG3NFD47U",
        votekey=votekey_addr,
        selkey=selkey_addr,
        votefst= 6000000,
        votelst=9000000,
        votekd= 1730,
        fee= 2000,
        flat_fee=True,
        first=6002000,
        last=6003000,
        gen="testnet-v1.0",
        gh="SGO1GKSzyE7IEPItTxCByw9x8FmnrCDexi9/cOUJOiI="

    )
    return txn

@pytest.fixture
def txn():
    return base64.b64decode(algosdk.encoding.msgpack_encode(get_default_tnx()))


def get_expected_messages(current_tnx):
    votepk = str(base64.b64encode(algosdk.encoding.decode_address(current_tnx.votepk)),'ascii').lower()
    vrfpk = str(base64.b64encode(algosdk.encoding.decode_address(current_tnx.selkey)),'ascii').lower()
    messages =  [['review', 'transaction'],
                 ['txn type', 'key reg'], 
                 ['sender', current_tnx.sender.lower()], 
                 ['fee (alg)', str(current_tnx.fee*0.000001)], 
                 ['genesis id', current_tnx.genesis_id.lower()], 
                 ['genesis hash', current_tnx.genesis_hash.lower()],
                 ['vote pk', votepk],
                 ['vrf pk', vrfpk], 
                 ['vote first', str(current_tnx.votefst)], 
                 ['vote last', str(current_tnx.votelst)], 
                 ['key dilution', str(current_tnx.votekd)], 
                 ['participating', 'yes'],
                 ['sign', 'transaction']]

    return messages


txn_labels = {
    'review', 'txn type', 'sender', 'vote pk','fee (alg)', 'genesis id', 'genesis hash', 'vrf pk', 
    'vote first',  'vote last', 'key dilution', 'participating', 'transaction'
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
