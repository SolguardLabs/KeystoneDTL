#include "kdtl_internal.h"

#include <string.h>

static kdtl_status add_amount_checked(kdtl_amount *target, kdtl_amount amount) {
    kdtl_amount next;

    if (target == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    if (!kdtl_amount_add(*target, amount, &next)) {
        return KDTL_ERR_OVERFLOW;
    }
    *target = next;
    return KDTL_OK;
}

static uint32_t count_accounts_with_asset(const kdtl_engine *engine, int asset_index) {
    uint32_t count = 0u;

    for (size_t i = 0; i < engine->account_count; i++) {
        const kdtl_account_record *account = &engine->accounts[i];
        if (!account->used) {
            continue;
        }
        if (account->liquid[asset_index] > 0u || account->locked[asset_index] > 0u) {
            count++;
        }
    }
    return count;
}

static void count_positions_for_asset(const kdtl_engine *engine, const char *asset,
                                      uint32_t *open_positions, uint32_t *terminal_positions) {
    *open_positions = 0u;
    *terminal_positions = 0u;

    for (size_t i = 0; i < engine->position_count; i++) {
        const kdtl_position_record *position = &engine->positions[i];
        if (!position->used || !kdtl_id_eq(position->asset, asset)) {
            continue;
        }
        if (position->state == KDTL_POSITION_OPEN) {
            (*open_positions)++;
        } else {
            (*terminal_positions)++;
        }
    }
}

kdtl_status kdtl_reconcile_asset(const kdtl_engine *engine, const char *asset,
                                 kdtl_reconciliation_view *out) {
    int asset_index;
    kdtl_status status;

    if (engine == NULL || asset == NULL || out == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    asset_index = kdtl_find_asset_index(engine, asset);
    if (asset_index < 0) {
        return KDTL_ERR_NOT_FOUND;
    }

    memset(out, 0, sizeof(*out));
    (void)kdtl_copy_id(out->asset, sizeof(out->asset), asset);
    out->account_count = count_accounts_with_asset(engine, asset_index);
    out->event_count = (uint64_t)engine->event_count;
    count_positions_for_asset(engine, asset, &out->open_positions, &out->terminal_positions);

    for (size_t i = 0; i < engine->account_count; i++) {
        const kdtl_account_record *account = &engine->accounts[i];
        if (!account->used) {
            continue;
        }
        status = add_amount_checked(&out->liquid_total, account->liquid[asset_index]);
        if (status != KDTL_OK) {
            return status;
        }
        status = add_amount_checked(&out->locked_total, account->locked[asset_index]);
        if (status != KDTL_OK) {
            return status;
        }
    }

    status = add_amount_checked(&out->accounted_total, out->liquid_total);
    if (status != KDTL_OK) {
        return status;
    }
    status = add_amount_checked(&out->accounted_total, out->locked_total);
    if (status != KDTL_OK) {
        return status;
    }

    out->conserved = true;
    return KDTL_OK;
}
