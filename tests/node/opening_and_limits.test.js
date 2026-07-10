import test from "node:test";
import assert from "node:assert/strict";
import {
    assertAccount,
    assertDigest,
    assertOpenRisk,
    assertReconciliation,
    scenario,
} from "../helpers/scenario_helpers.js";

test("open position locks payer guarantee and pays route premium", () => {
    const payload = scenario("open-basic");

    assert.equal(payload.scenario, "open-basic");
    assert.equal(payload.position_id, 1);
    assert.equal(payload.position.state, "open");
    assert.equal(payload.position.route_id, "route-open-001");
    assert.equal(payload.position.guarantee, 250_000);
    assert.equal(payload.position.locked, 250_000);
    assert.equal(payload.position.premium, 1_000);
    assert.equal(payload.position.window_start, 1_100);
    assert.equal(payload.position.window_end, 1_600);
    assertDigest(payload.position.digest);

    assertAccount(payload.payer, "payer-alpha", 749_000, 250_000);
    assertAccount(payload.operator, "operator-keystone", 1_000, 0);
    assertOpenRisk(payload.risk, 250_000, 250_000, 1);
    assert.equal(payload.risk.available_debt, 350_000);
    assertReconciliation(payload.reconciliation, 250_000, 1, 0);
});

test("operator policy update changes risk ceilings without mutating position", () => {
    const payload = scenario("limit-update");

    assert.equal(payload.scenario, "limit-update");
    assert.equal(payload.position.state, "open");
    assert.equal(payload.position.guarantee, 100_000);
    assert.equal(payload.before.max_active_debt, 500_000);
    assert.equal(payload.before.max_window_debt, 250_000);
    assert.equal(payload.after.max_active_debt, 800_000);
    assert.equal(payload.after.max_window_debt, 400_000);
    assert.equal(payload.after.indexed_debt, 100_000);
    assert.equal(payload.after.visible_reserved, 100_000);
    assert.equal(payload.after.available_debt, 700_000);
    assertReconciliation(payload.reconciliation, 100_000, 1, 0);
});
