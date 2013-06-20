
#ifdef CONFIG_DSUI

#include "edf_user_prog_dsui.h"
#include <kusp/dsui.h>
#include <stdlib.h>

static void edf_user_prog_dsui_register() __attribute__ ((constructor));

static void edf_user_prog_dsui_register()
{
    dsui_header_check(5, "edf_user_prog");
    struct datastream_ip *ip;
    
    for (ip = __start___edf_user_prog_datastream_ips;
            ip != __stop___edf_user_prog_datastream_ips; ip++) {
        dsui_register_ip(ip);
    }
}
#endif
