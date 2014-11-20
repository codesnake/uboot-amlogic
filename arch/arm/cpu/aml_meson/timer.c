#include <common.h>
#include "common/firmware/timer.c"

#define TICKET_BASE_ON_USEC (1000000/CONFIG_SYS_HZ)
ulong      get_timer          (ulong base)
{
    return get_utimer(base*TICKET_BASE_ON_USEC)/TICKET_BASE_ON_USEC;
}
