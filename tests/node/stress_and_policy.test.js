import assert from "node:assert/strict";
import test from "node:test";

import { scenario } from "../helpers/scenario_helpers.js";

test("stress report allocates credit loss through reserves and checks corridors", () => {
    const result = scenario("stress-report").report;

    assert.equal(result.performing_guarantee, 450_000);
    assert.equal(result.defaulted_guarantee, 150_000);
    assert.equal(result.gross_recovery, 60_000);
    assert.equal(result.liquidation_cost, 6_000);
    assert.equal(result.credit_loss, 96_000);
    assert.equal(result.usable_reserves, 60_000);
    assert.equal(result.reserve_absorption, 60_000);
    assert.equal(result.residual_loss, 36_000);
    assert.equal(result.stressed_liquidity, 800_000);
    assert.equal(result.coverage_assets, 564_000);
    assert.equal(result.required_buffer, 560_000);
    assert.equal(result.buffer_surplus, 240_000);
    assert.equal(result.coverage_bps, 8057);
    assert.equal(result.concentration_bps, 4000);
    assert.equal(result.within_corridor, true);
});

test("policy review escalates exposure and out-of-corridor proposals", () => {
    const result = scenario("policy-review");

    assert.equal(result.reviewed.structurally_valid, true);
    assert.equal(result.reviewed.within_corridor, true);
    assert.equal(result.reviewed.review_tier, "risk");
    assert.equal(result.reviewed.active_headroom, 200_000);
    assert.equal(result.reviewed.window_headroom, 100_000);
    assert.equal(result.reviewed.active_to_window_bps, 5000);
    assert.notEqual(result.reviewed.change_flags, 0);

    assert.equal(result.escalated.structurally_valid, true);
    assert.equal(result.escalated.within_corridor, false);
    assert.equal(result.escalated.review_tier, "governance");
});
