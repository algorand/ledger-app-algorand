#include "os.h"
#include "cx.h"
#include "os_io_seproxyhal.h"
#include "ux.h"

#include "algo_keys.h"
#include "algo_ui.h"
#include "algo_addr.h"
#include "algo_tx.h"
#include "command_handler.h"
#include "apdu_protocol_defines.h"

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];




/* The transaction that we might ask the user to approve. */
txn_t current_txn;


/* A buffer for collecting msgpack-encoded transaction via APDUs,
 * as well as for msgpack-encoding transaction prior to signing.
 */

#if defined(TARGET_NANOX)
#define TNX_BUFFER_SIZE 2048
#else
#define TNX_BUFFER_SIZE 900
#endif
static uint8_t msgpack_buf[TNX_BUFFER_SIZE];
static unsigned int msgpack_next_off;

static struct pubkey_s public_key;


void txn_approve()
{
  int sign_size = 0;
  unsigned int msg_len;

  msgpack_buf[0] = 'T';
  msgpack_buf[1] = 'X';
  msg_len = 2 + tx_encode(&current_txn, msgpack_buf+2, sizeof(msgpack_buf)-2);

  PRINTF("Signing message: %.*h\n", msg_len, msgpack_buf);
  PRINTF("Signing message: accountId:%d\n", current_txn.accountId);

  int error = algorand_sign_message(current_txn.accountId, &msgpack_buf[0], msg_len, G_io_apdu_buffer, &sign_size);
  if (error) {
    THROW(error);
  }

  G_io_apdu_buffer[sign_size++] = 0x90;
  G_io_apdu_buffer[sign_size++] = 0x00;


  // we've just signed the txn so we clear the static struct
  explicit_bzero(&current_txn, sizeof(current_txn));
  msgpack_next_off = 0;

  // Send back the response, do not restart the event loop
  io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, sign_size);

  // Display back the original UX
  ui_idle();
}

void address_approve()
{
  unsigned int tx = ALGORAND_PUBLIC_KEY_SIZE;
  memmove(G_io_apdu_buffer, public_key.data, ALGORAND_PUBLIC_KEY_SIZE);

  G_io_apdu_buffer[tx++] = 0x90;
  G_io_apdu_buffer[tx++] = 0x00;

  explicit_bzero(public_key.data, ALGORAND_PUBLIC_KEY_SIZE);
  // Send back the response, do not restart the event loop
  io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);

  // Display back the original UX
  ui_idle();
}

void
user_approval_denied()
{
  G_io_apdu_buffer[0] = 0x69;
  G_io_apdu_buffer[1] = 0x85;

  // Send back the response, do not restart the event loop
  io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);

  // Display back the original UX
  ui_idle();
}

static void copy_and_advance(void *dst, uint8_t **p, size_t len)
{
  memmove(dst, *p, len);
  *p += len;
}

void init_globals(){
  explicit_bzero(&current_txn, sizeof(current_txn));
  explicit_bzero(&public_key, sizeof(public_key));
  msgpack_next_off = 0;
}

static void handle_sign_payment(uint8_t ins)
{
  uint8_t *p;

  explicit_bzero(&current_txn, sizeof(current_txn));

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
}

static void handle_sign_keyreg(uint8_t ins)
{
  uint8_t *p;

  explicit_bzero(&current_txn, sizeof(current_txn));

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
}

static int handle_sign_msgpack(volatile unsigned int rx, volatile unsigned int *tx)
{
  char *error_msg;
  int error;

  error = parse_input_for_msgpack_command(G_io_apdu_buffer, rx, msgpack_buf, TNX_BUFFER_SIZE, &msgpack_next_off, &current_txn, &error_msg);
  if (error) {
    return error;
  }

  if (error_msg != NULL)
  {
    int errlen = strlen(error_msg);
    explicit_bzero(G_io_apdu_buffer, 65);
    memmove(&G_io_apdu_buffer[65], error_msg, errlen);
    *tx = 65 + errlen;
  } else {
    // we get here when all the data was received, otherwise an exception is thrown
    ui_txn();
  }

  return 0;
}

static int handle_get_public_key(volatile unsigned int rx, volatile unsigned int *tx)
{
  uint32_t account_id = 0;
  bool user_approval_required = G_io_apdu_buffer[OFFSET_P1] == P1_WITH_REQUEST_USER_APPROVAL;

  int error = parse_input_for_get_public_key_command(G_io_apdu_buffer, rx, &account_id);
  if (error) {
    return error;
  }
  /*
   * Push derived key to `G_io_apdu_buffer`
   * and return pushed buffer length.
   */
  error = fetch_public_key(account_id, &public_key);
  if (error) {
    return error;
  }

  if(user_approval_required){
    send_address_to_ui(&public_key);
    ui_address_approval();
  }
  else{
    memmove(G_io_apdu_buffer, public_key.data, ALGORAND_PUBLIC_KEY_SIZE);
    *tx = ALGORAND_PUBLIC_KEY_SIZE;
  }

  return error;
}

static void algorand_main(void)
{
  volatile unsigned int rx = 0;
  volatile unsigned int tx = 0;
  volatile unsigned int flags = 0;
  int error;


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

        PRINTF("New APDU received size: %d:\n%.*H\n",rx,  rx, G_io_apdu_buffer);


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
          handle_sign_payment(ins);
          flags |= IO_ASYNCH_REPLY;
          break;

        case INS_SIGN_KEYREG_V2:
        case INS_SIGN_KEYREG_V3:
          handle_sign_keyreg(ins);
          flags |= IO_ASYNCH_REPLY;
          break;

        case INS_SIGN_MSGPACK:
          error = handle_sign_msgpack(rx, &tx);
          if (error) {
            THROW(error);
          }
          if (tx != 0) {
            THROW(0x9000);
          } else {
            flags |= IO_ASYNCH_REPLY;
          }
          break;

        case INS_GET_PUBLIC_KEY:
          error = handle_get_public_key(rx, &tx);
          if (error) {
            THROW(error);
          }
          if (tx != 0) {
            THROW(0x9000);
          } else {
            flags |= IO_ASYNCH_REPLY;
          }
          break;

        case 0xFF: // return to dashboard
          CLOSE_TRY;
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

unsigned char io_event(unsigned char channel) {
  UNUSED(channel);
  // nothing done with the event, throw an error on the transport layer if
  // needed

  // can't have more than one tag in the reply, not supported yet.
  switch (G_io_seproxyhal_spi_buffer[0]) {
  case SEPROXYHAL_TAG_FINGER_EVENT:
    UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
    break;

  case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
    UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
    break;

  case SEPROXYHAL_TAG_STATUS_EVENT:
    if (G_io_apdu_media == IO_APDU_MEDIA_USB_HID && !(U4BE(G_io_seproxyhal_spi_buffer, 3) & SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
      THROW(EXCEPTION_IO_RESET);
    }
    // no break is intentional
  default:
    UX_DEFAULT_EVENT();
    break;

  case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
    UX_DISPLAYED_EVENT({});
    break;

  case SEPROXYHAL_TAG_TICKER_EVENT:
    UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer,
    {
    });
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
  text[0] = '\0';

  // ensure exception will work as planned
  os_boot();

  for (;;) {
    UX_INIT();

    BEGIN_TRY {
      TRY {
        io_seproxyhal_init();

#if defined(TARGET_NANOX)
        G_io_app.plane_mode = os_setting_get(OS_SETTING_PLANEMODE, NULL, 0);
#endif

        init_globals();

        USB_power(0);
        USB_power(1);

#if defined(TARGET_NANOX)
        BLE_power(0, NULL);
        BLE_power(1, "Nano X");
#endif

        ui_idle();
        algorand_main();
      }
      CATCH(EXCEPTION_IO_RESET) {
        // Reset IO and UX
        CLOSE_TRY;
        continue;
      }
      CATCH_ALL {
        CLOSE_TRY;
        break;
      }
      FINALLY {
      }
    }
    END_TRY;
  }

  os_sched_exit(-1);

  return 0;
}
