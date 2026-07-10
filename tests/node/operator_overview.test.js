import test from "node:test";
import assert from "node:assert/strict";
import {
    assertAccount,
    assertOpenRisk,
    assertReconciliation,
    scenario,
} from "../helpers/scenario_helpers.js";

test("operator overview exposes asset catalog and policy utilization", () => {
    const payload = scenario("operator-overview");

    assert.equal(payload.scenario, "operator-overview");
    assert.equal(payload.first_position_id, 1);
    assert.equal(payload.second_position_id, 2);

    assert.equal(payload.asset.asset, "USDC");
    assert.equal(payload.asset.decimals, 6);
    assert.equal(payload.asset.liquidation_fee_bps, 25);
    assert.equal(payload.asset.reserve_factor_bps, 500);
    assert.equal(payload.asset.liquid_total, 675_000);
    assert.equal(payload.asset.locked_total, 325_000);
    assert.equal(payload.asset.funded_accounts, 2);
    assert.equal(payload.asset.policy_count, 1);
    assert.equal(payload.asset.position_count, 2);
    assert.equal(payload.asset.open_positions, 2);

    assert.equal(payload.policy.operator_id, "operator-keystone");
    assert.equal(payload.policy.asset, "USDC");
    assert.equal(payload.policy.max_active_debt, 900_000);
    assert.equal(payload.policy.max_window_debt, 450_000);
    assert.equal(payload.policy.max_open_positions, 16);
    assert.equal(payload.policy.min_margin_bps, 1_200);
    assert.equal(payload.policy.window_grace_seconds, 90);
    assert.equal(payload.policy.version, 1);
    assert.equal(payload.policy.updated_at, 0);
    assert.equal(payload.policy.active_debt, 325_000);
    assert.equal(payload.policy.window_reserved, 325_000);
    assert.equal(payload.policy.active_utilization_bps, 3_611);
    assert.equal(payload.policy.window_utilization_bps, 7_222);
    assert.equal(payload.policy.open_positions, 2);

    assertAccount(payload.operator, "operator-keystone", 1_000, 0);
    assertOpenRisk(payload.risk, 325_000, 325_000, 2);
    assertReconciliation(payload.reconciliation, 325_000, 2, 0);
});
