#include "kdtl_internal.h"

#include <string.h>

static uint32_t ratio_bps(kdtl_amount numerator, kdtl_amount denominator) {
    uint64_t quotient;
    uint64_t remainder;

    if (denominator == 0u) {
        return numerator == 0u ? 0u : UINT32_MAX;
    }
    quotient = numerator / denominator;
    remainder = numerator % denominator;
    if (quotient >= UINT32_MAX / 10000u || remainder > UINT64_MAX / 10000u) {
        return UINT32_MAX;
    }
    quotient = quotient * 10000u + (remainder * 10000u) / denominator;
    return quotient > UINT32_MAX ? UINT32_MAX : (uint32_t)quotient;
}

static uint64_t change_flags(const kdtl_operator_policy *current,
                             const kdtl_operator_policy *proposed) {
    uint64_t flags = 0u;

    if (proposed->max_active_debt > current->max_active_debt) {
        flags |= KDTL_POLICY_ACTIVE_DEBT_INCREASE;
    }
    if (proposed->max_window_debt > current->max_window_debt) {
        flags |= KDTL_POLICY_WINDOW_DEBT_INCREASE;
    }
    if (proposed->max_open_positions > current->max_open_positions) {
        flags |= KDTL_POLICY_OPEN_POSITIONS_INCREASE;
    }
    if (proposed->min_margin_bps < current->min_margin_bps) {
        flags |= KDTL_POLICY_MARGIN_DECREASE;
    }
    if (proposed->window_grace_seconds > current->window_grace_seconds) {
        flags |= KDTL_POLICY_GRACE_INCREASE;
    }
    if (!kdtl_id_eq(proposed->operator_id, current->operator_id)) {
        flags |= KDTL_POLICY_OPERATOR_CHANGE;
    }
    if (!kdtl_id_eq(proposed->asset, current->asset)) {
        flags |= KDTL_POLICY_ASSET_CHANGE;
    }
    return flags;
}

static kdtl_review_tier review_tier(uint64_t flags, bool within_corridor) {
    const uint64_t identity = KDTL_POLICY_OPERATOR_CHANGE | KDTL_POLICY_ASSET_CHANGE;
    const uint64_t risk = KDTL_POLICY_MARGIN_DECREASE | KDTL_POLICY_GRACE_INCREASE;
    const uint64_t treasury = KDTL_POLICY_ACTIVE_DEBT_INCREASE | KDTL_POLICY_WINDOW_DEBT_INCREASE |
                              KDTL_POLICY_OPEN_POSITIONS_INCREASE;

    if (!within_corridor || (flags & identity) != 0u) {
        return KDTL_REVIEW_GOVERNANCE;
    }
    if ((flags & risk) != 0u) {
        return KDTL_REVIEW_RISK;
    }
    if ((flags & treasury) != 0u) {
        return KDTL_REVIEW_TREASURY;
    }
    return KDTL_REVIEW_ROUTINE;
}

kdtl_status kdtl_review_operator_policy(const kdtl_operator_policy *current,
                                        const kdtl_operator_policy *proposed,
                                        const kdtl_policy_corridor *corridor,
                                        kdtl_policy_assessment *out) {
    if (current == NULL || proposed == NULL || corridor == NULL || out == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    if (corridor->maximum_active_debt == 0u || corridor->maximum_window_debt == 0u ||
        corridor->maximum_open_positions == 0u || corridor->minimum_margin_bps > 10000u ||
        proposed->operator_id[0] == '\0' || proposed->asset[0] == '\0') {
        return KDTL_ERR_ARGUMENT;
    }

    memset(out, 0, sizeof(*out));
    out->change_flags = change_flags(current, proposed);
    out->structurally_valid = proposed->max_active_debt > 0u && proposed->max_window_debt > 0u &&
                              proposed->max_window_debt <= proposed->max_active_debt &&
                              proposed->max_open_positions > 0u &&
                              proposed->min_margin_bps <= 10000u;
    out->within_corridor = out->structurally_valid &&
                           proposed->max_active_debt <= corridor->maximum_active_debt &&
                           proposed->max_window_debt <= corridor->maximum_window_debt &&
                           proposed->max_open_positions <= corridor->maximum_open_positions &&
                           proposed->min_margin_bps >= corridor->minimum_margin_bps &&
                           proposed->window_grace_seconds <= corridor->maximum_grace_seconds;
    out->active_headroom =
        kdtl_amount_zero_floor_sub(corridor->maximum_active_debt, proposed->max_active_debt);
    out->window_headroom =
        kdtl_amount_zero_floor_sub(corridor->maximum_window_debt, proposed->max_window_debt);
    out->active_to_window_bps = ratio_bps(proposed->max_window_debt, proposed->max_active_debt);
    out->review_tier = review_tier(out->change_flags, out->within_corridor);
    return KDTL_OK;
}

const char *kdtl_review_tier_name(kdtl_review_tier tier) {
    switch (tier) {
    case KDTL_REVIEW_ROUTINE:
        return "routine";
    case KDTL_REVIEW_RISK:
        return "risk";
    case KDTL_REVIEW_TREASURY:
        return "treasury";
    case KDTL_REVIEW_GOVERNANCE:
        return "governance";
    default:
        return "unknown";
    }
}
