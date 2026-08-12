#pragma once
#include <stdint.h>

typedef int32_t status_t;

namespace android {

enum {
    OK                = 0,
    NO_ERROR          = 0,
    UNKNOWN_ERROR       = 0x80000000,
    NO_MEMORY           = -12,
    INVALID_OPERATION   = -38,
    BAD_VALUE           = -22,
    BAD_TYPE            = -2147483647,
    NAME_NOT_FOUND      = -2,
    PERMISSION_DENIED   = -1,
    NO_INIT             = -19,
    ALREADY_EXISTS      = -17,
    DEAD_OBJECT         = -32,
    FAILED_TRANSACTION  = -2147483646,
    BAD_INDEX           = -75,
    NOT_ENOUGH_DATA     = -61,
    WOULD_BLOCK         = -11,
    TIMED_OUT           = -110,
    UNKNOWN_TRANSACTION = -74,
    FDS_NOT_ALLOWED     = -2147483645,
    UNEXPECTED_NULL     = -2147483644,
};

} // namespace android
