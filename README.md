# Algorand App for Ledger Nano S

Run `make load` to build and load the application onto the device. After
installing and running the application, you can run `sign.py`.  Running
without any arguments should print the address corresponding to the
key on the Ledger device.  To sign a transaction, run `sign.py input.tx
output.tx`; this will ask the Ledger device to sign the transaction from
`input.tx`, and put the resulting signed transaction into `output.tx`.
You can use `goal clerk send .. -o input.tx` to construct an `input.tx`
file, and then use `goal clerk rawsend` to broadcast the `output.tx`
file to the Algorand network.
