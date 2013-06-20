
#ifdef CONFIG_DSUI

#include "edfup0_dsui.h"
#include <kusp/dsui.h>
#include <stdlib.h>

static void edfup0_dsui_register() __attribute__ ((constructor));

static void edfup0_dsui_register()
{
    dsui_header_check(5, "edfup0");
    struct datastream_ip *ip;
    
    for (ip = __start___edfup0_datastream_ips;
            ip != __stop___edfup0_datastream_ips; ip++) {
        dsui_register_ip(ip);
    }
}
#endif
