ifeq ($(BOLOS_SDK),)
$(error BOLOS_SDK is not set)
endif
include $(BOLOS_SDK)/Makefile.defines
include $(BOLOS_SDK)/Makefile.glyphs

# Main app configuration

APPNAME = "Algorand"
APPVERSION = 1.0.8
APP_LOAD_PARAMS = --appFlags 0x250 $(COMMON_LOAD_PARAMS)
APP_LOAD_PARAMS += --path "44'/283'"

# Choose appropriate icon

ifeq ($(TARGET_NAME),TARGET_NANOS)
ICONNAME=icons/app_logo_s.gif
else ifeq ($(TARGET_NAME),TARGET_NANOX)
ICONNAME=icons/app_logo_x.gif
endif

# Build configuration

APP_SOURCE_PATH += src
SDK_SOURCE_PATH += lib_stusb lib_stusb_impl

DEFINES += APPVERSION=\"$(APPVERSION)\"

DEFINES += OS_IO_SEPROXYHAL
DEFINES += HAVE_BAGL HAVE_SPRINTF
DEFINES += HAVE_BOLOS_APP_STACK_CANARY

ifeq ($(TARGET_NAME),TARGET_NANOS)
DEFINES += HAVE_UX_LEGACY
DEFINES += IO_SEPROXYHAL_BUFFER_SIZE_B=128
else ifeq ($(TARGET_NAME),TARGET_NANOX)
DEFINES += IO_SEPROXYHAL_BUFFER_SIZE_B=300
DEFINES += HAVE_BLE BLE_COMMAND_TIMEOUT_MS=2000
DEFINES += HAVE_BLE_APDU

DEFINES += HAVE_GLO096
DEFINES += HAVE_BAGL BAGL_WIDTH=128 BAGL_HEIGHT=64
DEFINES += HAVE_BAGL_ELLIPSIS
DEFINES += HAVE_BAGL_FONT_OPEN_SANS_REGULAR_11PX
DEFINES += HAVE_BAGL_FONT_OPEN_SANS_EXTRABOLD_11PX
DEFINES += HAVE_BAGL_FONT_OPEN_SANS_LIGHT_16PX

DEFINES += HAVE_UX_FLOW

SDK_SOURCE_PATH  += lib_blewbxx lib_blewbxx_impl
SDK_SOURCE_PATH  += lib_ux
else
$(error unknown device TARGET_NAME)
endif

# Enabling debug PRINTF
DEBUG = 0
ifneq ($(DEBUG),0)

        ifeq ($(TARGET_NAME),TARGET_NANOX)
                DEFINES   += HAVE_PRINTF PRINTF=mcu_usb_printf
        else
                DEFINES   += HAVE_PRINTF PRINTF=screen_printf
        endif
else
        DEFINES   += PRINTF\(...\)=
endif

DEFINES += HAVE_IO_USB HAVE_L4_USBLIB IO_USB_MAX_ENDPOINTS=4 IO_HID_EP_LENGTH=64 HAVE_USB_APDU

#WEBUSB_URL = ledger-app.algorand.com
#DEFINES += HAVE_WEBUSB WEBUSB_URL_SIZE_B=$(shell echo -n $(WEBUSB_URL) | wc -c) WEBUSB_URL=$(shell echo -n $(WEBUSB_URL) | sed -e "s/./\\\'\0\\\',/g")
DEFINES   += HAVE_WEBUSB WEBUSB_URL_SIZE_B=0 WEBUSB_URL=""

DEFINES += USB_SEGMENT_SIZE=64 BLE_SEGMENT_SIZE=32

# Compiler, assembler, and linker

ifneq ($(BOLOS_ENV),)
$(info BOLOS_ENV=$(BOLOS_ENV))
CLANGPATH := $(BOLOS_ENV)/clang-arm-fropi/bin/
GCCPATH := $(BOLOS_ENV)/gcc-arm-none-eabi-5_3-2016q1/bin/
else
$(info BOLOS_ENV is not set: falling back to CLANGPATH and GCCPATH)
endif

ifeq ($(CLANGPATH),)
$(info CLANGPATH is not set: clang will be used from PATH)
endif

ifeq ($(GCCPATH),)
$(info GCCPATH is not set: arm-none-eabi-* will be used from PATH)
endif

CC := $(CLANGPATH)clang
CFLAGS += -O3 -Oz

AS := $(GCCPATH)arm-none-eabi-gcc
AFLAGS +=

LD := $(GCCPATH)arm-none-eabi-gcc
LDFLAGS += -O3 -Oz
LDLIBS += -lm -lgcc -lc

# Main rules

all: default

load: all
	python -m ledgerblue.loadApp $(APP_LOAD_PARAMS)

delete:
	python -m ledgerblue.deleteApp $(COMMON_DELETE_PARAMS)

# Import generic rules from the SDK

include $(BOLOS_SDK)/Makefile.rules


listvariants:
	@echo VARIANTS COIN algorand
