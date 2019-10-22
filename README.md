# Algorand App for Ledger Nano S

Run `make load` to build and load the application onto the device. After
installing and running the application, you can run `cli/sign.py`.  Running
without any arguments should print the address corresponding to the
key on the Ledger device.  To sign a transaction, run `cli/sign.py input.tx
output.tx`; this will ask the Ledger device to sign the transaction from
`input.tx`, and put the resulting signed transaction into `output.tx`.
You can use `goal clerk send .. -o input.tx` to construct an `input.tx`
file, and then use `goal clerk rawsend` to broadcast the `output.tx`
file to the Algorand network.

# Development notes

- [Ledger documentation](https://ledger.readthedocs.io/)
- [Development environment setup](https://ledger.readthedocs.io/en/latest/userspace/getting_started.html)
- [Sample applications](https://github.com/LedgerHQ/ledger-sample-apps)
- [Ethereum wallet](https://github.com/LedgerHQ/ledger-app-eth/)
- [SSH agent](https://github.com/LedgerHQ/ledger-app-ssh-agent/)
- [Nano S SDK](https://github.com/LedgerHQ/nanos-secure-sdk)

## Python environment

- `sudo apt install python-hid python-hidapi python3-hid python3-hidapi`
- `sudo pip install ledgerblue`
- Set up `/etc/udev/rules.d` based on [these notes](https://github.com/LedgerHQ/blue-loader-python)

## Firmware update

- Install [Ledger Live](https://www.ledger.com/pages/ledger-live)
- [Update firmware](https://support.ledger.com/hc/en-us/articles/360002731113)

## Setting up a custom CA for automating app loading

- [Documentation](https://ledger.readthedocs.io/en/latest/userspace/debugging.html)
- `python -m ledgerblue.genCAPair`
- `python -m ledgerblue.setupCustomCA --targetId 0x31100004 --public 040db5032de3dc9ac155959bca5e163d1ab35789192495c99b39dceb82dafb5ffad14ce7fd32d739388b6017c606f26028fdfa3e7000fa8c9793740a7aff839587 --name dev`
- `export SCP_PRIVKEY=7f189771ea6ee2808e4a66e6b74600b7eadb720a7ccf06bfe2ac0f67c7103250`

## PRINTF-style debugging

- `python -m ledgerblue.loadMCU --targetId 0x01000001 --fileName blup_0.9_misc_m1.hex --nocrc`
- `python -m ledgerblue.loadMCU --targetId 0x01000001 --fileName mcu_1.7-printf_over_0.9.hex --reverse --nocrc`
- `./usbtool/usbtool -v 0x2c97 log`
- Edit `Makefile` to enable `PRINTF` (and edit it back for production to disable `PRINTF`)

To go back to release firmware:

- [Instructions](https://ledger.readthedocs.io/en/latest/userspace/debugging.html)
- `python -m ledgerblue.loadMCU --targetId 0x01000001 --fileName blup_0.9_misc_m1.hex --nocrc`
- `python -m ledgerblue.loadMCU --targetId 0x01000001 --fileName mcu_1.7_over_0.9.hex --reverse --nocrc`

## Python HID debugging

- Pass `debug=True` to `getDongle()` in `cli/sign.py`

## Glyph/icon

- `convert -resize 12 -extent 16x16 -gravity center -colors 2 ...`

## Assorted complaints / tricks

- Need `volatile` for N_ variables; not correctly done in default example
- ed25519 public keys have an extra garbage byte upfront, then 64 bytes of uncompressed X and Y points, in reverse byte order
- Have to pass full message to `cx_eddsa_sign()` despite it being the "hash"
- bip32 keygen returns 64 bytes instead of 32 needed for Ed25519; not documented anywhere
- BSS not actually zeroed out; have to explicitly initialize global variables
- Can't call PRINTF after UX_DISPLAY
- Converting `(int)-2` to `(char)` and then back to `(int)` produces 254; the base32 library broke as a result
- [Weird memory behavior](https://github.com/LedgerHQ/ledger-dev-doc/blob/master/source/userspace/memory.rst)
