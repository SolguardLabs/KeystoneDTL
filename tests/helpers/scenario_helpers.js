import assert from "node:assert/strict";
import { existsSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const root = join(here, "..", "..");
const binaryName = process.platform === "win32" ? "kdtl_scenario.exe" : "kdtl_scenario";
const binary = join(root, "build", binaryName);
const cache = new Map();
let built = false;

function ensureBuilt() {
    if (existsSync(binary)) {
        built = true;
        return;
    }
    execFileSync(process.execPath, ["scripts/build-native.mjs"], {
        cwd: root,
        stdio: "pipe",
        encoding: "utf8",
        timeout: 120_000,
    });
    built = true;
}

export function scenario(name) {
    if (!cache.has(name)) {
        ensureBuilt();
        const stdout = execFileSync(binary, [name], {
            cwd: root,
            encoding: "utf8",
            timeout: 120_000,
        });
        cache.set(name, JSON.parse(stdout));
    }
    return cache.get(name);
}

export function assertDigest(value) {
    assert.equal(typeof value, "string");
    assert.match(value, /^[0-9a-f]{64}$/u);
}

export function assertAccount(view, account, liquid, locked = 0) {
    assert.equal(view.account, account);
    assert.equal(view.asset, "USDC");
    assert.equal(view.liquid, liquid);
    assert.equal(view.locked, locked);
}

export function assertOpenRisk(risk, indexed, visible, openPositions) {
    assert.equal(risk.operator_id, "operator-keystone");
    assert.equal(risk.asset, "USDC");
    assert.equal(risk.indexed_debt, indexed);
    assert.equal(risk.visible_reserved, visible);
    assert.equal(risk.claimable_open, visible);
    assert.equal(risk.open_positions, openPositions);
    assert.equal(risk.within_policy, true);
}

export function assertReconciliation(view, locked, openPositions, terminalPositions = 0) {
    assert.equal(view.asset, "USDC");
    assert.equal(view.liquid_total + view.locked_total, view.accounted_total);
    assert.equal(view.accounted_total, 1_000_000);
    assert.equal(view.locked_total, locked);
    assert.equal(view.open_positions, openPositions);
    assert.equal(view.terminal_positions, terminalPositions);
    assert.equal(view.conserved, true);
}
