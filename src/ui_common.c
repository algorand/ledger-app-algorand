#include "ui_common.h"
#include "os.h"

//------------------------------------------------------------------------------

ui_strings_t ui_strings;
int ux_step;
int ux_steps_count;

//------------------------------------------------------------------------------

void
ui_common_init()
{
  os_memset(&ui_strings, 0, sizeof(ui_strings));
  ux_step = 0;
  ux_steps_count = 1;
  return;
}

// override point, but nothing more to do
void
io_seproxyhal_display(const bagl_element_t *element)
{
  io_seproxyhal_display_default((bagl_element_t *)element);
  return;
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
      UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {
#ifndef TARGET_NANOX
        if (UX_ALLOWED) {
          if (ux_steps_count > 0) {
            // prepare next screen
            ux_step = (ux_step + 1) % ux_steps_count;
            // redisplay screen
            UX_REDISPLAY();
          }
        }
#endif // TARGET_NANOX
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
      }
      return io_seproxyhal_spi_recv(G_io_apdu_buffer, sizeof(G_io_apdu_buffer), 0);

    default:
      THROW(INVALID_PARAMETER);
      break;
  }
  return 0;
}
