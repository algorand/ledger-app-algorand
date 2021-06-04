import struct
import logging

def chunks(txn, chunk_size=250):
    size = chunk_size
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


def sign_algo_txn(dongle, txn,chunk_size=250, p1=0x00):
    for apdu in apdus(chunks(txn, chunk_size), p1=(p1 & 0x7f)):
        sig = dongle.exchange(apdu)
    return sig

def sign_algo_txn_with_account(dongle, txn, account_id, chunk_size=250, p1=0x00):
    return sign_algo_txn(dongle, struct.pack('>I',account_id) +txn, chunk_size, p1 | 0x1)


   

