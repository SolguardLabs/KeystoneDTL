import test from "node:test";
import assert from "node:assert/strict";
import { assertOpenRisk, assertReconciliation, scenario } from "../helpers/scenario_helpers.js";

test("event journal exposes ordered lifecycle records for route movement", () => {
    const payload = scenario("event-journal");

    assert.equal(payload.scenario, "event-journal");
    assert.equal(payload.opened.sequence, 9);
    assert.equal(payload.opened.kind, 5);
    assert.equal(payload.opened.position_id, 1);
    assert.equal(payload.opened.amount, 125_000);
    assert.equal(payload.opened.note, "open");

    assert.equal(payload.moved.sequence, 10);
    assert.equal(payload.moved.kind, 6);
    assert.equal(payload.moved.position_id, 1);
    assert.equal(payload.moved.amount, 125_000);
    assert.equal(payload.moved.note, "route-delay");

    assertOpenRisk(payload.risk, 125_000, 125_000, 1);
    assertReconciliation(payload.reconciliation, 125_000, 1, 0);
});
