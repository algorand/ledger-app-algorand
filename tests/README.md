# app-algorand-test

## Functional tests

### Overview

Pytest-base test suite for the [app-algorand](https://github.com/LedgerHQ/app-algorand) Nano App.

This test suite is mainly based on:
 - `pytest` framework.
 -  ̀speculos` Docker container.
 - `ledgerblue` tool.
 - Docker Python SDK.
 - Algorand Python SDK.


### Install

This test suite requires [https://docs.docker.com/engine/install/ubuntu/](Docker) engine
and assumes the [https://hub.docker.com/r/ledgerhq/speculos](`speculos`) image being pulled.
Python environment may be installed within a virtualenv:

  ```
  docker pull ghcr.io/ledgerhq/speculos
  virtualenv env
  env/bin/activate
  pip install -r requirements.txt
  ```

### Run tests

  ```
  make test
  ```

## Unit tests

### Build

```console
cmake -Btests/build -Htests/
make -C tests/build/
```

### Run

```console
make -C tests/build/ test
```

Arguments can be given to `ctest`. For instance, to make the output of the test
`test_str` verbose:

```console
make -C tests/build/ test ARGS='-V -R test_str'
```

## Fuzzing

### Build

```console
cmake -Btests/build -Htests/
make -C tests/build/
```

Code coverage can be enabled thanks to:

```console
cmake -Btests/build -Htests/ -DCODE_COVERAGE=1
```

### Run

```console
export ASAN_SYMBOLIZER_PATH=/usr/lib/llvm-9/bin/llvm-symbolizer
./tests/build/fuzz_tx
```

A corpus of transactions can also be used:

```
cp -r tests/fuzzing-corpus /tmp/corpus
./tests/build/fuzz_tx /tmp/corpus
```

The corpus was generated using `tests/generate-corpus.patch` against https://github.com/algorand/py-algorand-sdk/blob/6b304e4b080d95286f836d8cd6d8cc3aeeefdfd5/test_unit.py.

If code coverage is enabled, the fuzzer should be run with a limited number of individual tests, for instance:

```console
export LLVM_PROFILE_FILE=/tmp/fuzzing.profraw
./tests/build/fuzz_tx -runs=1000000
```

and results can be viewed with the following commands:

```console
llvm-profdata-9 merge --sparse /tmp/fuzzing.profraw -o /tmp/fuzzing.profdata
llvm-cov-9 show tests/build/fuzz_tx -instr-profile=/tmp/fuzzing.profdata -show-line-counts-or-regions -output-dir=/tmp/html-coverage -format=html
www-browser /tmp/html-coverage/
```

## APDU Format for Multi-Account Support

The format of the APDUs in app release that implements multi-account support has been kept backward compatible with
previous message format (1.0.7). Two messages have been modified to implement multi-account support.

### `INS_GET_PUBLIC_KEY`:

Original format of this instruction is fixed with no payload.
<pre>
    ------------------------------------
    | CLA  | INS  |  P1  |  P2  |  LC  |
    ------------------------------------
    | 0x80 | 0x03 | 0x00 | 0x00 | 0x00 |
    ------------------------------------
</pre>

New format enhances this APDU with a 4-byte payload that encodes an account number (big endian 32-bit unsigned integer).
This payload is optional so that former format may still be used. So user may send:
<pre>
    ------------------------------------
    | CLA  | INS  |  P1  |  P2  |  LC  |
    ------------------------------------
    | 0x80 | 0x03 | 0x00 | 0x00 | 0x00 |
    ------------------------------------
</pre>
or
<pre>
    ----------------------------------------------------------------
    | CLA  | INS  |  P1  |  P2  |  LC  |    PAYLOAD (4 bytes)      |
    ----------------------------------------------------------------
    | 0x80 | 0x03 | 0x00 | 0x00 | 0x04 |        {account}          |
    ----------------------------------------------------------------
</pre>

The account number is used to derive keys from BIP32 path `44'/283'/<account>'/0/0`
(note that the account number is hardened as shown by the `'` sign). Account number defaults
to `0x0` in the case of APDU with empty payload.


### `INS_SIGN_MSGPACK`

Original format is as shown below where transaction contents may be split in multiple APDUs:
<pre>
    ------------------------------------------------------------------------ - - -
    | CLA  | INS  |  P1  |  P2  |  LC  |  PAYLOAD (N1 bytes)
    ------------------------------------------------------------------------ - - -
    | 0x80 | 0x03 | 0x00 | 0x80 |  N1  | {MessagePack Chunk#1}
    ------------------------------------------------------------------------ - - -
    ...
    ------------------------------------------------------------------------ - - -
    | CLA  | INS  |  P1  |  P2  |  LC  |  PAYLOAD (Ni bytes)
    ------------------------------------------------------------------------ - - -
    | 0x80 | 0x03 | 0x80 | 0x80 |  Ni   | {MessagePack Chunk#i}
    ------------------------------------------------------------------------ - - -
    ...
    ------------------------------------------------------------------------ - - -
    | CLA  | INS  |  P1  |  P2  |  LC  |  PAYLOAD (NI bytes)
    ------------------------------------------------------------------------ - - -
    | 0x80 | 0x03 | 0x80 | 0x00 |  NI  | {MessagePack Chunk#I}
    ------------------------------------------------------------------------ - - -
</pre>
If one single APDU may contain a whole transaction, `P1` and `P2` are both `0x00`.

New format enhances messaging with an optional account number that must be inserted
in the first chunk of the sequence. As an optional payload, bit `0` of field `P1` in
the first chunk must be set if present in the message.

And as for `INS_GET_PUBLIC_KEY` instruction, it is a big-endian encoded 32-bit
unsigned integer word. 

The resulting sequence of chunks is as follows:
<pre>
    ------------------------------------------------------------------------ - - -
    | CLA  | INS  |  P1  |  P2  |  LC  |  PAYLOAD (N1 bytes)
    ------------------------------------------------------------------------ - - -
    | 0x80 | 0x03 | 0x01 | 0x80 |  N1  | {account (4 bytes)} + {MessagePack Chunk#1 (N1 - 4 bytes)}
    ------------------------------------------------------------------------ - - -
    ...
    ------------------------------------------------------------------------ - - -
    | CLA  | INS  |  P1  |  P2  |  LC  |  PAYLOAD (Ni bytes)
    ------------------------------------------------------------------------ - - -
    | 0x80 | 0x03 | 0x80 | 0x80 |  Ni   | {MessagePack Chunk#i}
    ------------------------------------------------------------------------ - - -
    ...
    ------------------------------------------------------------------------ - - -
    | CLA  | INS  |  P1  |  P2  |  LC  |  PAYLOAD (NI bytes)
    ------------------------------------------------------------------------ - - -
    | 0x80 | 0x03 | 0x80 | 0x00 |  NI  | {MessagePack Chunk#I}
    ------------------------------------------------------------------------ - - -
</pre>
If one signle APDU is needed for the whole transaction along with the account number,
`P1` and `P2` are `0x01` and `0x00` respectively. 

If the account number is not inserted within the message, the former message format is used
(`P1` in the first chunk is `0x00`) and the account number defaults to `0x00` for the transaction
signature.

