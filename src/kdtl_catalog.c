#include "kdtl_internal.h"

#include <string.h>

static kdtl_status add_checked(kdtl_amount *target, kdtl_amount value) {
    kdtl_amount next;

    if (target == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    if (!kdtl_amount_add(*target, value, &next)) {
        return KDTL_ERR_OVERFLOW;
    }
    *target = next;
    return KDTL_OK;
}

static uint32_t utilization_bps(kdtl_amount used, kdtl_amount limit) {
    if (limit == 0u || used == 0u) {
        return 0u;
    }
    if (used >= limit) {
        return 10000u;
    }
    if (used <= UINT64_MAX / 10000u) {
        return (uint32_t)((used * 10000u) / limit);
    }
    return (uint32_t)(used / ((limit / 10000u) + 1u));
}

static uint32_t asset_policy_count(const kdtl_engine *engine, const char *asset) {
    uint32_t count = 0u;

    for (size_t i = 0; i < engine->policy_count; i++) {
        const kdtl_policy_record *policy = &engine->policies[i];
        if (policy->used && kdtl_id_eq(policy->policy.asset, asset)) {
            count++;
        }
    }
    return count;
}

static kdtl_amount current_window_reserved(const kdtl_engine *engine, const char *operator_id,
                                           const char *asset, kdtl_timestamp as_of) {
    kdtl_amount total = 0u;

    for (size_t i = 0; i < engine->limit_bucket_count; i++) {
        const kdtl_limit_bucket *bucket = &engine->limit_buckets[i];
        kdtl_amount next;
        if (!bucket->used || !kdtl_id_eq(bucket->operator_id, operator_id) ||
            !kdtl_id_eq(bucket->asset, asset)) {
            continue;
        }
        if (bucket->window_start > as_of || bucket->window_end <= as_of) {
            continue;
        }
        if (kdtl_amount_add(total, bucket->reserved, &next)) {
            total = next;
        }
    }
    return total;
}

kdtl_status kdtl_get_asset(const kdtl_engine *engine, const char *asset, kdtl_asset_view *out) {
    int asset_index;

    if (engine == NULL || asset == NULL || out == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    asset_index = kdtl_find_asset_index(engine, asset);
    if (asset_index < 0) {
        return KDTL_ERR_NOT_FOUND;
    }

    memset(out, 0, sizeof(*out));
    (void)kdtl_copy_id(out->asset, sizeof(out->asset), asset);
    out->decimals = engine->assets[asset_index].decimals;
    out->liquidation_fee_bps = engine->assets[asset_index].liquidation_fee_bps;
    out->reserve_factor_bps = engine->assets[asset_index].reserve_factor_bps;
    out->policy_count = asset_policy_count(engine, asset);
    out->position_count = kdtl_positions_asset_count(engine, asset);
    out->open_positions = kdtl_positions_asset_open_count(engine, asset);

    for (size_t i = 0; i < engine->account_count; i++) {
        const kdtl_account_record *account = &engine->accounts[i];
        if (!account->used) {
            continue;
        }
        if (account->liquid[asset_index] > 0u || account->locked[asset_index] > 0u) {
            out->funded_accounts++;
        }
        if (add_checked(&out->liquid_total, account->liquid[asset_index]) != KDTL_OK ||
            add_checked(&out->locked_total, account->locked[asset_index]) != KDTL_OK) {
            return KDTL_ERR_OVERFLOW;
        }
    }
    return KDTL_OK;
}

kdtl_status kdtl_get_operator_policy(const kdtl_engine *engine, const char *operator_id,
                                     const char *asset, kdtl_timestamp as_of,
                                     kdtl_policy_view *out) {
    const kdtl_policy_record *record;

    if (engine == NULL || operator_id == NULL || asset == NULL || out == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    record = kdtl_find_policy_const(engine, operator_id, asset);
    if (record == NULL) {
        return KDTL_ERR_NOT_FOUND;
    }

    memset(out, 0, sizeof(*out));
    (void)kdtl_copy_id(out->operator_id, sizeof(out->operator_id), operator_id);
    (void)kdtl_copy_id(out->asset, sizeof(out->asset), asset);
    out->max_active_debt = record->policy.max_active_debt;
    out->max_window_debt = record->policy.max_window_debt;
    out->max_open_positions = record->policy.max_open_positions;
    out->min_margin_bps = record->policy.min_margin_bps;
    out->window_grace_seconds = record->policy.window_grace_seconds;
    out->version = record->version;
    out->updated_at = record->updated_at;
    out->active_debt = kdtl_debt_indexed_active(engine, operator_id, asset, as_of);
    out->window_reserved = current_window_reserved(engine, operator_id, asset, as_of);
    out->active_utilization_bps = utilization_bps(out->active_debt, out->max_active_debt);
    out->window_utilization_bps = utilization_bps(out->window_reserved, out->max_window_debt);
    out->open_positions = kdtl_positions_open_count(engine, operator_id, asset);
    return KDTL_OK;
}
