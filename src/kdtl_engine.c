#include "kdtl_internal.h"

#include <stdlib.h>
#include <string.h>

kdtl_engine *kdtl_engine_create(void) {
    kdtl_engine *engine = (kdtl_engine *)calloc(1u, sizeof(kdtl_engine));
    if (engine != NULL) {
        engine->next_position_id = 1u;
        engine->next_event_sequence = 0u;
    }
    return engine;
}

void kdtl_engine_destroy(kdtl_engine *engine) {
    free(engine);
}

void kdtl_engine_reset(kdtl_engine *engine) {
    if (engine == NULL) {
        return;
    }
    memset(engine, 0, sizeof(*engine));
    engine->next_position_id = 1u;
    engine->next_event_sequence = 0u;
}

const char *kdtl_last_error(const kdtl_engine *engine) {
    if (engine == NULL || engine->last_error[0] == '\0') {
        return "";
    }
    return engine->last_error;
}
