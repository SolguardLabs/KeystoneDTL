#ifndef KEYSTONE_DTL_H
#define KEYSTONE_DTL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KDTL_ACCOUNT_ID_LEN 40u
#define KDTL_OPERATOR_ID_LEN 40u
#define KDTL_ASSET_ID_LEN 16u
#define KDTL_ROUTE_ID_LEN 40u
#define KDTL_REASON_LEN 96u
#define KDTL_HASH_LEN 65u

typedef struct kdtl_engine kdtl_engine;
typedef uint64_t kdtl_amount;
typedef uint64_t kdtl_position_id;
typedef uint64_t kdtl_timestamp;

typedef enum kdtl_status {
    KDTL_OK = 0,
    KDTL_ERR_ARGUMENT = 1,
    KDTL_ERR_NOT_FOUND = 2,
    KDTL_ERR_EXISTS = 3,
    KDTL_ERR_CAPACITY = 4,
    KDTL_ERR_BALANCE = 5,
    KDTL_ERR_STATE = 6,
    KDTL_ERR_LIMIT = 7,
    KDTL_ERR_OVERFLOW = 8,
    KDTL_ERR_BUFFER = 9
} kdtl_status;

typedef enum kdtl_position_state {
    KDTL_POSITION_OPEN = 1,
    KDTL_POSITION_LIQUIDATED = 2,
    KDTL_POSITION_CANCELLED = 3,
    KDTL_POSITION_EXPIRED = 4
} kdtl_position_state;

typedef enum kdtl_event_kind {
    KDTL_EVENT_ACCOUNT_REGISTERED = 1,
    KDTL_EVENT_ASSET_REGISTERED = 2,
    KDTL_EVENT_OPERATOR_LIMIT_SET = 3,
    KDTL_EVENT_DEPOSITED = 4,
    KDTL_EVENT_POSITION_OPENED = 5,
    KDTL_EVENT_POSITION_REWINDOWED = 6,
    KDTL_EVENT_POSITION_LIQUIDATED = 7,
    KDTL_EVENT_POSITION_CANCELLED = 8,
    KDTL_EVENT_POSITION_EXPIRED = 9
} kdtl_event_kind;

typedef struct kdtl_asset_config {
    char asset[KDTL_ASSET_ID_LEN];
    uint8_t decimals;
    uint16_t liquidation_fee_bps;
    uint16_t reserve_factor_bps;
} kdtl_asset_config;

typedef struct kdtl_operator_policy {
    char operator_id[KDTL_OPERATOR_ID_LEN];
    char asset[KDTL_ASSET_ID_LEN];
    kdtl_amount max_active_debt;
    kdtl_amount max_window_debt;
    uint32_t max_open_positions;
    uint16_t min_margin_bps;
    uint32_t window_grace_seconds;
} kdtl_operator_policy;

typedef struct kdtl_route_terms {
    char payer[KDTL_ACCOUNT_ID_LEN];
    char beneficiary[KDTL_ACCOUNT_ID_LEN];
    char operator_id[KDTL_OPERATOR_ID_LEN];
    char asset[KDTL_ASSET_ID_LEN];
    char route_id[KDTL_ROUTE_ID_LEN];
    kdtl_amount guarantee;
    kdtl_amount premium;
    uint16_t margin_bps;
    kdtl_timestamp opened_at;
    kdtl_timestamp window_start;
    kdtl_timestamp window_end;
    kdtl_timestamp expires_at;
    uint64_t nonce;
} kdtl_route_terms;

typedef struct kdtl_move_request {
    kdtl_position_id position_id;
    kdtl_timestamp now;
    kdtl_timestamp new_window_start;
    kdtl_timestamp new_window_end;
    char reason[KDTL_REASON_LEN];
} kdtl_move_request;

typedef struct kdtl_liquidation_claim {
    kdtl_position_id position_id;
    char liquidator[KDTL_ACCOUNT_ID_LEN];
    kdtl_amount observed_loss;
    uint16_t penalty_bps;
    kdtl_timestamp now;
} kdtl_liquidation_claim;

typedef struct kdtl_cancel_request {
    kdtl_position_id position_id;
    char requested_by[KDTL_ACCOUNT_ID_LEN];
    char refund_to[KDTL_ACCOUNT_ID_LEN];
    kdtl_timestamp now;
} kdtl_cancel_request;

typedef struct kdtl_position_view {
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
    char digest[KDTL_HASH_LEN];
} kdtl_position_view;

typedef struct kdtl_risk_view {
    char operator_id[KDTL_OPERATOR_ID_LEN];
    char asset[KDTL_ASSET_ID_LEN];
    kdtl_timestamp as_of;
    kdtl_amount max_active_debt;
    kdtl_amount max_window_debt;
    kdtl_amount indexed_debt;
    kdtl_amount visible_reserved;
    kdtl_amount claimable_open;
    kdtl_amount available_debt;
    uint32_t open_positions;
    uint32_t limit_bucket_count;
    uint32_t debt_bucket_count;
    bool within_policy;
} kdtl_risk_view;

typedef struct kdtl_account_view {
    char account[KDTL_ACCOUNT_ID_LEN];
    char asset[KDTL_ASSET_ID_LEN];
    kdtl_amount liquid;
    kdtl_amount locked;
} kdtl_account_view;

typedef struct kdtl_asset_view {
    char asset[KDTL_ASSET_ID_LEN];
    uint8_t decimals;
    uint16_t liquidation_fee_bps;
    uint16_t reserve_factor_bps;
    kdtl_amount liquid_total;
    kdtl_amount locked_total;
    uint32_t funded_accounts;
    uint32_t policy_count;
    uint32_t position_count;
    uint32_t open_positions;
} kdtl_asset_view;

typedef struct kdtl_policy_view {
    char operator_id[KDTL_OPERATOR_ID_LEN];
    char asset[KDTL_ASSET_ID_LEN];
    kdtl_amount max_active_debt;
    kdtl_amount max_window_debt;
    uint32_t max_open_positions;
    uint16_t min_margin_bps;
    uint32_t window_grace_seconds;
    uint64_t version;
    kdtl_timestamp updated_at;
    kdtl_amount active_debt;
    kdtl_amount window_reserved;
    uint32_t active_utilization_bps;
    uint32_t window_utilization_bps;
    uint32_t open_positions;
} kdtl_policy_view;

typedef struct kdtl_reconciliation_view {
    char asset[KDTL_ASSET_ID_LEN];
    kdtl_amount liquid_total;
    kdtl_amount locked_total;
    kdtl_amount accounted_total;
    uint32_t account_count;
    uint32_t open_positions;
    uint32_t terminal_positions;
    uint64_t event_count;
    bool conserved;
} kdtl_reconciliation_view;

typedef struct kdtl_event_view {
    uint64_t sequence;
    kdtl_event_kind kind;
    kdtl_timestamp at;
    char operator_id[KDTL_OPERATOR_ID_LEN];
    char asset[KDTL_ASSET_ID_LEN];
    kdtl_position_id position_id;
    kdtl_amount amount;
    char note[KDTL_REASON_LEN];
} kdtl_event_view;

kdtl_engine *kdtl_engine_create(void);
void kdtl_engine_destroy(kdtl_engine *engine);
void kdtl_engine_reset(kdtl_engine *engine);

const char *kdtl_status_name(kdtl_status status);
const char *kdtl_position_state_name(kdtl_position_state state);
const char *kdtl_last_error(const kdtl_engine *engine);

kdtl_status kdtl_register_asset(kdtl_engine *engine, const kdtl_asset_config *config);
kdtl_status kdtl_register_account(kdtl_engine *engine, const char *account);
kdtl_status kdtl_set_operator_policy(kdtl_engine *engine, const kdtl_operator_policy *policy);
kdtl_status kdtl_update_operator_policy(kdtl_engine *engine, const kdtl_operator_policy *policy,
                                        kdtl_timestamp now);
kdtl_status kdtl_deposit(kdtl_engine *engine, const char *account, const char *asset,
                         kdtl_amount amount);

kdtl_status kdtl_open_position(kdtl_engine *engine, const kdtl_route_terms *terms,
                               kdtl_position_id *out_id);
kdtl_status kdtl_move_position(kdtl_engine *engine, const kdtl_move_request *request);
kdtl_status kdtl_liquidate_position(kdtl_engine *engine, const kdtl_liquidation_claim *claim);
kdtl_status kdtl_cancel_position(kdtl_engine *engine, const kdtl_cancel_request *request);
kdtl_status kdtl_expire_positions(kdtl_engine *engine, kdtl_timestamp now, uint32_t *expired);

kdtl_status kdtl_get_position(const kdtl_engine *engine, kdtl_position_id id,
                              kdtl_position_view *out);
kdtl_status kdtl_get_operator_risk(const kdtl_engine *engine, const char *operator_id,
                                   const char *asset, kdtl_timestamp as_of, kdtl_risk_view *out);
kdtl_status kdtl_get_account(const kdtl_engine *engine, const char *account, const char *asset,
                             kdtl_account_view *out);
kdtl_status kdtl_get_asset(const kdtl_engine *engine, const char *asset, kdtl_asset_view *out);
kdtl_status kdtl_get_operator_policy(const kdtl_engine *engine, const char *operator_id,
                                     const char *asset, kdtl_timestamp as_of,
                                     kdtl_policy_view *out);
kdtl_status kdtl_reconcile_asset(const kdtl_engine *engine, const char *asset,
                                 kdtl_reconciliation_view *out);
kdtl_status kdtl_get_event(const kdtl_engine *engine, uint64_t sequence, kdtl_event_view *out);

kdtl_status kdtl_format_position_json(const kdtl_position_view *view, char *buffer, size_t len);
kdtl_status kdtl_format_risk_json(const kdtl_risk_view *view, char *buffer, size_t len);
kdtl_status kdtl_format_account_json(const kdtl_account_view *view, char *buffer, size_t len);
kdtl_status kdtl_format_asset_json(const kdtl_asset_view *view, char *buffer, size_t len);
kdtl_status kdtl_format_policy_json(const kdtl_policy_view *view, char *buffer, size_t len);
kdtl_status kdtl_format_reconciliation_json(const kdtl_reconciliation_view *view, char *buffer,
                                            size_t len);
kdtl_status kdtl_format_event_json(const kdtl_event_view *view, char *buffer, size_t len);

uint64_t kdtl_hash_terms(const kdtl_route_terms *terms);
uint64_t kdtl_hash_position_view(const kdtl_position_view *view);

#ifdef __cplusplus
}
#endif

#endif
