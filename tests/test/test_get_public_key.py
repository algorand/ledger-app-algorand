import pytest
import logging
import struct
import algosdk

from . import speculos


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

clicked_labels = {
    'verify', 
    'address'
}

def getPubKey_ui_handler(event, buttons, messages_seen):
    logging.warning(event)
    if type(event) == dict:
        label = event['text'].lower()
    elif type(event) == list:
        label = sorted(event, key=lambda e: e['y'])[0]['text'].lower()
    else:
        raise Exception(f"enexpceted events type is {type(event)}")
    logging.warning('label => %s' % label)

    messages_seen.append(label)

    logging.warning(len(list(filter(lambda l: l in label, clicked_labels))))
    if len(list(filter(lambda l: l in label, clicked_labels))) > 0:
        if label == "address":
            buttons.press(buttons.RIGHT, buttons.LEFT, buttons.RIGHT_RELEASE, buttons.LEFT_RELEASE)
        else:
            buttons.press(buttons.RIGHT, buttons.RIGHT_RELEASE)
    return messages_seen

def test_ins_with_4_bytes_payload_and_user_approval(dongle):
    """
    Test that sending `INS_GET_PUBLIC_KEY` (0x03) APDU with a
    4-byte (`uint32_t`) payload returns a public key as a
    32-byte long `bytes`, after asking the user to approve the corresponding address
    """
    try:

        excpected_messages = [
        ['verify',''],
        ['address',''],
        [],
        ['approve',''],
        ['address','']]


        apdu = struct.pack('>BBBBBI', 0x80, 0x3, 0x80, 0x0, 0x0, 0x0)

        with dongle.screen_event_handler(getPubKey_ui_handler):
            key = dongle.exchange(apdu)
            messages = dongle.get_messages()
            logging.info(messages)

        excpected_messages[2] = ['address',algosdk.encoding.encode_address(key).lower()]
        assert messages == excpected_messages
        
    except speculos.CommException as e:
        logging.error(e)
        assert False


@pytest.fixture(params=[1, 2, 3])
def invalid_size_apdu(request):
    l = request.param
    return struct.pack('>BBBBB%ds' % l, 0x80, 0x3, 0x0, 0x0, l, bytes(l))


def test_ins_with_invalid_paylod_sizes(dongle, invalid_size_apdu):
    """
    """
    with pytest.raises(speculos.CommException) as excinfo:
        dongle.exchange(invalid_size_apdu)
    assert excinfo.value.sw == 0x6a85


def test_ins_without_payload_returns_account_0_key(dongle):
    """
    """
    try:
        apdu = struct.pack('>BBBBB', 0x80, 0x3, 0x0, 0x0, 0x0)
        key1 = dongle.exchange(apdu)
        assert type(key1) == bytes
        assert len(key1) == 32
        apdu = struct.pack('>BBBBBI', 0x80, 0x3, 0x0, 0x0, 0x0, 0x0)
        key2 = dongle.exchange(apdu)
        assert type(key2) == bytes
        assert len(key2) == 32

        assert key1 == key2
    except speculos.CommException as e:
        logging.error(e)
        assert False


@pytest.fixture(params=[1, 2, 3, 5, 8, 14])
def account_apdu(request):
    account = request.param
    return struct.pack('>BBBBBI', 0x80, 0x3, 0x0, 0x0, 0x4, account)


def test_ins_with_non_0_account_does_not_return_account_0_key(dongle, account_apdu):
    """
    """
    try:
        key1 = dongle.exchange(struct.pack('>BBBBB', 0x80, 0x3, 0x0, 0x0, 0x0))
        assert type(key1) == bytes
        assert len(key1) == 32
        key2 = dongle.exchange(account_apdu)
        assert type(key2) == bytes
        assert len(key2) == 32

        assert key1 != key2
    except speculos.CommException as e:
        logging.error(e)
        assert False


