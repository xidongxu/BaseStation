#include "cJSON.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <memory.h>

#if 0
int cJSON_hook_init(void) {
    cJSON_Hooks cJSON_hook;

    cJSON_hook.malloc_fn = (void *(*)(size_t sz))rt_malloc;
    cJSON_hook.free_fn = rt_free;

    cJSON_InitHooks(&cJSON_hook);

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(cJSON_hook_init);
#endif

extern "C" void *cJSON_port_malloc(size_t size)
{
    return malloc(size);
}
extern "C" void *cJSON_port_realloc(void *rmem, size_t newsize)
{
    return realloc(rmem, newsize);;
}

extern "C" void cJSON_port_free(void *rmem)
{
    free(rmem);
}
