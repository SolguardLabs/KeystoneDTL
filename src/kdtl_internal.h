#ifndef KDTL_INTERNAL_H
#define KDTL_INTERNAL_H

#include "keystone_dtl.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#define KDTL_MAX_ACCOUNTS 64u
#define KDTL_MAX_ASSETS 16u
#define KDTL_MAX_POLICIES 48u
#define KDTL_MAX_POSITIONS 256u
#define KDTL_MAX_BUCKETS 512u
#define KDTL_MAX_EVENTS 512u
#define KDTL_LAST_ERROR_LEN 256u

typedef struct kdtl_account_record {
    bool used;
    char account[KDTL_ACCOUNT_ID_LEN];
    kdtl_amount liquid[KDTL_MAX_ASSETS];
    kdtl_amount locked[KDTL_MAX_ASSETS];
} kdtl_account_record;

typedef struct kdtl_asset_record {
    bool used;
    char asset[KDTL_ASSET_ID_LEN];
    uint8_t decimals;
    uint16_t liquidation_fee_bps;
    uint16_t reserve_factor_bps;
} kdtl_asset_record;

typedef struct kdtl_policy_record {
    bool used;
    kdtl_operator_policy policy;
    uint64_t version;
    kdtl_timestamp updated_at;
} kdtl_policy_record;

typedef struct kdtl_limit_bucket {
    bool used;
    char operator_id[KDTL_OPERATOR_ID_LEN];
    char asset[KDTL_ASSET_ID_LEN];
    kdtl_timestamp window_start;
    kdtl_timestamp window_end;
    kdtl_amount reserved;
    uint32_t positions;
    uint64_t policy_version;
} kdtl_limit_bucket;

typedef struct kdtl_debt_bucket {
    bool used;
    char operator_id[KDTL_OPERATOR_ID_LEN];
    char asset[KDTL_ASSET_ID_LEN];
    kdtl_timestamp window_start;
    kdtl_timestamp window_end;
    kdtl_amount pending;
    kdtl_amount claimed;
    kdtl_amount released;
    uint32_t positions;
    kdtl_timestamp last_touched;
} kdtl_debt_bucket;

typedef struct kdtl_position_record {
    bool used;
    kdtl_position_id id;
    kdtl_position_state state;
    char payer[KDTL_ACCOUNT_ID_LEN];
    char beneficiary[KDTL_ACCOUNT_ID_LEN];
    char operator_id[KDTL_OPERATOR_ID_LEN];
    char asset[KDTL_ASSET_ID_LEN];
    char route_id[KDTL_ROUTE_ID_LEN];
    kdtl_amount guarantee;
    kdtl_amount locked;
    kdtl_amount paid_out;
    kdtl_amount refunded;
    kdtl_amount premium;
    uint16_t margin_bps;
    kdtl_timestamp opened_at;
    kdtl_timestamp updated_at;
    kdtl_timestamp window_start;
    kdtl_timestamp window_end;
    kdtl_timestamp expires_at;
    uint64_t nonce;
    uint64_t terms_hash;
} kdtl_position_record;

typedef struct kdtl_event_record {
    bool used;
    uint64_t sequence;
    kdtl_event_kind kind;
    kdtl_timestamp at;
    char operator_id[KDTL_OPERATOR_ID_LEN];
    char asset[KDTL_ASSET_ID_LEN];
    kdtl_position_id position_id;
    kdtl_amount amount;
    char note[KDTL_REASON_LEN];
} kdtl_event_record;

struct kdtl_engine {
    kdtl_account_record accounts[KDTL_MAX_ACCOUNTS];
    kdtl_asset_record assets[KDTL_MAX_ASSETS];
    kdtl_policy_record policies[KDTL_MAX_POLICIES];
    kdtl_limit_bucket limit_buckets[KDTL_MAX_BUCKETS];
    kdtl_debt_bucket debt_buckets[KDTL_MAX_BUCKETS];
    kdtl_position_record positions[KDTL_MAX_POSITIONS];
    kdtl_event_record events[KDTL_MAX_EVENTS];
    size_t account_count;
    size_t asset_count;
    size_t policy_count;
    size_t limit_bucket_count;
    size_t debt_bucket_count;
    size_t position_count;
    size_t event_count;
    kdtl_position_id next_position_id;
    uint64_t next_event_sequence;
    char last_error[KDTL_LAST_ERROR_LEN];
};

void kdtl_clear_error(kdtl_engine *engine);
kdtl_status kdtl_fail(kdtl_engine *engine, kdtl_status status, const char *fmt, ...);
kdtl_status kdtl_failv(kdtl_engine *engine, kdtl_status status, const char *fmt, va_list args);

bool kdtl_copy_id(char *dst, size_t dst_len, const char *src);
bool kdtl_id_eq(const char *lhs, const char *rhs);
bool kdtl_amount_add(kdtl_amount lhs, kdtl_amount rhs, kdtl_amount *out);
bool kdtl_amount_sub(kdtl_amount lhs, kdtl_amount rhs, kdtl_amount *out);
bool kdtl_amount_mul_bps(kdtl_amount amount, uint16_t bps, kdtl_amount *out);
kdtl_amount kdtl_amount_min(kdtl_amount lhs, kdtl_amount rhs);
kdtl_amount kdtl_amount_zero_floor_sub(kdtl_amount lhs, kdtl_amount rhs);

int kdtl_find_asset_index(const kdtl_engine *engine, const char *asset);
int kdtl_find_account_index(const kdtl_engine *engine, const char *account);
kdtl_policy_record *kdtl_find_policy(kdtl_engine *engine, const char *operator_id,
                                     const char *asset);
const kdtl_policy_record *kdtl_find_policy_const(const kdtl_engine *engine,
                                                 const char *operator_id,
                                                 const char *asset);
kdtl_position_record *kdtl_find_position(kdtl_engine *engine, kdtl_position_id id);
const kdtl_position_record *kdtl_find_position_const(const kdtl_engine *engine,
                                                     kdtl_position_id id);

kdtl_status kdtl_debit_liquid(kdtl_engine *engine, const char *account, const char *asset,
                              kdtl_amount amount);
kdtl_status kdtl_credit_liquid(kdtl_engine *engine, const char *account, const char *asset,
                               kdtl_amount amount);
kdtl_status kdtl_lock_account(kdtl_engine *engine, const char *account, const char *asset,
                              kdtl_amount amount);
kdtl_status kdtl_unlock_account(kdtl_engine *engine, const char *account, const char *asset,
                                kdtl_amount amount);
kdtl_status kdtl_debit_locked(kdtl_engine *engine, const char *account, const char *asset,
                              kdtl_amount amount);

kdtl_status kdtl_limit_reserve(kdtl_engine *engine, const kdtl_operator_policy *policy,
                               kdtl_timestamp window_start, kdtl_timestamp window_end,
                               kdtl_amount amount);
kdtl_status kdtl_limit_release(kdtl_engine *engine, const char *operator_id, const char *asset,
                               kdtl_timestamp window_start, kdtl_timestamp window_end,
                               kdtl_amount amount);
kdtl_amount kdtl_limit_visible_reserved(const kdtl_engine *engine, const char *operator_id,
                                        const char *asset, kdtl_timestamp as_of);
kdtl_amount kdtl_limit_window_reserved(const kdtl_engine *engine, const char *operator_id,
                                       const char *asset, kdtl_timestamp window_start,
                                       kdtl_timestamp window_end);
uint32_t kdtl_limit_bucket_count(const kdtl_engine *engine, const char *operator_id,
                                 const char *asset);

kdtl_status kdtl_debt_add(kdtl_engine *engine, const char *operator_id, const char *asset,
                          kdtl_timestamp window_start, kdtl_timestamp window_end,
                          kdtl_amount amount, kdtl_timestamp now);
kdtl_status kdtl_debt_release(kdtl_engine *engine, const char *operator_id, const char *asset,
                              kdtl_timestamp window_start, kdtl_timestamp window_end,
                              kdtl_amount amount, kdtl_timestamp now);
kdtl_status kdtl_debt_claim(kdtl_engine *engine, const char *operator_id, const char *asset,
                            kdtl_timestamp window_start, kdtl_timestamp window_end,
                            kdtl_amount amount, kdtl_timestamp now);
kdtl_status kdtl_debt_mark_window(kdtl_engine *engine, const char *operator_id,
                                  const char *asset, kdtl_timestamp window_start,
                                  kdtl_timestamp window_end, kdtl_timestamp now);
kdtl_amount kdtl_debt_indexed_active(const kdtl_engine *engine, const char *operator_id,
                                     const char *asset, kdtl_timestamp as_of);
uint32_t kdtl_debt_bucket_count(const kdtl_engine *engine, const char *operator_id,
                                const char *asset);

kdtl_amount kdtl_positions_claimable_open(const kdtl_engine *engine, const char *operator_id,
                                          const char *asset, kdtl_timestamp as_of);
uint32_t kdtl_positions_open_count(const kdtl_engine *engine, const char *operator_id,
                                   const char *asset);
uint32_t kdtl_positions_asset_count(const kdtl_engine *engine, const char *asset);
uint32_t kdtl_positions_asset_open_count(const kdtl_engine *engine, const char *asset);

kdtl_status kdtl_record_event(kdtl_engine *engine, kdtl_event_kind kind, kdtl_timestamp at,
                              const char *operator_id, const char *asset,
                              kdtl_position_id position_id, kdtl_amount amount,
                              const char *note);
void kdtl_fill_position_view(const kdtl_position_record *record, kdtl_position_view *out);
void kdtl_digest_hex(uint64_t value, char out[KDTL_HASH_LEN]);
uint64_t kdtl_fnv1a_bytes(const void *data, size_t len, uint64_t seed);
uint64_t kdtl_fnv1a_string(const char *text, uint64_t seed);

#endif
