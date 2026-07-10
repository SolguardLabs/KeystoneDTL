import test from "node:test";
import assert from "node:assert/strict";
import {
    assertAccount,
    assertOpenRisk,
    assertReconciliation,
    scenario,
} from "../helpers/scenario_helpers.js";

test("liquidation pays observed loss plus penalty and refunds remainder", () => {
    const payload = scenario("liquidation");

    assert.equal(payload.scenario, "liquidation");
    assert.equal(payload.position.state, "liquidated");
    assert.equal(payload.position.guarantee, 300_000);
    assert.equal(payload.position.locked, 0);
    assert.equal(payload.position.paid_out, 126_000);
    assert.equal(payload.position.refunded, 174_000);
    assertAccount(payload.liquidator, "liquidator-gamma", 126_000, 0);
    assertAccount(payload.payer, "payer-alpha", 874_000, 0);
    assertOpenRisk(payload.risk, 0, 0, 0);
    assertReconciliation(payload.reconciliation, 0, 0, 1);
});

test("cancellation before the execution window releases the full guarantee", () => {
    const payload = scenario("cancellation");

    assert.equal(payload.scenario, "cancellation");
    assert.equal(payload.position.state, "cancelled");
    assert.equal(payload.position.locked, 0);
    assert.equal(payload.position.refunded, 180_000);
    assertAccount(payload.payer, "payer-alpha", 1_000_000, 0);
    assertOpenRisk(payload.risk, 0, 0, 0);
    assertReconciliation(payload.reconciliation, 0, 0, 1);
});

test("expiration closes claimable positions at the terminal timestamp", () => {
    const payload = scenario("expiration");

    assert.equal(payload.scenario, "expiration");
    assert.equal(payload.expired, 1);
    assert.equal(payload.position.state, "expired");
    assert.equal(payload.position.locked, 0);
    assert.equal(payload.position.refunded, 90_000);
    assertAccount(payload.payer, "payer-alpha", 1_000_000, 0);
    assertOpenRisk(payload.risk, 0, 0, 0);
    assertReconciliation(payload.reconciliation, 0, 0, 1);
});
