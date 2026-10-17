#include "keystone_dtl.h"

#include <stdio.h>

static kdtl_status write_result(int written, size_t len) {
    return written < 0 || (size_t)written >= len ? KDTL_ERR_BUFFER : KDTL_OK;
}

kdtl_status kdtl_format_stress_json(const kdtl_stress_result *view, char *buffer, size_t len) {
    int written;

    if (view == NULL || buffer == NULL || len == 0u) {
        return KDTL_ERR_ARGUMENT;
    }
    written = snprintf(
        buffer, len,
        "{\"performing_guarantee\":%llu,\"defaulted_guarantee\":%llu,"
        "\"gross_recovery\":%llu,\"liquidation_cost\":%llu,\"net_recovery\":%llu,"
        "\"credit_loss\":%llu,\"usable_reserves\":%llu,\"reserve_absorption\":%llu,"
        "\"residual_loss\":%llu,\"stressed_liquidity\":%llu,\"coverage_assets\":%llu,"
        "\"required_buffer\":%llu,\"buffer_surplus\":%llu,\"coverage_bps\":%u,"
        "\"concentration_bps\":%u,\"coverage_pass\":%s,\"concentration_pass\":%s,"
        "\"within_corridor\":%s}",
        (unsigned long long)view->performing_guarantee,
        (unsigned long long)view->defaulted_guarantee, (unsigned long long)view->gross_recovery,
        (unsigned long long)view->liquidation_cost, (unsigned long long)view->net_recovery,
        (unsigned long long)view->credit_loss, (unsigned long long)view->usable_reserves,
        (unsigned long long)view->reserve_absorption, (unsigned long long)view->residual_loss,
        (unsigned long long)view->stressed_liquidity, (unsigned long long)view->coverage_assets,
        (unsigned long long)view->required_buffer, (unsigned long long)view->buffer_surplus,
        view->coverage_bps, view->concentration_bps, view->coverage_pass ? "true" : "false",
        view->concentration_pass ? "true" : "false", view->within_corridor ? "true" : "false");
    return write_result(written, len);
}

kdtl_status kdtl_format_policy_assessment_json(const kdtl_policy_assessment *view, char *buffer,
                                               size_t len) {
    int written;

    if (view == NULL || buffer == NULL || len == 0u) {
        return KDTL_ERR_ARGUMENT;
    }
    written = snprintf(
        buffer, len,
        "{\"change_flags\":%llu,\"active_headroom\":%llu,"
        "\"window_headroom\":%llu,\"active_to_window_bps\":%u,"
        "\"review_tier\":\"%s\",\"structurally_valid\":%s,"
        "\"within_corridor\":%s}",
        (unsigned long long)view->change_flags, (unsigned long long)view->active_headroom,
        (unsigned long long)view->window_headroom, view->active_to_window_bps,
        kdtl_review_tier_name(view->review_tier), view->structurally_valid ? "true" : "false",
        view->within_corridor ? "true" : "false");
    return write_result(written, len);
}
