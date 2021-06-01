#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

#define EXCEPTION             1
#define INVALID_PARAMETER     2
#define EXCEPTION_OVERFLOW    3
#define EXCEPTION_SECURITY    4
#define INVALID_CRC           5
#define INVALID_CHECKSUM      6
#define INVALID_COUNTER       7
#define NOT_SUPPORTED         8
#define INVALID_STATE         9
#define TIMEOUT               10
#define EXCEPTION_PIC         11
#define EXCEPTION_APPEXIT     12
#define EXCEPTION_IO_OVERFLOW 13
#define EXCEPTION_IO_HEADER   14
#define EXCEPTION_IO_STATE    15
#define EXCEPTION_IO_RESET    16
#define EXCEPTION_CXPORT      17
#define EXCEPTION_SYSTEM      18
#define NOT_ENOUGH_SPACE      19

#define PRINTF(strbuf, ...) do { } while (0)

#define os_sched_exit(x) assert(false)
