#include "main.h"
#include "ui_idle.h"
#include "ui_tx_approval.h"

//------------------------------------------------------------------------------

#define OFFSET_CLA    0
#define OFFSET_INS    1
#define OFFSET_P1     2
#define OFFSET_P2     3
#define OFFSET_LC     4
#define OFFSET_CDATA  5

#define CLA                 0x80
#define INS_SIGN_PAYMENT    0x01    // Deprecated, unused
#define INS_SIGN_KEYREG     0x02    // Deprecated, unused
#define INS_GET_PUBLIC_KEY  0x03
#define INS_SIGN_PAYMENT_V2 0x04
#define INS_SIGN_KEYREG_V2  0x05
#define INS_SIGN_PAYMENT_V3 0x06
#define INS_SIGN_KEYREG_V3  0x07

//------------------------------------------------------------------------------

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];
context_t context;

//------------------------------------------------------------------------------

static void
algorand_main(void);
static void
app_exit(void);
static void
handleApdu(volatile unsigned int *flags, volatile unsigned int *tx);
static void
handleGetPublicKey(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength,
                   volatile unsigned int *flags, volatile unsigned int *tx);
static void
handleSignPaymentV3(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength,
                    volatile unsigned int *flags, volatile unsigned int *tx);
static void
handleSignKeyRegV3(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength,
                   volatile unsigned int *flags, volatile unsigned int *tx);
static void
copy_and_advance(void *dst, uint8_t **p, size_t len);

//------------------------------------------------------------------------------

__attribute__((section(".boot")))
int
main(void)
{
  // exit critical section
  __asm volatile("cpsie i");

  os_memset(&context, 0, sizeof(context));
  ui_common_init();

  // ensure exception will work as planned
  os_boot();

  for (;;) {
    UX_INIT();

    BEGIN_TRY {
      TRY {
        io_seproxyhal_init();

        USB_power(0);
        USB_power(1);

        ui_idle();

        algorand_main();
      }
      CATCH (EXCEPTION_IO_RESET) {
        // reset IO and UX before continuing
        continue;
      }
      CATCH_ALL {
        break;
      }
      FINALLY {
      }
    }
    END_TRY;
  }
  app_exit();
  return 0;
}

//------------------------------------------------------------------------------

static void
algorand_main(void)
{
  volatile unsigned int rx = 0;
  volatile unsigned int tx = 0;
  volatile unsigned int flags = 0;

  // DESIGN NOTE: the bootloader ignores the way APDU are fetched. The only
  // goal is to retrieve APDU.
  // When APDU are to be fetched from multiple IOs, like NFC+USB+BLE, make
  // sure the io_event is called with a
  // switch event, before the apdu is replied to the bootloader. This avoid
  // APDU injection faults.
  for (;;) {
    volatile unsigned short sw = 0;

    BEGIN_TRY {
      TRY {
        rx = tx;
        tx = 0; // ensure no race in catch_other if io_exchange throws
                // an error
        rx = io_exchange(CHANNEL_APDU | flags, rx);
        flags = 0;

        // no apdu received, well, reset the session, and reset the
        // bootloader configuration
        if (rx == 0) {
          THROW(0x6982);
        }

        handleApdu(&flags, &tx);
      }
      CATCH(EXCEPTION_IO_RESET) {
        THROW(EXCEPTION_IO_RESET);
      }
      CATCH_OTHER(e) {
        switch (e & 0xF000) {
          case 0x6000:
            // Wipe the transaction context and report the exception
            sw = e;
            os_memset(&context, 0, sizeof(context));
            break;
          case 0x9000:
            // All is well
            sw = e;
            break;
          default:
            // Internal error
            sw = 0x6800 | (e & 0x7FF);
            break;
        }
        if (e != 0x9000) {
          flags &= ~IO_ASYNCH_REPLY;
        }
        // Unexpected exception => report
        G_io_apdu_buffer[tx] = sw >> 8;
        G_io_apdu_buffer[tx + 1] = sw;
        tx += 2;
      }
      FINALLY {
      }
    }
    END_TRY;
  }

//return_to_dashboard:
  return;
}

static void
app_exit(void)
{
  BEGIN_TRY_L(exit) {
    TRY_L(exit) {
      os_sched_exit(-1);
    }
    FINALLY_L(exit) {
    }
  }
  END_TRY_L(exit);
  return;
}

static void
handleApdu(volatile unsigned int *flags, volatile unsigned int *tx)
{
  unsigned short sw = 0;

  BEGIN_TRY {
    TRY {
      if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
        THROW(0x6E00);
      }

      switch (G_io_apdu_buffer[OFFSET_INS]) {
        case INS_GET_PUBLIC_KEY:
          handleGetPublicKey(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], &G_io_apdu_buffer[OFFSET_CDATA], G_io_apdu_buffer[OFFSET_LC], flags, tx);
          break;
        case INS_SIGN_PAYMENT_V3:
          handleSignPaymentV3(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], &G_io_apdu_buffer[OFFSET_CDATA], G_io_apdu_buffer[OFFSET_LC], flags, tx);
          break;
        case INS_SIGN_KEYREG_V3:
          handleSignKeyRegV3(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], &G_io_apdu_buffer[OFFSET_CDATA], G_io_apdu_buffer[OFFSET_LC], flags, tx);
          break;
        default:
          THROW(0x6D00);
          break;
      }
    }
    CATCH(EXCEPTION_IO_RESET) {
      THROW(EXCEPTION_IO_RESET);
    }
    CATCH_OTHER(e) {
      switch (e & 0xF000) {
        case 0x6000:
          // Wipe the transaction context and report the exception
          sw = e;
          os_memset(&context, 0, sizeof(context));
          break;
        case 0x9000:
          // All is well
          sw = e;
          break;
        default:
          // Internal error
          sw = 0x6800 | (e & 0x7FF);
          break;
      }
      // Unexpected exception => report
      G_io_apdu_buffer[*tx] = sw >> 8;
      G_io_apdu_buffer[*tx + 1] = sw;
      *tx += 2;
    }
    FINALLY {
    }
  }
  END_TRY;
}

static void
handleGetPublicKey(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength,
                   volatile unsigned int *flags, volatile unsigned int *tx)
{
  uint8_t publicKey[32];

  UNUSED(dataLength);
  UNUSED(flags);

  if (p1 != 0 || p2 != 0) {
    THROW(0x6B00);
  }

  algorand_public_key(publicKey);

  os_memmove(G_io_apdu_buffer, publicKey, sizeof(publicKey));
  *tx = sizeof(publicKey);

  THROW(0x9000);
  return;
}

static void
handleSignPaymentV3(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength,
                    volatile unsigned int *flags, volatile unsigned int *tx)
{
  if ((p1 != 0 && p1 != 1) || p2 != 0) {
    THROW(0x6B00);
  }

  if (context.state == 0) {
    uint8_t *p;

    if (dataLength != 32 + 8 + 8 + 8 + 32 + 32 + 32 + 8 + 32) {
      THROW(0x6B00);
    }

    p = dataBuffer;
    context.current_tx.type = PAYMENT;
    copy_and_advance( context.current_tx.sender,                    &p, 32);
    copy_and_advance(&context.current_tx.fee,                       &p, 8);
    copy_and_advance(&context.current_tx.firstValid,                &p, 8);
    copy_and_advance(&context.current_tx.lastValid,                 &p, 8);
    copy_and_advance( context.current_tx.genesisID,                 &p, 32);
    copy_and_advance( context.current_tx.genesisHash,               &p, 32);
    copy_and_advance( context.current_tx.payload.payment.receiver,  &p, 32);
    copy_and_advance(&context.current_tx.payload.payment.amount,    &p, 8);
    copy_and_advance( context.current_tx.payload.payment.close,     &p, 32);

    context.state = 1;
  }
  else {
    if (dataLength > MAX_NOTE_FIELD_SIZE - context.current_tx.note_size) {
      THROW(0x6B00);
    }

    os_memmove(context.current_tx.note, dataBuffer, dataLength);
    context.current_tx.note_size += dataLength;
  }

  //is last block?
  if (p1 == 0) {
    THROW(0x9000);
  }

  *flags |= IO_ASYNCH_REPLY;

  context.state = 0;

  //show UI to confirm approval
  ui_show_tx_approval();
  return;
}

static void
handleSignKeyRegV3(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength,
                   volatile unsigned int *flags, volatile unsigned int *tx)
{
  if ((p1 != 0 && p1 != 1) || p2 != 0) {
    THROW(0x6B00);
  }

  if (context.state == 0) {
    uint8_t *p;

    if (dataLength != 32 + 8 + 8 + 8 + 32 + 32 + 32 + 32) {
      THROW(0x6B00);
    }

    p = dataBuffer;
    context.current_tx.type = KEYREG;
    copy_and_advance( context.current_tx.sender,                &p, 32);
    copy_and_advance(&context.current_tx.fee,                   &p, 8);
    copy_and_advance(&context.current_tx.firstValid,            &p, 8);
    copy_and_advance(&context.current_tx.lastValid,             &p, 8);
    copy_and_advance( context.current_tx.genesisID,             &p, 32);
    copy_and_advance( context.current_tx.genesisHash,           &p, 32);
    copy_and_advance( context.current_tx.payload.keyreg.votepk, &p, 32);
    copy_and_advance(&context.current_tx.payload.keyreg.vrfpk,  &p, 32);

    context.state = 1;
  }
  else {
    if (dataLength > MAX_NOTE_FIELD_SIZE - context.current_tx.note_size) {
      THROW(0x6B00);
    }

    os_memmove(context.current_tx.note, dataBuffer, dataLength);
    context.current_tx.note_size += dataLength;
  }

  //is last block?
  if (p1 == 0) {
    THROW(0x9000);
  }

  *flags |= IO_ASYNCH_REPLY;

  //show UI to confirm approval
  ui_show_tx_approval();
  return;
}

static void
copy_and_advance(void *dst, uint8_t **p, size_t len)
{
  os_memmove(dst, *p, len);
  *p += len;
  return;
}
