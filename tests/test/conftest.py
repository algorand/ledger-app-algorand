import pytest

from .speculos import SpeculosContainer

import base64
import msgpack
from algosdk import transaction

import algomsgpack


@pytest.fixture(scope='session')
def app(pytestconfig):
    return pytestconfig.option.app


@pytest.fixture(scope='session')
def apdu_port(pytestconfig):
    return pytestconfig.option.apdu_port


@pytest.fixture(scope='session')
def speculos(app, apdu_port):
    speculos = SpeculosContainer(app=app, apdu_port=apdu_port)
    speculos.start()
    print("Started container")
    yield speculos
    print("Stopping container")
    speculos.stop()


@pytest.fixture(scope='session')
def dongle(speculos, pytestconfig):
    dongle = speculos.connect(debug=pytestconfig.option.verbose > 0)
    print("Connected dongle")
    yield dongle
    print("Disconnecting dongle")
    dongle.close()


def pytest_addoption(parser, pluginmanager):
    parser.addoption("--app", dest="app")
    parser.addoption("--apdu_port", dest="apdu_port", type=int, default=9999)
    



def genTxns():
    yield transaction.PaymentTxn(
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

def genTxnPayload(txns):
    for txn in txns:
        if isinstance(txn, Transaction):
            txn = {"txn": txn.dictify()}
        yield base64.b64decode(encoding.msgpack_encode(txn))
