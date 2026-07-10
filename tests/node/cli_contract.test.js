import test from "node:test";
import assert from "node:assert/strict";
import { scenario } from "../helpers/scenario_helpers.js";

test("catalog exposes supported scenario names", () => {
    const payload = scenario("catalog");

    assert.equal(payload.scenario, "catalog");
    assert.equal(payload.library, "KeystoneDTL");
    assert.equal(payload.language, "C11");
    assert.equal(payload.wire, "json");
    assert.deepEqual(payload.scenarios, [
        "open-basic",
        "limit-update",
        "liquidation",
        "cancellation",
        "expiration",
        "window-rollover",
        "event-journal",
        "rejections",
        "operator-overview",
    ]);
});
