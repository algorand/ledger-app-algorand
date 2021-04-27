import pytest
import logging
import struct
import algosdk

from . import speculos
from . import txn_utils
from . import ui_interaction


clicked_labels = {
    'verify' ,'address (', 'approve', 'reject'
}



def test_ins_with_no_payload(dongle):
    """
    Test that sending `INS_GET_PUBLIC_KEY` (0x03) APDU without payload
    returns a public key as a 32-byte long `bytes`.
    """
    try:
        apdu = struct.pack('>BBBBB', 0x80, 0x3, 0x0, 0x0, 0x0)
        key = dongle.exchange(apdu)
        assert type(key) == bytes
        assert len(key) == 32
    except speculos.CommException as e:
        logging.error(e)
        assert False


def test_ins_with_4_bytes_payload(dongle):
    """
    Test that sending `INS_GET_PUBLIC_KEY` (0x03) APDU with a
    4-byte (`uint32_t`) payload returns a public key as a
    32-byte long `bytes`.
    """
    try:
        apdu = struct.pack('>BBBBBI', 0x80, 0x3, 0x0, 0x0, 0x0, 0x0)
        key = dongle.exchange(apdu)
        assert len(key) == 32
    except speculos.CommException as e:
        logging.error(e)
        assert False



def test_ins_with_4_bytes_payload_and_user_approval(dongle):
    """
    Test that sending `INS_GET_PUBLIC_KEY` (0x03) APDU with a
    4-byte (`uint32_t`) payload returns a public key as a
    32-byte long `bytes`, after asking the user to approve the corresponding address
    """
    try:

        excpected_messages = [
        ['verify','address'],
        [],
        ['approve','address']]


        apdu = struct.pack('>BBBBBI', 0x80, 0x3, 0x80, 0x0, 0x0, 0x0)

        with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, clicked_labels, 'approve'):
            key = dongle.exchange(apdu)
            messages = dongle.get_messages()
            logging.info(messages)

        excpected_messages[1] = ['address',algosdk.encoding.encode_address(key).lower()]
        assert messages == excpected_messages
        
    except speculos.CommException as e:
        logging.error(e)
        assert False


@pytest.mark.parametrize('account_id', [1,2,10,50])
def test_ins_with_4_bytes_payload_and_user_approval_non_default_account(dongle,account_id):
    """
    Test the display UI when the app receives INS_GET_PUBLIC_KEY with different account ids
    """
    try:

        excpected_messages = [
        ['verify','address'],
        [],
        ['approve','address']]


        apdu = struct.pack('>BBBBBI', 0x80, 0x3, 0x80, 0x0, 0x0, account_id)

        with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, clicked_labels, 'approve'):
            key = dongle.exchange(apdu)
            messages = dongle.get_messages()
            logging.info(messages)

        excpected_messages[1] = ['address',algosdk.encoding.encode_address(key).lower()]
        assert messages == excpected_messages
        
    except speculos.CommException as e:
        logging.error(e)
        assert False



def test_ins_with_4_bytes_payload_and_user_reject(dongle):
    """
    """
    apdu = struct.pack('>BBBBBI', 0x80, 0x3, 0x80, 0x0, 0x0, 0x0)
    
    with dongle.screen_event_handler(ui_interaction.confirm_on_lablel, clicked_labels, "reject"):
            with pytest.raises(speculos.CommException) as excinfo:
                _ = dongle.exchange(apdu)
            assert excinfo.value.sw == 0x6985
    


@pytest.fixture(params=[1, 2, 3])
def invalid_size_apdu(request):
    l = request.param
    return struct.pack('>BBBBB%ds' % l, 0x80, 0x3, 0x0, 0x0, l, bytes(l))


def test_ins_with_small_paylod(dongle, invalid_size_apdu):
    """
    """
    with pytest.raises(speculos.CommException) as excinfo:
        dongle.exchange(invalid_size_apdu)
    assert excinfo.value.sw == 0x6a86



def test_ins_with_invalid_paylod_sizes(dongle):
    """
    """
    with pytest.raises(speculos.CommException) as excinfo:
        dongle.exchange(struct.pack('>BBBBB', 0x80, 0x3, 0x0, 0x0, 0x4))
    assert excinfo.value.sw == 0x6a87


def test_ins_without_payload_returns_account_0_key(dongle):
    """
    """
    try:
        apdu = struct.pack('>BBBBB', 0x80, 0x3, 0x0, 0x0, 0x0)
        key1 = dongle.exchange(apdu)
        assert type(key1) == bytes
        assert len(key1) == 32
        apdu = struct.pack('>BBBBBI', 0x80, 0x3, 0x0, 0x0, 0x4, 0x0)
        key2 = dongle.exchange(apdu)
        assert type(key2) == bytes
        assert len(key2) == 32

        assert key1 == key2
    except speculos.CommException as e:
        logging.error(e)
        assert False


@pytest.mark.parametrize('account_id', [1,2,10,50])
def test_ins_with_non_0_account_does_not_return_account_0_key(dongle, account_id):
    """
    """
    try:
        key1 = dongle.exchange(struct.pack('>BBBBB', 0x80, 0x3, 0x0, 0x0, 0x0))
        assert type(key1) == bytes
        assert len(key1) == 32
        key2 = dongle.exchange(struct.pack('>BBBBBI', 0x80, 0x3, 0x0, 0x0, 0x4, account_id))
        assert type(key2) == bytes
        assert len(key2) == 32

        assert key1 != key2
    except speculos.CommException as e:
        logging.error(e)
        assert False

@pytest.mark.parametrize('account_id', [1,2,10,50])
def test_ins_with_getting_same_pub_key_for_non_zero_account(dongle, account_id):
    """
    """
    try:
        key1 = dongle.exchange(struct.pack('>BBBBBI', 0x80, 0x3, 0x0, 0x0, 0x4, account_id))
        assert type(key1) == bytes
        assert len(key1) == 32
        key2 = dongle.exchange(struct.pack('>BBBBBI', 0x80, 0x3, 0x0, 0x0, 0x4, account_id))
        assert type(key2) == bytes
        assert len(key2) == 32

        assert key1 == key2
    except speculos.CommException as e:
        logging.error(e)
        assert False



