
#ifdef CONFIG_DSUI

#include "edfup1_dsui.h"
#include <kusp/dsui.h>
#include <stdlib.h>

static void edfup1_dsui_register() __attribute__ ((constructor));

static void edfup1_dsui_register()
{
    dsui_header_check(5, "edfup1");
    struct datastream_ip *ip;
    
    for (ip = __start___edfup1_datastream_ips;
            ip != __stop___edfup1_datastream_ips; ip++) {
        dsui_register_ip(ip);
    }
}
#endif
