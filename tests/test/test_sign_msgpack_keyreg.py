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
def keyreg_txn():
    sp_var = algosdk.future.transaction.SuggestedParams(2000, 6002000, 6002000, "SGO1GKSzyE7IEPItTxCByw9x8FmnrCDexi9/cOUJOiI=", "testnet-v1.0", True)

    b64votekey = "1V2BE2lbFvS937H7pJebN0zxkqe1Nrv+aVHDTPbYRlw="
    b64selkey = "87iBW46PP4BpTDz6+IEGvxY6JqEaOtV0g+VWcJqoqtc="
    stateproofkey = "f0CYOA4yXovNBFMFX+1I/tYVBaAl7VN6e0Ki5yZA3H6jGqsU/LYHNaBkMQ/rN4M4F3UmNcpaTmbVbq+GgDsrhQ=="

    txn = algosdk.future.transaction.KeyregTxn(
        sender="YTOO52XR6UWNM6OUUDOGWVTNJYBWR5NJ3VCJTZUSR42JERFJFAG3NFD47U",
        sp=sp_var,
        votekey=b64votekey,
        selkey=b64selkey,
        votefst= 6200000,
        votelst=9500000,
        votekd= 1730,
        sprfkey=stateproofkey,
    )
    return txn


def get_expected_messages(current_txn):
    # if current_txn.? == True:
    #     participating_flag = 'yes'
    # else:
    #     participating_flag = 'no'
    messages =  [['review', 'transaction'],
                 ['txn type', 'key reg'], 
                 ['sender', current_txn.sender.lower()], 
                 ['fee (alg)', str(current_txn.fee*0.000001)], 
                 ['genesis id', current_txn.genesis_id.lower()], 
                 ['genesis hash', current_txn.genesis_hash.lower()],
                 ['vote pk', current_txn.votepk.lower()],
                 ['vrf pk', current_txn.selkey.lower()],
                 ['stateproof pk', current_txn.sprfkey.lower()], 
                 ['vote first', str(current_txn.votefst)], 
                 ['vote last', str(current_txn.votelst)], 
                 ['key dilution', str(current_txn.votekd)], 
                 ['participating', 'yes'],
                 ['sign', 'transaction']]

    return messages



txn_labels = {
    'review', 'txn type', 'sender', 'fee (alg)', 'genesis id', 'genesis hash', 'vote pk','vrf pk', 'stateproof pk',
    'vote first',  'vote last', 'key dilution', 'participating', 'sign'
} 

conf_label = "sign"



def test_sign_msgpack_asset_validate_display(dongle, keyreg_txn):
    """
    """
    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(keyreg_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        _ = txn_utils.sign_algo_txn(dongle, decoded_txn)
        messages = dongle.get_messages()
    logging.info(messages)
    logging.info(get_expected_messages(keyreg_txn))
    assert get_expected_messages(keyreg_txn) == messages

    
    
def test_sign_msgpack_with_default_account(dongle, keyreg_txn):
    """
    """
    apdu = struct.pack('>BBBBB', 0x80, 0x3, 0x0, 0x0, 0x0)
    pubKey = dongle.exchange(apdu)
    decoded_txn= base64.b64decode(algosdk.encoding.msgpack_encode(keyreg_txn))
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, txn_labels, conf_label):
        logging.info(decoded_txn)
        txnSig = txn_utils.sign_algo_txn(dongle, decoded_txn)

    assert len(txnSig) == 64
    verify_key = nacl.signing.VerifyKey(pubKey)
    verify_key.verify(smessage=b'TX' + decoded_txn, signature=txnSig)
