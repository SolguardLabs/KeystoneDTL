#include "kdtl_internal.h"

#include <string.h>

static bool windows_equal(kdtl_timestamp a_start, kdtl_timestamp a_end, kdtl_timestamp b_start,
                          kdtl_timestamp b_end) {
    return a_start == b_start && a_end == b_end;
}

static kdtl_status validate_terms(kdtl_engine *engine, const kdtl_route_terms *terms,
                                  const kdtl_policy_record *policy) {
    kdtl_amount projected;
    kdtl_amount indexed;

    if (terms->guarantee == 0u || terms->route_id[0] == '\0') {
        return kdtl_fail(engine, KDTL_ERR_ARGUMENT, "invalid route terms");
    }
    if (terms->window_end <= terms->window_start || terms->expires_at < terms->window_end) {
        return kdtl_fail(engine, KDTL_ERR_ARGUMENT, "invalid temporal window");
    }
    if (terms->margin_bps < policy->policy.min_margin_bps || terms->margin_bps > 10000u) {
        return kdtl_fail(engine, KDTL_ERR_LIMIT, "route margin below policy");
    }
    if (terms->guarantee > policy->policy.max_window_debt) {
        return kdtl_fail(engine, KDTL_ERR_LIMIT, "guarantee exceeds window policy");
    }

    indexed =
        kdtl_debt_indexed_active(engine, terms->operator_id, terms->asset, terms->opened_at);
    if (!kdtl_amount_add(indexed, terms->guarantee, &projected)) {
        return kdtl_fail(engine, KDTL_ERR_OVERFLOW, "risk projection overflow");
    }
    if (projected > policy->policy.max_active_debt) {
        return kdtl_fail(engine, KDTL_ERR_LIMIT, "active debt policy exceeded");
    }

    for (size_t i = 0; i < engine->position_count; i++) {
        const kdtl_position_record *position = &engine->positions[i];
        if (!position->used) {
            continue;
        }
        if (kdtl_id_eq(position->payer, terms->payer) && position->nonce == terms->nonce) {
            return kdtl_fail(engine, KDTL_ERR_EXISTS, "payer nonce already used");
        }
        if (kdtl_id_eq(position->route_id, terms->route_id)) {
            return kdtl_fail(engine, KDTL_ERR_EXISTS, "route already registered");
        }
    }

    return KDTL_OK;
}

kdtl_position_record *kdtl_find_position(kdtl_engine *engine, kdtl_position_id id) {
    if (engine == NULL || id == 0u) {
        return NULL;
    }
    for (size_t i = 0; i < engine->position_count; i++) {
        if (engine->positions[i].used && engine->positions[i].id == id) {
            return &engine->positions[i];
        }
    }
    return NULL;
}

const kdtl_position_record *kdtl_find_position_const(const kdtl_engine *engine,
                                                     kdtl_position_id id) {
    if (engine == NULL || id == 0u) {
        return NULL;
    }
    for (size_t i = 0; i < engine->position_count; i++) {
        if (engine->positions[i].used && engine->positions[i].id == id) {
            return &engine->positions[i];
        }
    }
    return NULL;
}

kdtl_status kdtl_open_position(kdtl_engine *engine, const kdtl_route_terms *terms,
                               kdtl_position_id *out_id) {
    const kdtl_policy_record *policy;
    kdtl_position_record *position;
    kdtl_status status;
    bool premium_debited = false;

    if (engine == NULL || terms == NULL || out_id == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    kdtl_clear_error(engine);

    *out_id = 0u;
    if (engine->position_count >= KDTL_MAX_POSITIONS) {
        return kdtl_fail(engine, KDTL_ERR_CAPACITY, "position capacity reached");
    }
    if (kdtl_find_account_index(engine, terms->payer) < 0 ||
        kdtl_find_account_index(engine, terms->beneficiary) < 0 ||
        kdtl_find_account_index(engine, terms->operator_id) < 0) {
        return kdtl_fail(engine, KDTL_ERR_NOT_FOUND, "route account not registered");
    }
    if (kdtl_find_asset_index(engine, terms->asset) < 0) {
        return kdtl_fail(engine, KDTL_ERR_NOT_FOUND, "route asset not registered");
    }
    policy = kdtl_find_policy_const(engine, terms->operator_id, terms->asset);
    if (policy == NULL) {
        return kdtl_fail(engine, KDTL_ERR_NOT_FOUND, "operator policy not registered");
    }

    status = validate_terms(engine, terms, policy);
    if (status != KDTL_OK) {
        return status;
    }

    status = kdtl_limit_reserve(engine, &policy->policy, terms->window_start, terms->window_end,
                                terms->guarantee);
    if (status != KDTL_OK) {
        return status;
    }
    status = kdtl_debt_add(engine, terms->operator_id, terms->asset, terms->window_start,
                           terms->window_end, terms->guarantee, terms->opened_at);
    if (status != KDTL_OK) {
        (void)kdtl_limit_release(engine, terms->operator_id, terms->asset, terms->window_start,
                                 terms->window_end, terms->guarantee);
        return status;
    }
    status = kdtl_lock_account(engine, terms->payer, terms->asset, terms->guarantee);
    if (status != KDTL_OK) {
        (void)kdtl_debt_release(engine, terms->operator_id, terms->asset, terms->window_start,
                                terms->window_end, terms->guarantee, terms->opened_at);
        (void)kdtl_limit_release(engine, terms->operator_id, terms->asset, terms->window_start,
                                 terms->window_end, terms->guarantee);
        return status;
    }

    if (terms->premium > 0u) {
        status = kdtl_debit_liquid(engine, terms->payer, terms->asset, terms->premium);
        if (status != KDTL_OK) {
            (void)kdtl_unlock_account(engine, terms->payer, terms->asset, terms->guarantee);
            (void)kdtl_debt_release(engine, terms->operator_id, terms->asset, terms->window_start,
                                    terms->window_end, terms->guarantee, terms->opened_at);
            (void)kdtl_limit_release(engine, terms->operator_id, terms->asset, terms->window_start,
                                     terms->window_end, terms->guarantee);
            return status;
        }
        premium_debited = true;
        status = kdtl_credit_liquid(engine, terms->operator_id, terms->asset, terms->premium);
        if (status != KDTL_OK) {
            if (premium_debited) {
                (void)kdtl_credit_liquid(engine, terms->payer, terms->asset, terms->premium);
            }
            (void)kdtl_unlock_account(engine, terms->payer, terms->asset, terms->guarantee);
            (void)kdtl_debt_release(engine, terms->operator_id, terms->asset, terms->window_start,
                                    terms->window_end, terms->guarantee, terms->opened_at);
            (void)kdtl_limit_release(engine, terms->operator_id, terms->asset, terms->window_start,
                                     terms->window_end, terms->guarantee);
            return status;
        }
    }

    position = &engine->positions[engine->position_count++];
    memset(position, 0, sizeof(*position));
    position->used = true;
    position->id = engine->next_position_id++;
    position->state = KDTL_POSITION_OPEN;
    (void)kdtl_copy_id(position->payer, sizeof(position->payer), terms->payer);
    (void)kdtl_copy_id(position->beneficiary, sizeof(position->beneficiary), terms->beneficiary);
    (void)kdtl_copy_id(position->operator_id, sizeof(position->operator_id), terms->operator_id);
    (void)kdtl_copy_id(position->asset, sizeof(position->asset), terms->asset);
    (void)kdtl_copy_id(position->route_id, sizeof(position->route_id), terms->route_id);
    position->guarantee = terms->guarantee;
    position->locked = terms->guarantee;
    position->premium = terms->premium;
    position->margin_bps = terms->margin_bps;
    position->opened_at = terms->opened_at;
    position->updated_at = terms->opened_at;
    position->window_start = terms->window_start;
    position->window_end = terms->window_end;
    position->expires_at = terms->expires_at;
    position->nonce = terms->nonce;
    position->terms_hash = kdtl_hash_terms(terms);

    *out_id = position->id;
    return kdtl_record_event(engine, KDTL_EVENT_POSITION_OPENED, terms->opened_at,
                             terms->operator_id, terms->asset, position->id, terms->guarantee,
                             "open");
}

kdtl_status kdtl_move_position(kdtl_engine *engine, const kdtl_move_request *request) {
    kdtl_position_record *position;
    const kdtl_policy_record *policy;
    kdtl_timestamp old_start;
    kdtl_timestamp old_end;
    const char *note;
    kdtl_status status;

    if (engine == NULL || request == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    kdtl_clear_error(engine);

    position = kdtl_find_position(engine, request->position_id);
    if (position == NULL) {
        return kdtl_fail(engine, KDTL_ERR_NOT_FOUND, "position not found");
    }
    if (position->state != KDTL_POSITION_OPEN) {
        return kdtl_fail(engine, KDTL_ERR_STATE, "position is terminal");
    }
    if (request->new_window_end <= request->new_window_start ||
        request->new_window_end > position->expires_at) {
        return kdtl_fail(engine, KDTL_ERR_ARGUMENT, "invalid requested window");
    }
    if (request->now > position->expires_at) {
        return kdtl_fail(engine, KDTL_ERR_STATE, "position expired");
    }

    if (windows_equal(position->window_start, position->window_end, request->new_window_start,
                      request->new_window_end)) {
        position->updated_at = request->now;
        return kdtl_record_event(engine, KDTL_EVENT_POSITION_REWINDOWED, request->now,
                                 position->operator_id, position->asset, position->id,
                                 position->locked, "rewindow");
    }

    policy = kdtl_find_policy_const(engine, position->operator_id, position->asset);
    if (policy == NULL) {
        return kdtl_fail(engine, KDTL_ERR_NOT_FOUND, "operator policy not registered");
    }

    old_start = position->window_start;
    old_end = position->window_end;
    status = kdtl_limit_reserve(engine, &policy->policy, request->new_window_start,
                                request->new_window_end, position->locked);
    if (status != KDTL_OK) {
        return status;
    }
    status = kdtl_limit_release(engine, position->operator_id, position->asset, old_start, old_end,
                                position->locked);
    if (status != KDTL_OK) {
        return status;
    }

    position->window_start = request->new_window_start;
    position->window_end = request->new_window_end;
    position->updated_at = request->now;

    status = kdtl_debt_mark_window(engine, position->operator_id, position->asset,
                                   position->window_start, position->window_end, request->now);
    if (status != KDTL_OK) {
        return status;
    }

    note = request->reason[0] == '\0' ? "rewindow" : request->reason;
    return kdtl_record_event(engine, KDTL_EVENT_POSITION_REWINDOWED, request->now,
                             position->operator_id, position->asset, position->id,
                             position->locked, note);
}

kdtl_status kdtl_cancel_position(kdtl_engine *engine, const kdtl_cancel_request *request) {
    kdtl_position_record *position;
    bool same_refund;
    kdtl_status status;

    if (engine == NULL || request == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    kdtl_clear_error(engine);

    position = kdtl_find_position(engine, request->position_id);
    if (position == NULL) {
        return kdtl_fail(engine, KDTL_ERR_NOT_FOUND, "position not found");
    }
    if (position->state != KDTL_POSITION_OPEN) {
        return kdtl_fail(engine, KDTL_ERR_STATE, "position is terminal");
    }
    if (!kdtl_id_eq(request->requested_by, position->payer) &&
        !kdtl_id_eq(request->requested_by, position->beneficiary)) {
        return kdtl_fail(engine, KDTL_ERR_STATE, "cancel request not authorized");
    }
    if (request->now >= position->window_start) {
        return kdtl_fail(engine, KDTL_ERR_STATE, "cancel window closed");
    }
    if (kdtl_find_account_index(engine, request->refund_to) < 0) {
        return kdtl_fail(engine, KDTL_ERR_NOT_FOUND, "refund account not registered");
    }

    same_refund = kdtl_id_eq(request->refund_to, position->payer);
    if (same_refund) {
        status = kdtl_unlock_account(engine, position->payer, position->asset, position->locked);
    } else {
        status = kdtl_debit_locked(engine, position->payer, position->asset, position->locked);
        if (status == KDTL_OK) {
            status = kdtl_credit_liquid(engine, request->refund_to, position->asset,
                                        position->locked);
        }
    }
    if (status != KDTL_OK) {
        return status;
    }

    position->refunded = position->locked;
    position->locked = 0u;
    position->state = KDTL_POSITION_CANCELLED;
    position->updated_at = request->now;
    (void)kdtl_limit_release(engine, position->operator_id, position->asset,
                             position->window_start, position->window_end, position->guarantee);
    (void)kdtl_debt_release(engine, position->operator_id, position->asset,
                            position->window_start, position->window_end, position->guarantee,
                            request->now);

    return kdtl_record_event(engine, KDTL_EVENT_POSITION_CANCELLED, request->now,
                             position->operator_id, position->asset, position->id,
                             position->refunded, "cancel");
}

kdtl_status kdtl_expire_positions(kdtl_engine *engine, kdtl_timestamp now, uint32_t *expired) {
    uint32_t count = 0u;

    if (engine == NULL || expired == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    kdtl_clear_error(engine);

    for (size_t i = 0; i < engine->position_count; i++) {
        kdtl_position_record *position = &engine->positions[i];
        if (!position->used || position->state != KDTL_POSITION_OPEN || position->expires_at > now) {
            continue;
        }

        if (position->locked > 0u) {
            kdtl_status status =
                kdtl_unlock_account(engine, position->payer, position->asset, position->locked);
            if (status != KDTL_OK) {
                return status;
            }
        }
        position->refunded = position->locked;
        position->locked = 0u;
        position->state = KDTL_POSITION_EXPIRED;
        position->updated_at = now;
        (void)kdtl_limit_release(engine, position->operator_id, position->asset,
                                 position->window_start, position->window_end,
                                 position->guarantee);
        (void)kdtl_debt_release(engine, position->operator_id, position->asset,
                                position->window_start, position->window_end,
                                position->guarantee, now);
        (void)kdtl_record_event(engine, KDTL_EVENT_POSITION_EXPIRED, now, position->operator_id,
                                position->asset, position->id, position->refunded, "expire");
        count++;
    }
    *expired = count;
    return KDTL_OK;
}

kdtl_amount kdtl_positions_claimable_open(const kdtl_engine *engine, const char *operator_id,
                                          const char *asset, kdtl_timestamp as_of) {
    kdtl_amount total = 0u;

    if (engine == NULL || operator_id == NULL || asset == NULL) {
        return 0u;
    }
    for (size_t i = 0; i < engine->position_count; i++) {
        const kdtl_position_record *position = &engine->positions[i];
        kdtl_amount next;
        if (!position->used || position->state != KDTL_POSITION_OPEN ||
            !kdtl_id_eq(position->operator_id, operator_id) || !kdtl_id_eq(position->asset, asset) ||
            position->expires_at <= as_of) {
            continue;
        }
        if (kdtl_amount_add(total, position->locked, &next)) {
            total = next;
        }
    }
    return total;
}

uint32_t kdtl_positions_open_count(const kdtl_engine *engine, const char *operator_id,
                                   const char *asset) {
    uint32_t count = 0u;

    if (engine == NULL || operator_id == NULL || asset == NULL) {
        return 0u;
    }
    for (size_t i = 0; i < engine->position_count; i++) {
        const kdtl_position_record *position = &engine->positions[i];
        if (position->used && position->state == KDTL_POSITION_OPEN &&
            kdtl_id_eq(position->operator_id, operator_id) && kdtl_id_eq(position->asset, asset)) {
            count++;
        }
    }
    return count;
}

uint32_t kdtl_positions_asset_count(const kdtl_engine *engine, const char *asset) {
    uint32_t count = 0u;

    if (engine == NULL || asset == NULL) {
        return 0u;
    }
    for (size_t i = 0; i < engine->position_count; i++) {
        const kdtl_position_record *position = &engine->positions[i];
        if (position->used && kdtl_id_eq(position->asset, asset)) {
            count++;
        }
    }
    return count;
}

uint32_t kdtl_positions_asset_open_count(const kdtl_engine *engine, const char *asset) {
    uint32_t count = 0u;

    if (engine == NULL || asset == NULL) {
        return 0u;
    }
    for (size_t i = 0; i < engine->position_count; i++) {
        const kdtl_position_record *position = &engine->positions[i];
        if (position->used && position->state == KDTL_POSITION_OPEN &&
            kdtl_id_eq(position->asset, asset)) {
            count++;
        }
    }
    return count;
}

void kdtl_fill_position_view(const kdtl_position_record *record, kdtl_position_view *out) {
    uint64_t digest;

    if (record == NULL || out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->id = record->id;
    out->state = record->state;
    (void)kdtl_copy_id(out->payer, sizeof(out->payer), record->payer);
    (void)kdtl_copy_id(out->beneficiary, sizeof(out->beneficiary), record->beneficiary);
    (void)kdtl_copy_id(out->operator_id, sizeof(out->operator_id), record->operator_id);
    (void)kdtl_copy_id(out->asset, sizeof(out->asset), record->asset);
    (void)kdtl_copy_id(out->route_id, sizeof(out->route_id), record->route_id);
    out->guarantee = record->guarantee;
    out->locked = record->locked;
    out->paid_out = record->paid_out;
    out->refunded = record->refunded;
    out->premium = record->premium;
    out->margin_bps = record->margin_bps;
    out->opened_at = record->opened_at;
    out->updated_at = record->updated_at;
    out->window_start = record->window_start;
    out->window_end = record->window_end;
    out->expires_at = record->expires_at;
    digest = kdtl_hash_position_view(out) ^ record->terms_hash;
    kdtl_digest_hex(digest, out->digest);
}

kdtl_status kdtl_get_position(const kdtl_engine *engine, kdtl_position_id id,
                              kdtl_position_view *out) {
    const kdtl_position_record *record;

    if (engine == NULL || out == NULL || id == 0u) {
        return KDTL_ERR_ARGUMENT;
    }
    record = kdtl_find_position_const(engine, id);
    if (record == NULL) {
        return KDTL_ERR_NOT_FOUND;
    }
    kdtl_fill_position_view(record, out);
    return KDTL_OK;
}
