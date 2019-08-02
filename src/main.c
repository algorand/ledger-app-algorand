#include "os.h"
#include "cx.h"
#include "os_io_seproxyhal.h"

#include "algo_keys.h"
#include "algo_ui.h"
#include "algo_tx.h"

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

#define CLA                 0x80
#define INS_SIGN_PAYMENT    0x01    // Deprecated, unused
#define INS_SIGN_KEYREG     0x02    // Deprecated, unused
#define INS_GET_PUBLIC_KEY  0x03
#define INS_SIGN_PAYMENT_V2 0x04
#define INS_SIGN_KEYREG_V2  0x05

struct txn current_txn;

void
txn_approve()
{
  unsigned int tx = 0;

  unsigned char msg[256];
  unsigned int msg_len;

  msg[0] = 'T';
  msg[1] = 'X';
  msg_len = 2 + tx_encode(&current_txn, msg+2, sizeof(msg)-2);

  PRINTF("Signing message: %.*h\n", msg_len, msg);

  cx_ecfp_private_key_t privateKey;
  algorand_private_key(&privateKey);

  int sig_len = cx_eddsa_sign(&privateKey,
                              0, CX_SHA512,
                              &msg[0], msg_len,
                              NULL, 0,
                              G_io_apdu_buffer,
                              6+2*(32+1), // Formerly from cx_compliance_141.c
                              NULL);

  tx = sig_len;
  G_io_apdu_buffer[tx++] = 0x90;
  G_io_apdu_buffer[tx++] = 0x00;

  // Send back the response, do not restart the event loop
  io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);

  // Display back the original UX
  ui_idle();
}

void
txn_deny()
{
  G_io_apdu_buffer[0] = 0x69;
  G_io_apdu_buffer[1] = 0x85;

  // Send back the response, do not restart the event loop
  io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);

  // Display back the original UX
  ui_idle();
}

static void
algorand_main(void)
{
  volatile unsigned int rx = 0;
  volatile unsigned int tx = 0;
  volatile unsigned int flags = 0;

  // next timer callback in 500 ms
  UX_CALLBACK_SET_INTERVAL(500);

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

        if (G_io_apdu_buffer[0] != CLA) {
          THROW(0x6E00);
        }

        switch (G_io_apdu_buffer[1]) {
        case INS_SIGN_PAYMENT_V2: {
          os_memset(&current_txn, 0, sizeof(current_txn));
          uint8_t *p = &G_io_apdu_buffer[2];

          current_txn.type = PAYMENT;

          os_memmove(current_txn.sender, p, 32);
          p += 32;

          os_memmove(&current_txn.fee, p, 8);
          p += 8;

          os_memmove(&current_txn.firstValid, p, 8);
          p += 8;

          os_memmove(&current_txn.lastValid, p, 8);
          p += 8;

          os_memmove(current_txn.genesisID, p, 32);
          p += 32;

          os_memmove(current_txn.genesisHash, p, 32);
          p += 32;

          os_memmove(current_txn.receiver, p, 32);
          p += 32;

          os_memmove(&current_txn.amount, p, 8);
          p += 8;

          os_memmove(current_txn.close, p, 32);
          p += 32;

          ui_txn();
          flags |= IO_ASYNCH_REPLY;
        } break;

        case INS_SIGN_KEYREG_V2: {
          os_memset(&current_txn, 0, sizeof(current_txn));
          uint8_t *p = &G_io_apdu_buffer[2];

          current_txn.type = KEYREG;

          os_memmove(current_txn.sender, p, 32);
          p += 32;

          os_memmove(&current_txn.fee, p, 8);
          p += 8;

          os_memmove(&current_txn.firstValid, p, 8);
          p += 8;

          os_memmove(&current_txn.lastValid, p, 8);
          p += 8;

          os_memmove(current_txn.genesisID, p, 32);
          p += 32;

          os_memmove(current_txn.genesisHash, p, 32);
          p += 32;

          os_memmove(current_txn.votepk, p, 32);
          p += 32;

          os_memmove(current_txn.vrfpk, p, 32);
          p += 32;

          ui_txn();
          flags |= IO_ASYNCH_REPLY;
        } break;

        case INS_GET_PUBLIC_KEY: {
          uint8_t publicKey[32];
          algorand_public_key(publicKey);
          os_memmove(G_io_apdu_buffer, publicKey, sizeof(publicKey));
          tx = sizeof(publicKey);
          THROW(0x9000);
        } break;

        case 0xFF: // return to dashboard
          goto return_to_dashboard;

        default:
          THROW(0x6D00);
          break;
        }
      }
      CATCH_OTHER(e) {
        switch (e & 0xF000) {
        case 0x6000:
        case 0x9000:
          sw = e;
          break;
        default:
          sw = 0x6800 | (e & 0x7FF);
          break;
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

return_to_dashboard:
  return;
}

void
io_seproxyhal_display(const bagl_element_t *element)
{
  io_seproxyhal_display_default((bagl_element_t *)element);
}

unsigned char
io_event(unsigned char channel)
{
  // nothing done with the event, throw an error on the transport layer if
  // needed

  // can't have more than one tag in the reply, not supported yet.
  switch (G_io_seproxyhal_spi_buffer[0]) {
  case SEPROXYHAL_TAG_FINGER_EVENT:
    UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
    break;

  case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT: // for Nano S
    UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
    break;

  case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
    UX_DISPLAYED_EVENT();
    break;

  case SEPROXYHAL_TAG_TICKER_EVENT:
    UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {
        // defaulty retrig very soon (will be overriden during
        // stepper_prepro)
        UX_CALLBACK_SET_INTERVAL(500);
        UX_REDISPLAY();
    });
    break;

  // unknown events are acknowledged
  default:
    UX_DEFAULT_EVENT();
    break;
  }

  // close the event if not done previously (by a display or whatever)
  if (!io_seproxyhal_spi_is_status_sent()) {
    io_seproxyhal_general_status();
  }

  // command has been processed, DO NOT reset the current APDU transport
  return 1;
}

unsigned short
io_exchange_al(unsigned char channel, unsigned short tx_len)
{
  switch (channel & ~(IO_FLAGS)) {
  case CHANNEL_KEYBOARD:
    break;

  // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
  case CHANNEL_SPI:
    if (tx_len) {
      io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

      if (channel & IO_RESET_AFTER_REPLIED) {
        reset();
      }
      return 0; // nothing received from the master so far (it's a tx transaction)
    } else {
      return io_seproxyhal_spi_recv(G_io_apdu_buffer, sizeof(G_io_apdu_buffer), 0);
    }

  default:
    THROW(INVALID_PARAMETER);
  }
  return 0;
}

__attribute__((section(".boot")))
int
main(void)
{
  // exit critical section
  __asm volatile("cpsie i");

  // What kind of horrible program loader fails to zero out the BSS?
  lineBuffer[0] = '\0';

  // ensure exception will work as planned
  os_boot();

  UX_INIT();
  UX_MENU_INIT();

  BEGIN_TRY {
    TRY {
      io_seproxyhal_init();

      USB_power(0);
      USB_power(1);

      ui_idle();

      algorand_main();
    }
    CATCH_OTHER(e) {
    }
    FINALLY {
    }
  }
  END_TRY;
}
