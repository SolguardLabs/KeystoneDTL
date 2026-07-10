import test from "node:test";
import assert from "node:assert/strict";
import { assertOpenRisk, assertReconciliation, scenario } from "../helpers/scenario_helpers.js";

test("policy rejections leave the accepted route and accounting intact", () => {
    const payload = scenario("rejections");

    assert.equal(payload.scenario, "rejections");
    assert.equal(payload.oversize_status, "limit");
    assert.match(payload.oversize_error, /window policy/u);
    assert.equal(payload.update_status, "limit");
    assert.match(payload.update_error, /active window/u);

    assert.equal(payload.position.state, "open");
    assert.equal(payload.position.route_id, "route-reject-002");
    assert.equal(payload.position.guarantee, 150_000);
    assert.equal(payload.position.locked, 150_000);
    assertOpenRisk(payload.risk, 150_000, 150_000, 1);
    assertReconciliation(payload.reconciliation, 150_000, 1, 0);
});
