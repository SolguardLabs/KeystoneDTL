#include "kdtl_internal.h"

#include <string.h>

kdtl_status kdtl_liquidate_position(kdtl_engine *engine, const kdtl_liquidation_claim *claim) {
    kdtl_position_record *position;
    kdtl_amount penalty;
    kdtl_amount requested;
    kdtl_amount payout;
    kdtl_amount refund;
    kdtl_status status;

    if (engine == NULL || claim == NULL) {
        return KDTL_ERR_ARGUMENT;
    }
    kdtl_clear_error(engine);

    position = kdtl_find_position(engine, claim->position_id);
    if (position == NULL) {
        return kdtl_fail(engine, KDTL_ERR_NOT_FOUND, "position not found");
    }
    if (position->state != KDTL_POSITION_OPEN) {
        return kdtl_fail(engine, KDTL_ERR_STATE, "position is terminal");
    }
    if (claim->observed_loss == 0u) {
        return kdtl_fail(engine, KDTL_ERR_ARGUMENT, "observed loss is zero");
    }
    if (claim->penalty_bps > 10000u) {
        return kdtl_fail(engine, KDTL_ERR_ARGUMENT, "invalid penalty");
    }
    if (claim->now < position->window_start || claim->now > position->expires_at) {
        return kdtl_fail(engine, KDTL_ERR_STATE, "liquidation outside claim window");
    }
    if (kdtl_find_account_index(engine, claim->liquidator) < 0) {
        return kdtl_fail(engine, KDTL_ERR_NOT_FOUND, "liquidator not registered");
    }

    if (!kdtl_amount_mul_bps(claim->observed_loss, claim->penalty_bps, &penalty)) {
        return kdtl_fail(engine, KDTL_ERR_OVERFLOW, "penalty overflow");
    }
    if (!kdtl_amount_add(claim->observed_loss, penalty, &requested)) {
        return kdtl_fail(engine, KDTL_ERR_OVERFLOW, "claim overflow");
    }
    payout = kdtl_amount_min(requested, position->locked);
    refund = position->locked - payout;

    status = kdtl_debit_locked(engine, position->payer, position->asset, position->locked);
    if (status != KDTL_OK) {
        return status;
    }
    status = kdtl_credit_liquid(engine, claim->liquidator, position->asset, payout);
    if (status != KDTL_OK) {
        return status;
    }
    status = kdtl_credit_liquid(engine, position->payer, position->asset, refund);
    if (status != KDTL_OK) {
        return status;
    }

    position->paid_out = payout;
    position->refunded = refund;
    position->locked = 0u;
    position->state = KDTL_POSITION_LIQUIDATED;
    position->updated_at = claim->now;

    (void)kdtl_limit_release(engine, position->operator_id, position->asset, position->window_start,
                             position->window_end, position->guarantee);
    (void)kdtl_debt_claim(engine, position->operator_id, position->asset, position->window_start,
                          position->window_end, position->guarantee, claim->now);

    return kdtl_record_event(engine, KDTL_EVENT_POSITION_LIQUIDATED, claim->now,
                             position->operator_id, position->asset, position->id, payout,
                             "liquidate");
}
