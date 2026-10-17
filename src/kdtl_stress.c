#include "kdtl_internal.h"

#include <limits.h>
#include <string.h>

static uint32_t ratio_bps(kdtl_amount numerator, kdtl_amount denominator) {
    uint64_t whole;
    uint64_t remainder;
    uint64_t scaled;

    if (denominator == 0u) {
        return numerator == 0u ? 10000u : UINT32_MAX;
    }
    if (numerator >= denominator) {
        whole = numerator / denominator;
        if (whole >= UINT32_MAX / 10000u) {
            return UINT32_MAX;
        }
    }
    whole = numerator / denominator;
    remainder = numerator % denominator;
    scaled = whole * 10000u;
    if (remainder > UINT64_MAX / 10000u) {
        return UINT32_MAX;
    }
    scaled += (remainder * 10000u) / denominator;
    return scaled > UINT32_MAX ? UINT32_MAX : (uint32_t)scaled;
}

static bool valid_scenario(const kdtl_stress_scenario *scenario) {
    return scenario != NULL && scenario->default_bps <= 10000u &&
           scenario->recovery_bps <= 10000u && scenario->liquidation_cost_bps <= 10000u &&
           scenario->liquidity_haircut_bps <= 10000u && scenario->reserve_haircut_bps <= 10000u &&
           scenario->minimum_coverage_bps <= 10000u &&
           scenario->maximum_concentration_bps <= 10000u;
}

kdtl_status kdtl_evaluate_stress(const kdtl_stress_snapshot *snapshot,
                                 const kdtl_stress_scenario *scenario, kdtl_stress_result *out) {
    kdtl_amount total_exposure;
    kdtl_amount unavailable_liquidity;
    kdtl_amount reserve_loss;

    if (snapshot == NULL || out == NULL || !valid_scenario(scenario)) {
        return KDTL_ERR_ARGUMENT;
    }
    if (snapshot->largest_route > snapshot->active_guarantee ||
        snapshot->protocol_reserves > snapshot->funded_liquidity ||
        !kdtl_amount_add(snapshot->active_guarantee, snapshot->pending_settlement,
                         &total_exposure)) {
        return KDTL_ERR_ARGUMENT;
    }

    memset(out, 0, sizeof(*out));
    if (!kdtl_amount_mul_bps(snapshot->active_guarantee, scenario->default_bps,
                             &out->defaulted_guarantee) ||
        !kdtl_amount_sub(snapshot->active_guarantee, out->defaulted_guarantee,
                         &out->performing_guarantee) ||
        !kdtl_amount_mul_bps(out->defaulted_guarantee, scenario->recovery_bps,
                             &out->gross_recovery) ||
        !kdtl_amount_mul_bps(out->gross_recovery, scenario->liquidation_cost_bps,
                             &out->liquidation_cost) ||
        !kdtl_amount_sub(out->gross_recovery, out->liquidation_cost, &out->net_recovery) ||
        !kdtl_amount_add(out->performing_guarantee, out->net_recovery, &out->coverage_assets) ||
        !kdtl_amount_sub(snapshot->active_guarantee, out->coverage_assets, &out->credit_loss) ||
        !kdtl_amount_mul_bps(snapshot->protocol_reserves, scenario->reserve_haircut_bps,
                             &reserve_loss) ||
        !kdtl_amount_sub(snapshot->protocol_reserves, reserve_loss, &out->usable_reserves) ||
        !kdtl_amount_mul_bps(snapshot->funded_liquidity, scenario->liquidity_haircut_bps,
                             &unavailable_liquidity) ||
        !kdtl_amount_sub(snapshot->funded_liquidity, unavailable_liquidity,
                         &out->stressed_liquidity)) {
        return KDTL_ERR_OVERFLOW;
    }

    out->reserve_absorption = kdtl_amount_min(out->credit_loss, out->usable_reserves);
    if (!kdtl_amount_sub(out->credit_loss, out->reserve_absorption, &out->residual_loss) ||
        !kdtl_amount_add(out->coverage_assets, out->reserve_absorption, &out->coverage_assets) ||
        !kdtl_amount_mul_bps(total_exposure, scenario->minimum_coverage_bps,
                             &out->required_buffer)) {
        return KDTL_ERR_OVERFLOW;
    }

    out->buffer_surplus = kdtl_amount_zero_floor_sub(out->stressed_liquidity, out->required_buffer);
    out->coverage_bps = ratio_bps(out->coverage_assets, total_exposure);
    out->concentration_bps = ratio_bps(snapshot->largest_route, snapshot->active_guarantee);
    out->coverage_pass = out->coverage_bps >= scenario->minimum_coverage_bps;
    out->concentration_pass = out->concentration_bps <= scenario->maximum_concentration_bps;
    out->within_corridor = out->coverage_pass && out->concentration_pass &&
                           out->stressed_liquidity >= out->required_buffer;
    return KDTL_OK;
}
