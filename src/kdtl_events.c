#include "kdtl_internal.h"

#include <string.h>

kdtl_status kdtl_record_event(kdtl_engine *engine, kdtl_event_kind kind, kdtl_timestamp at,
                              const char *operator_id, const char *asset,
                              kdtl_position_id position_id, kdtl_amount amount, const char *note) {
    kdtl_event_record *record;

    if (engine == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    if (engine->event_count >= KDTL_MAX_EVENTS) {
        return kdtl_fail(engine, KDTL_ERR_CAPACITY, "event journal capacity reached");
    }

    record = &engine->events[engine->event_count++];
    memset(record, 0, sizeof(*record));
    record->used = true;
    record->sequence = ++engine->next_event_sequence;
    record->kind = kind;
    record->at = at;
    record->position_id = position_id;
    record->amount = amount;

    if (operator_id != NULL && operator_id[0] != '\0') {
        (void)kdtl_copy_id(record->operator_id, sizeof(record->operator_id), operator_id);
    }
    if (asset != NULL && asset[0] != '\0') {
        (void)kdtl_copy_id(record->asset, sizeof(record->asset), asset);
    }
    if (note != NULL && note[0] != '\0') {
        (void)kdtl_copy_id(record->note, sizeof(record->note), note);
    }
    return KDTL_OK;
}

kdtl_status kdtl_get_event(const kdtl_engine *engine, uint64_t sequence, kdtl_event_view *out) {
    if (engine == NULL || out == NULL || sequence == 0u) {
        return KDTL_ERR_ARGUMENT;
    }

    for (size_t i = 0; i < engine->event_count; i++) {
        const kdtl_event_record *record = &engine->events[i];
        if (record->used && record->sequence == sequence) {
            memset(out, 0, sizeof(*out));
            out->sequence = record->sequence;
            out->kind = record->kind;
            out->at = record->at;
            out->position_id = record->position_id;
            out->amount = record->amount;
            (void)kdtl_copy_id(out->operator_id, sizeof(out->operator_id), record->operator_id);
            (void)kdtl_copy_id(out->asset, sizeof(out->asset), record->asset);
            (void)kdtl_copy_id(out->note, sizeof(out->note), record->note);
            return KDTL_OK;
        }
    }
    return KDTL_ERR_NOT_FOUND;
}
