#include "kdtl_internal.h"

#include <stdio.h>

static kdtl_status write_checked(int written, size_t len) {
    if (written < 0) {
        return KDTL_ERR_BUFFER;
    }
    if ((size_t)written >= len) {
        return KDTL_ERR_BUFFER;
    }
    return KDTL_OK;
}

kdtl_status kdtl_format_position_json(const kdtl_position_view *view, char *buffer, size_t len) {
    int written;

    if (view == NULL || buffer == NULL || len == 0u) {
        return KDTL_ERR_ARGUMENT;
    }
    written =
        snprintf(buffer, len,
                 "{\"id\":%llu,\"state\":\"%s\",\"payer\":\"%s\",\"beneficiary\":\"%s\","
                 "\"operator_id\":\"%s\",\"asset\":\"%s\",\"route_id\":\"%s\",\"guarantee\":%llu,"
                 "\"locked\":%llu,\"paid_out\":%llu,\"refunded\":%llu,\"premium\":%llu,"
                 "\"margin_bps\":%u,\"opened_at\":%llu,\"updated_at\":%llu,"
                 "\"window_start\":%llu,\"window_end\":%llu,\"expires_at\":%llu,\"digest\":\"%s\"}",
                 (unsigned long long)view->id, kdtl_position_state_name(view->state), view->payer,
                 view->beneficiary, view->operator_id, view->asset, view->route_id,
                 (unsigned long long)view->guarantee, (unsigned long long)view->locked,
                 (unsigned long long)view->paid_out, (unsigned long long)view->refunded,
                 (unsigned long long)view->premium, (unsigned)view->margin_bps,
                 (unsigned long long)view->opened_at, (unsigned long long)view->updated_at,
                 (unsigned long long)view->window_start, (unsigned long long)view->window_end,
                 (unsigned long long)view->expires_at, view->digest);
    return write_checked(written, len);
}

kdtl_status kdtl_format_risk_json(const kdtl_risk_view *view, char *buffer, size_t len) {
    int written;

    if (view == NULL || buffer == NULL || len == 0u) {
        return KDTL_ERR_ARGUMENT;
    }
    written = snprintf(
        buffer, len,
        "{\"operator_id\":\"%s\",\"asset\":\"%s\",\"as_of\":%llu,"
        "\"max_active_debt\":%llu,\"max_window_debt\":%llu,\"indexed_debt\":%llu,"
        "\"visible_reserved\":%llu,\"claimable_open\":%llu,\"available_debt\":%llu,"
        "\"open_positions\":%u,\"limit_bucket_count\":%u,\"debt_bucket_count\":%u,"
        "\"within_policy\":%s}",
        view->operator_id, view->asset, (unsigned long long)view->as_of,
        (unsigned long long)view->max_active_debt, (unsigned long long)view->max_window_debt,
        (unsigned long long)view->indexed_debt, (unsigned long long)view->visible_reserved,
        (unsigned long long)view->claimable_open, (unsigned long long)view->available_debt,
        view->open_positions, view->limit_bucket_count, view->debt_bucket_count,
        view->within_policy ? "true" : "false");
    return write_checked(written, len);
}

kdtl_status kdtl_format_account_json(const kdtl_account_view *view, char *buffer, size_t len) {
    int written;

    if (view == NULL || buffer == NULL || len == 0u) {
        return KDTL_ERR_ARGUMENT;
    }
    written = snprintf(buffer, len,
                       "{\"account\":\"%s\",\"asset\":\"%s\",\"liquid\":%llu,\"locked\":%llu}",
                       view->account, view->asset, (unsigned long long)view->liquid,
                       (unsigned long long)view->locked);
    return write_checked(written, len);
}

kdtl_status kdtl_format_asset_json(const kdtl_asset_view *view, char *buffer, size_t len) {
    int written;

    if (view == NULL || buffer == NULL || len == 0u) {
        return KDTL_ERR_ARGUMENT;
    }
    written = snprintf(buffer, len,
                       "{\"asset\":\"%s\",\"decimals\":%u,\"liquidation_fee_bps\":%u,"
                       "\"reserve_factor_bps\":%u,\"liquid_total\":%llu,\"locked_total\":%llu,"
                       "\"funded_accounts\":%u,\"policy_count\":%u,\"position_count\":%u,"
                       "\"open_positions\":%u}",
                       view->asset, (unsigned)view->decimals, (unsigned)view->liquidation_fee_bps,
                       (unsigned)view->reserve_factor_bps, (unsigned long long)view->liquid_total,
                       (unsigned long long)view->locked_total, view->funded_accounts,
                       view->policy_count, view->position_count, view->open_positions);
    return write_checked(written, len);
}

kdtl_status kdtl_format_policy_json(const kdtl_policy_view *view, char *buffer, size_t len) {
    int written;

    if (view == NULL || buffer == NULL || len == 0u) {
        return KDTL_ERR_ARGUMENT;
    }
    written =
        snprintf(buffer, len,
                 "{\"operator_id\":\"%s\",\"asset\":\"%s\",\"max_active_debt\":%llu,"
                 "\"max_window_debt\":%llu,\"max_open_positions\":%u,\"min_margin_bps\":%u,"
                 "\"window_grace_seconds\":%u,\"version\":%llu,\"updated_at\":%llu,"
                 "\"active_debt\":%llu,\"window_reserved\":%llu,\"active_utilization_bps\":%u,"
                 "\"window_utilization_bps\":%u,\"open_positions\":%u}",
                 view->operator_id, view->asset, (unsigned long long)view->max_active_debt,
                 (unsigned long long)view->max_window_debt, view->max_open_positions,
                 (unsigned)view->min_margin_bps, view->window_grace_seconds,
                 (unsigned long long)view->version, (unsigned long long)view->updated_at,
                 (unsigned long long)view->active_debt, (unsigned long long)view->window_reserved,
                 view->active_utilization_bps, view->window_utilization_bps, view->open_positions);
    return write_checked(written, len);
}

kdtl_status kdtl_format_reconciliation_json(const kdtl_reconciliation_view *view, char *buffer,
                                            size_t len) {
    int written;

    if (view == NULL || buffer == NULL || len == 0u) {
        return KDTL_ERR_ARGUMENT;
    }
    written =
        snprintf(buffer, len,
                 "{\"asset\":\"%s\",\"liquid_total\":%llu,\"locked_total\":%llu,"
                 "\"accounted_total\":%llu,\"account_count\":%u,\"open_positions\":%u,"
                 "\"terminal_positions\":%u,\"event_count\":%llu,\"conserved\":%s}",
                 view->asset, (unsigned long long)view->liquid_total,
                 (unsigned long long)view->locked_total, (unsigned long long)view->accounted_total,
                 view->account_count, view->open_positions, view->terminal_positions,
                 (unsigned long long)view->event_count, view->conserved ? "true" : "false");
    return write_checked(written, len);
}

kdtl_status kdtl_format_event_json(const kdtl_event_view *view, char *buffer, size_t len) {
    int written;

    if (view == NULL || buffer == NULL || len == 0u) {
        return KDTL_ERR_ARGUMENT;
    }
    written = snprintf(buffer, len,
                       "{\"sequence\":%llu,\"kind\":%u,\"at\":%llu,\"operator_id\":\"%s\","
                       "\"asset\":\"%s\",\"position_id\":%llu,\"amount\":%llu,\"note\":\"%s\"}",
                       (unsigned long long)view->sequence, (unsigned)view->kind,
                       (unsigned long long)view->at, view->operator_id, view->asset,
                       (unsigned long long)view->position_id, (unsigned long long)view->amount,
                       view->note);
    return write_checked(written, len);
}
