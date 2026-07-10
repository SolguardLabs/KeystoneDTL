import { readdirSync, statSync } from "node:fs";
import { join } from "node:path";
import { spawnSync } from "node:child_process";

const roots = ["scripts", "tests"];
const files = [];

function walk(dir) {
    for (const entry of readdirSync(dir)) {
        const path = join(dir, entry);
        const stat = statSync(path);
        if (stat.isDirectory()) {
            walk(path);
        } else if (/\.(mjs|js)$/u.test(entry)) {
            files.push(path);
        }
    }
}

for (const root of roots) {
    walk(root);
}

for (const file of files) {
    const result = spawnSync(process.execPath, ["--check", file], { stdio: "inherit" });
    if (result.status !== 0) {
        process.exit(result.status ?? 1);
    }
}

console.log(`Checked ${files.length} JavaScript files.`);
