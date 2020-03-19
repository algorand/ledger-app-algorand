#include "os.h"
#include "cx.h"
#include "os_io_seproxyhal.h"

#include "algo_keys.h"
#include "algo_ui.h"
#include "algo_tx.h"

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

#define OFFSET_CLA    0
#define OFFSET_INS    1
#define OFFSET_P1     2
#define OFFSET_P2     3
#define OFFSET_LC     4
#define OFFSET_CDATA  5

#define CLA           0x80

#define P1_FIRST 0x00
#define P1_MORE  0x80
#define P2_LAST  0x00
#define P2_MORE  0x80

#define INS_SIGN_PAYMENT    0x01    // Deprecated, unused
#define INS_SIGN_KEYREG     0x02    // Deprecated, unused
#define INS_GET_PUBLIC_KEY  0x03
#define INS_SIGN_PAYMENT_V2 0x04
#define INS_SIGN_KEYREG_V2  0x05
#define INS_SIGN_PAYMENT_V3 0x06
#define INS_SIGN_KEYREG_V3  0x07
#define INS_SIGN_MSGPACK    0x08

/* The transaction that we might ask the user to approve. */
struct txn current_txn;

/* A buffer for collecting msgpack-encoded transaction via APDUs,
 * as well as for msgpack-encoding transaction prior to signing.
 */
#if defined(TARGET_NANOX)
static uint8_t msgpack_buf[2048];
#else
static uint8_t msgpack_buf[1024];
#endif
static unsigned int msgpack_next_off;

void
txn_approve()
{
  unsigned int tx = 0;

  unsigned int msg_len;

  msgpack_buf[0] = 'T';
  msgpack_buf[1] = 'X';
  msg_len = 2 + tx_encode(&current_txn, msgpack_buf+2, sizeof(msgpack_buf)-2);

  PRINTF("Signing message: %.*h\n", msg_len, msgpack_buf);

  cx_ecfp_private_key_t privateKey;
  algorand_private_key(&privateKey);

  int sig_len = cx_eddsa_sign(&privateKey,
                              0, CX_SHA512,
                              &msgpack_buf[0], msg_len,
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
copy_and_advance(void *dst, uint8_t **p, size_t len)
{
  os_memmove(dst, *p, len);
  *p += len;
}

static void
algorand_main(void)
{
  volatile unsigned int rx = 0;
  volatile unsigned int tx = 0;
  volatile unsigned int flags = 0;

  algorand_key_derive();
  algorand_public_key(publicKey);

  msgpack_next_off = 0;

#if defined(TARGET_NANOS)
  // next timer callback in 500 ms
  UX_CALLBACK_SET_INTERVAL(500);
#endif

#if defined(TARGET_NANOX)
  // enable bluetooth on nano x
  G_io_app.plane_mode = os_setting_get(OS_SETTING_PLANEMODE, NULL, 0);
  BLE_power(0, NULL);
  BLE_power(1, "Nano X");
#endif

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

        if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
          THROW(0x6E00);
        }

        uint8_t ins = G_io_apdu_buffer[OFFSET_INS];
        switch (ins) {
        case INS_SIGN_PAYMENT_V2:
        case INS_SIGN_PAYMENT_V3:
        {
          os_memset(&current_txn, 0, sizeof(current_txn));
          uint8_t *p;
          if (ins == INS_SIGN_PAYMENT_V2) {
            p = &G_io_apdu_buffer[2];
          } else {
            p = &G_io_apdu_buffer[OFFSET_CDATA];
          }

          current_txn.type = PAYMENT;
          copy_and_advance( current_txn.sender,           &p, 32);
          copy_and_advance(&current_txn.fee,              &p, 8);
          copy_and_advance(&current_txn.firstValid,       &p, 8);
          copy_and_advance(&current_txn.lastValid,        &p, 8);
          copy_and_advance( current_txn.genesisID,        &p, 32);
          copy_and_advance( current_txn.genesisHash,      &p, 32);
          copy_and_advance( current_txn.payment.receiver, &p, 32);
          copy_and_advance(&current_txn.payment.amount,   &p, 8);
          copy_and_advance( current_txn.payment.close,    &p, 32);

          ui_txn();
          flags |= IO_ASYNCH_REPLY;
        } break;

        case INS_SIGN_KEYREG_V2:
        case INS_SIGN_KEYREG_V3:
        {
          os_memset(&current_txn, 0, sizeof(current_txn));
          uint8_t *p;
          if (ins == INS_SIGN_KEYREG_V2) {
            p = &G_io_apdu_buffer[2];
          } else {
            p = &G_io_apdu_buffer[OFFSET_CDATA];
          }

          current_txn.type = KEYREG;
          copy_and_advance( current_txn.sender,        &p, 32);
          copy_and_advance(&current_txn.fee,           &p, 8);
          copy_and_advance(&current_txn.firstValid,    &p, 8);
          copy_and_advance(&current_txn.lastValid,     &p, 8);
          copy_and_advance( current_txn.genesisID,     &p, 32);
          copy_and_advance( current_txn.genesisHash,   &p, 32);
          copy_and_advance( current_txn.keyreg.votepk, &p, 32);
          copy_and_advance( current_txn.keyreg.vrfpk,  &p, 32);

          ui_txn();
          flags |= IO_ASYNCH_REPLY;
        } break;

        case INS_SIGN_MSGPACK: {
          switch (G_io_apdu_buffer[OFFSET_P1]) {
          case P1_FIRST:
            msgpack_next_off = 0;
            break;
          case P1_MORE:
            break;
          default:
            THROW(0x6B00);
          }

          uint8_t lc = G_io_apdu_buffer[OFFSET_LC];
          if (msgpack_next_off + lc > sizeof(msgpack_buf)) {
            THROW(0x6700);
          }

          os_memmove(&msgpack_buf[msgpack_next_off], &G_io_apdu_buffer[OFFSET_CDATA], lc);
          msgpack_next_off += lc;

          switch (G_io_apdu_buffer[OFFSET_P2]) {
          case P2_LAST:
            {
              char *err = tx_decode(msgpack_buf, msgpack_next_off, &current_txn);
              if (err != NULL) {
                /* Return error by sending a response length longer than
                 * the usual ed25519 signature.
                 */
                int errlen = strlen(err);
                os_memset(G_io_apdu_buffer, 0, 65);
                os_memmove(&G_io_apdu_buffer[65], err, errlen);
                tx = 65 + errlen;
                THROW(0x9000);
              }

              ui_txn();
              flags |= IO_ASYNCH_REPLY;
            }
            break;
          case P2_MORE:
            THROW(0x9000);
          default:
            THROW(0x6B00);
          }
        } break;

        case INS_GET_PUBLIC_KEY: {
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
      CATCH(EXCEPTION_IO_RESET){
        THROW(EXCEPTION_IO_RESET);
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
#if defined(TARGET_NANOS)
        // defaulty retrig very soon (will be overriden during
        // stepper_prepro)
        UX_CALLBACK_SET_INTERVAL(500);
        UX_REDISPLAY();
#endif
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

  for (;;) {
    UX_INIT();

#if defined(TARGET_NANOS)
    UX_MENU_INIT();
#endif

    BEGIN_TRY {
      TRY {
        io_seproxyhal_init();

        USB_power(0);
        USB_power(1);

        ui_idle();

        algorand_main();
      }
      CATCH(EXCEPTION_IO_RESET) {
        // Reset IO and UX
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
}
