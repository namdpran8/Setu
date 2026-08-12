#pragma once

#include <stdint.h>
#include <stdlib.h>

#ifdef _MSC_VER
#define htodl(x) (x)
#define htods(x) (x)
#define dtohl(x) (x)
#define dtohs(x) (x)

#define htole32(x) (x)
#define le32toh(x) (x)
#define htole16(x) (x)
#define le16toh(x) (x)
#endif
