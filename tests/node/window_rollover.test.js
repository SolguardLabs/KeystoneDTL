import test from "node:test";
import assert from "node:assert/strict";
import {
    assertAccount,
    assertDigest,
    assertOpenRisk,
    assertReconciliation,
    scenario,
} from "../helpers/scenario_helpers.js";

test("window rollover keeps the position open under the requested execution window", () => {
    const payload = scenario("window-rollover");

    assert.equal(payload.scenario, "window-rollover");
    assert.equal(payload.position.state, "open");
    assert.equal(payload.position.route_id, "route-roll-001");
    assert.equal(payload.position.guarantee, 200_000);
    assert.equal(payload.position.locked, 200_000);
    assert.equal(payload.position.window_start, 1_500);
    assert.equal(payload.position.window_end, 2_100);
    assert.equal(payload.position.updated_at, 1_200);
    assertDigest(payload.position.digest);

    assertOpenRisk(payload.risk, 200_000, 200_000, 1);
    assertAccount(payload.payer, "payer-alpha", 800_000, 200_000);
    assertReconciliation(payload.reconciliation, 200_000, 1, 0);
});
