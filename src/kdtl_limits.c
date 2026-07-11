#include "kdtl_internal.h"

#include <string.h>

static bool window_valid(kdtl_timestamp start, kdtl_timestamp end) {
    return end > start;
}

static bool same_limit_key(const kdtl_limit_bucket *bucket, const char *operator_id,
                           const char *asset, kdtl_timestamp window_start,
                           kdtl_timestamp window_end) {
    return bucket->used && kdtl_id_eq(bucket->operator_id, operator_id) &&
           kdtl_id_eq(bucket->asset, asset) && bucket->window_start == window_start &&
           bucket->window_end == window_end;
}

static bool same_debt_key(const kdtl_debt_bucket *bucket, const char *operator_id,
                          const char *asset, kdtl_timestamp window_start,
                          kdtl_timestamp window_end) {
    return bucket->used && kdtl_id_eq(bucket->operator_id, operator_id) &&
           kdtl_id_eq(bucket->asset, asset) && bucket->window_start == window_start &&
           bucket->window_end == window_end;
}

static kdtl_limit_bucket *find_limit_bucket(kdtl_engine *engine, const char *operator_id,
                                            const char *asset, kdtl_timestamp window_start,
                                            kdtl_timestamp window_end, bool create,
                                            uint64_t policy_version) {
    kdtl_limit_bucket *bucket;

    for (size_t i = 0; i < engine->limit_bucket_count; i++) {
        bucket = &engine->limit_buckets[i];
        if (same_limit_key(bucket, operator_id, asset, window_start, window_end)) {
            return bucket;
        }
    }

    if (!create || engine->limit_bucket_count >= KDTL_MAX_BUCKETS) {
        return NULL;
    }

    bucket = &engine->limit_buckets[engine->limit_bucket_count++];
    memset(bucket, 0, sizeof(*bucket));
    bucket->used = true;
    (void)kdtl_copy_id(bucket->operator_id, sizeof(bucket->operator_id), operator_id);
    (void)kdtl_copy_id(bucket->asset, sizeof(bucket->asset), asset);
    bucket->window_start = window_start;
    bucket->window_end = window_end;
    bucket->policy_version = policy_version;
    return bucket;
}

static kdtl_debt_bucket *find_debt_bucket(kdtl_engine *engine, const char *operator_id,
                                          const char *asset, kdtl_timestamp window_start,
                                          kdtl_timestamp window_end, bool create,
                                          kdtl_timestamp now) {
    kdtl_debt_bucket *bucket;

    for (size_t i = 0; i < engine->debt_bucket_count; i++) {
        bucket = &engine->debt_buckets[i];
        if (same_debt_key(bucket, operator_id, asset, window_start, window_end)) {
            return bucket;
        }
    }

    if (!create || engine->debt_bucket_count >= KDTL_MAX_BUCKETS) {
        return NULL;
    }

    bucket = &engine->debt_buckets[engine->debt_bucket_count++];
    memset(bucket, 0, sizeof(*bucket));
    bucket->used = true;
    (void)kdtl_copy_id(bucket->operator_id, sizeof(bucket->operator_id), operator_id);
    (void)kdtl_copy_id(bucket->asset, sizeof(bucket->asset), asset);
    bucket->window_start = window_start;
    bucket->window_end = window_end;
    bucket->last_touched = now;
    return bucket;
}

static bool policy_valid(const kdtl_operator_policy *policy) {
    if (policy == NULL) {
        return false;
    }
    if (policy->operator_id[0] == '\0' || policy->asset[0] == '\0') {
        return false;
    }
    if (policy->max_active_debt == 0u || policy->max_window_debt == 0u) {
        return false;
    }
    if (policy->max_window_debt > policy->max_active_debt) {
        return false;
    }
    if (policy->max_open_positions == 0u || policy->min_margin_bps > 10000u) {
        return false;
    }
    return true;
}

kdtl_policy_record *kdtl_find_policy(kdtl_engine *engine, const char *operator_id,
                                     const char *asset) {
    if (engine == NULL || operator_id == NULL || asset == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < engine->policy_count; i++) {
        kdtl_policy_record *record = &engine->policies[i];
        if (record->used && kdtl_id_eq(record->policy.operator_id, operator_id) &&
            kdtl_id_eq(record->policy.asset, asset)) {
            return record;
        }
    }
    return NULL;
}

const kdtl_policy_record *kdtl_find_policy_const(const kdtl_engine *engine, const char *operator_id,
                                                 const char *asset) {
    if (engine == NULL || operator_id == NULL || asset == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < engine->policy_count; i++) {
        const kdtl_policy_record *record = &engine->policies[i];
        if (record->used && kdtl_id_eq(record->policy.operator_id, operator_id) &&
            kdtl_id_eq(record->policy.asset, asset)) {
            return record;
        }
    }
    return NULL;
}

kdtl_status kdtl_set_operator_policy(kdtl_engine *engine, const kdtl_operator_policy *policy) {
    kdtl_policy_record *record;

    if (engine == NULL || policy == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    kdtl_clear_error(engine);

    if (!policy_valid(policy)) {
        return kdtl_fail(engine, KDTL_ERR_ARGUMENT, "invalid operator policy");
    }
    if (kdtl_find_asset_index(engine, policy->asset) < 0) {
        return kdtl_fail(engine, KDTL_ERR_NOT_FOUND, "policy asset not registered");
    }

    record = kdtl_find_policy(engine, policy->operator_id, policy->asset);
    if (record == NULL) {
        if (engine->policy_count >= KDTL_MAX_POLICIES) {
            return kdtl_fail(engine, KDTL_ERR_CAPACITY, "operator policy capacity reached");
        }
        record = &engine->policies[engine->policy_count++];
        memset(record, 0, sizeof(*record));
        record->used = true;
        record->version = 1u;
    } else {
        record->version++;
    }

    record->policy = *policy;
    record->updated_at = 0u;
    return kdtl_record_event(engine, KDTL_EVENT_OPERATOR_LIMIT_SET, 0u, policy->operator_id,
                             policy->asset, 0u, policy->max_active_debt, "policy");
}

kdtl_status kdtl_update_operator_policy(kdtl_engine *engine, const kdtl_operator_policy *policy,
                                        kdtl_timestamp now) {
    kdtl_policy_record *record;

    if (engine == NULL || policy == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    kdtl_clear_error(engine);

    if (!policy_valid(policy)) {
        return kdtl_fail(engine, KDTL_ERR_ARGUMENT, "invalid operator policy");
    }
    record = kdtl_find_policy(engine, policy->operator_id, policy->asset);
    if (record == NULL) {
        return kdtl_set_operator_policy(engine, policy);
    }

    for (size_t i = 0; i < engine->limit_bucket_count; i++) {
        const kdtl_limit_bucket *bucket = &engine->limit_buckets[i];
        if (!bucket->used || !kdtl_id_eq(bucket->operator_id, policy->operator_id) ||
            !kdtl_id_eq(bucket->asset, policy->asset) || bucket->window_end <= now) {
            continue;
        }
        if (bucket->reserved > policy->max_window_debt) {
            return kdtl_fail(engine, KDTL_ERR_LIMIT, "active window exceeds requested policy");
        }
    }

    if (kdtl_debt_indexed_active(engine, policy->operator_id, policy->asset, now) >
        policy->max_active_debt) {
        return kdtl_fail(engine, KDTL_ERR_LIMIT, "active debt exceeds requested policy");
    }

    record->policy = *policy;
    record->version++;
    record->updated_at = now;
    return kdtl_record_event(engine, KDTL_EVENT_OPERATOR_LIMIT_SET, now, policy->operator_id,
                             policy->asset, 0u, policy->max_active_debt, "policy-update");
}

kdtl_status kdtl_limit_reserve(kdtl_engine *engine, const kdtl_operator_policy *policy,
                               kdtl_timestamp window_start, kdtl_timestamp window_end,
                               kdtl_amount amount) {
    kdtl_policy_record *record;
    kdtl_limit_bucket *bucket;
    kdtl_amount next;

    if (engine == NULL || policy == NULL || amount == 0u ||
        !window_valid(window_start, window_end)) {
        return KDTL_ERR_ARGUMENT;
    }

    record = kdtl_find_policy(engine, policy->operator_id, policy->asset);
    if (record == NULL) {
        return kdtl_fail(engine, KDTL_ERR_NOT_FOUND, "operator policy not registered");
    }

    bucket = find_limit_bucket(engine, policy->operator_id, policy->asset, window_start, window_end,
                               true, record->version);
    if (bucket == NULL) {
        return kdtl_fail(engine, KDTL_ERR_CAPACITY, "limit bucket capacity reached");
    }
    if (!kdtl_amount_add(bucket->reserved, amount, &next)) {
        return kdtl_fail(engine, KDTL_ERR_OVERFLOW, "limit bucket overflow");
    }
    if (next > policy->max_window_debt) {
        return kdtl_fail(engine, KDTL_ERR_LIMIT, "window limit exceeded");
    }
    if (bucket->positions + 1u > policy->max_open_positions) {
        return kdtl_fail(engine, KDTL_ERR_LIMIT, "window position count exceeded");
    }

    bucket->reserved = next;
    bucket->positions++;
    bucket->policy_version = record->version;
    return KDTL_OK;
}

kdtl_status kdtl_limit_release(kdtl_engine *engine, const char *operator_id, const char *asset,
                               kdtl_timestamp window_start, kdtl_timestamp window_end,
                               kdtl_amount amount) {
    kdtl_limit_bucket *bucket;

    if (engine == NULL || operator_id == NULL || asset == NULL || amount == 0u) {
        return KDTL_ERR_ARGUMENT;
    }

    bucket = find_limit_bucket(engine, operator_id, asset, window_start, window_end, false, 0u);
    if (bucket == NULL) {
        return KDTL_OK;
    }
    bucket->reserved = kdtl_amount_zero_floor_sub(bucket->reserved, amount);
    if (bucket->positions > 0u) {
        bucket->positions--;
    }
    return KDTL_OK;
}

kdtl_amount kdtl_limit_visible_reserved(const kdtl_engine *engine, const char *operator_id,
                                        const char *asset, kdtl_timestamp as_of) {
    kdtl_amount total = 0u;

    if (engine == NULL || operator_id == NULL || asset == NULL) {
        return 0u;
    }
    for (size_t i = 0; i < engine->limit_bucket_count; i++) {
        const kdtl_limit_bucket *bucket = &engine->limit_buckets[i];
        kdtl_amount next;
        if (!bucket->used || !kdtl_id_eq(bucket->operator_id, operator_id) ||
            !kdtl_id_eq(bucket->asset, asset) || bucket->window_end <= as_of) {
            continue;
        }
        if (kdtl_amount_add(total, bucket->reserved, &next)) {
            total = next;
        }
    }
    return total;
}

kdtl_amount kdtl_limit_window_reserved(const kdtl_engine *engine, const char *operator_id,
                                       const char *asset, kdtl_timestamp window_start,
                                       kdtl_timestamp window_end) {
    if (engine == NULL || operator_id == NULL || asset == NULL) {
        return 0u;
    }
    for (size_t i = 0; i < engine->limit_bucket_count; i++) {
        const kdtl_limit_bucket *bucket = &engine->limit_buckets[i];
        if (same_limit_key(bucket, operator_id, asset, window_start, window_end)) {
            return bucket->reserved;
        }
    }
    return 0u;
}

uint32_t kdtl_limit_bucket_count(const kdtl_engine *engine, const char *operator_id,
                                 const char *asset) {
    uint32_t count = 0u;

    if (engine == NULL || operator_id == NULL || asset == NULL) {
        return 0u;
    }
    for (size_t i = 0; i < engine->limit_bucket_count; i++) {
        const kdtl_limit_bucket *bucket = &engine->limit_buckets[i];
        if (bucket->used && kdtl_id_eq(bucket->operator_id, operator_id) &&
            kdtl_id_eq(bucket->asset, asset)) {
            count++;
        }
    }
    return count;
}

kdtl_status kdtl_debt_add(kdtl_engine *engine, const char *operator_id, const char *asset,
                          kdtl_timestamp window_start, kdtl_timestamp window_end,
                          kdtl_amount amount, kdtl_timestamp now) {
    kdtl_debt_bucket *bucket;
    kdtl_amount next;

    if (engine == NULL || operator_id == NULL || asset == NULL || amount == 0u ||
        !window_valid(window_start, window_end)) {
        return KDTL_ERR_ARGUMENT;
    }
    bucket = find_debt_bucket(engine, operator_id, asset, window_start, window_end, true, now);
    if (bucket == NULL) {
        return kdtl_fail(engine, KDTL_ERR_CAPACITY, "debt bucket capacity reached");
    }
    if (!kdtl_amount_add(bucket->pending, amount, &next)) {
        return kdtl_fail(engine, KDTL_ERR_OVERFLOW, "debt bucket overflow");
    }
    bucket->pending = next;
    bucket->positions++;
    bucket->last_touched = now;
    return KDTL_OK;
}

kdtl_status kdtl_debt_release(kdtl_engine *engine, const char *operator_id, const char *asset,
                              kdtl_timestamp window_start, kdtl_timestamp window_end,
                              kdtl_amount amount, kdtl_timestamp now) {
    kdtl_debt_bucket *bucket;
    kdtl_amount next;

    if (engine == NULL || operator_id == NULL || asset == NULL || amount == 0u) {
        return KDTL_ERR_ARGUMENT;
    }
    bucket = find_debt_bucket(engine, operator_id, asset, window_start, window_end, false, now);
    if (bucket == NULL) {
        return KDTL_OK;
    }
    bucket->pending = kdtl_amount_zero_floor_sub(bucket->pending, amount);
    if (kdtl_amount_add(bucket->released, amount, &next)) {
        bucket->released = next;
    }
    if (bucket->positions > 0u) {
        bucket->positions--;
    }
    bucket->last_touched = now;
    return KDTL_OK;
}

kdtl_status kdtl_debt_claim(kdtl_engine *engine, const char *operator_id, const char *asset,
                            kdtl_timestamp window_start, kdtl_timestamp window_end,
                            kdtl_amount amount, kdtl_timestamp now) {
    kdtl_debt_bucket *bucket;
    kdtl_amount next;

    if (engine == NULL || operator_id == NULL || asset == NULL || amount == 0u) {
        return KDTL_ERR_ARGUMENT;
    }
    bucket = find_debt_bucket(engine, operator_id, asset, window_start, window_end, false, now);
    if (bucket == NULL) {
        return KDTL_OK;
    }
    bucket->pending = kdtl_amount_zero_floor_sub(bucket->pending, amount);
    if (kdtl_amount_add(bucket->claimed, amount, &next)) {
        bucket->claimed = next;
    }
    if (bucket->positions > 0u) {
        bucket->positions--;
    }
    bucket->last_touched = now;
    return KDTL_OK;
}

kdtl_status kdtl_debt_mark_window(kdtl_engine *engine, const char *operator_id, const char *asset,
                                  kdtl_timestamp window_start, kdtl_timestamp window_end,
                                  kdtl_timestamp now) {
    kdtl_debt_bucket *bucket;

    if (engine == NULL || operator_id == NULL || asset == NULL ||
        !window_valid(window_start, window_end)) {
        return KDTL_ERR_ARGUMENT;
    }
    bucket = find_debt_bucket(engine, operator_id, asset, window_start, window_end, true, now);
    if (bucket == NULL) {
        return kdtl_fail(engine, KDTL_ERR_CAPACITY, "debt bucket capacity reached");
    }
    bucket->last_touched = now;
    return KDTL_OK;
}

kdtl_amount kdtl_debt_indexed_active(const kdtl_engine *engine, const char *operator_id,
                                     const char *asset, kdtl_timestamp as_of) {
    kdtl_amount total = 0u;

    if (engine == NULL || operator_id == NULL || asset == NULL) {
        return 0u;
    }
    for (size_t i = 0; i < engine->debt_bucket_count; i++) {
        const kdtl_debt_bucket *bucket = &engine->debt_buckets[i];
        kdtl_amount next;
        if (!bucket->used || !kdtl_id_eq(bucket->operator_id, operator_id) ||
            !kdtl_id_eq(bucket->asset, asset) || bucket->window_end <= as_of) {
            continue;
        }
        if (kdtl_amount_add(total, bucket->pending, &next)) {
            total = next;
        }
    }
    return total;
}

uint32_t kdtl_debt_bucket_count(const kdtl_engine *engine, const char *operator_id,
                                const char *asset) {
    uint32_t count = 0u;

    if (engine == NULL || operator_id == NULL || asset == NULL) {
        return 0u;
    }
    for (size_t i = 0; i < engine->debt_bucket_count; i++) {
        const kdtl_debt_bucket *bucket = &engine->debt_buckets[i];
        if (bucket->used && kdtl_id_eq(bucket->operator_id, operator_id) &&
            kdtl_id_eq(bucket->asset, asset)) {
            count++;
        }
    }
    return count;
}

kdtl_status kdtl_get_operator_risk(const kdtl_engine *engine, const char *operator_id,
                                   const char *asset, kdtl_timestamp as_of, kdtl_risk_view *out) {
    const kdtl_policy_record *policy;
    kdtl_amount indexed;

    if (engine == NULL || operator_id == NULL || asset == NULL || out == NULL) {
        return KDTL_ERR_ARGUMENT;
    }

    policy = kdtl_find_policy_const(engine, operator_id, asset);
    if (policy == NULL) {
        return KDTL_ERR_NOT_FOUND;
    }

    indexed = kdtl_debt_indexed_active(engine, operator_id, asset, as_of);
    memset(out, 0, sizeof(*out));
    (void)kdtl_copy_id(out->operator_id, sizeof(out->operator_id), operator_id);
    (void)kdtl_copy_id(out->asset, sizeof(out->asset), asset);
    out->as_of = as_of;
    out->max_active_debt = policy->policy.max_active_debt;
    out->max_window_debt = policy->policy.max_window_debt;
    out->indexed_debt = indexed;
    out->visible_reserved = kdtl_limit_visible_reserved(engine, operator_id, asset, as_of);
    out->claimable_open = kdtl_positions_claimable_open(engine, operator_id, asset, as_of);
    out->available_debt = kdtl_amount_zero_floor_sub(policy->policy.max_active_debt, indexed);
    out->open_positions = kdtl_positions_open_count(engine, operator_id, asset);
    out->limit_bucket_count = kdtl_limit_bucket_count(engine, operator_id, asset);
    out->debt_bucket_count = kdtl_debt_bucket_count(engine, operator_id, asset);
    out->within_policy = indexed <= policy->policy.max_active_debt;
    return KDTL_OK;
}
