
#ifdef CONFIG_DSUI

#include "cpu-bound-workloops_dsui.h"
#include <kusp/dsui.h>
#include <stdlib.h>

static void cpu-bound-workloops_dsui_register() __attribute__ ((constructor));

static void cpu-bound-workloops_dsui_register()
{
    dsui_header_check(5, "cpu-bound-workloops");
    struct datastream_ip *ip;
    
    for (ip = __start___cpu-bound-workloops_datastream_ips;
            ip != __stop___cpu-bound-workloops_datastream_ips; ip++) {
        dsui_register_ip(ip);
    }
}
#endif
