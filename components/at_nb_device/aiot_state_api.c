#include "core_stdinc.h"
#include "aiot_state_api.h"

aiot_state_logcb_t g_logcb_handler = NULL;

int32_t aiot_state_set_logcb(aiot_state_logcb_t handler)
{
    g_logcb_handler = handler;
    return 0;
}

