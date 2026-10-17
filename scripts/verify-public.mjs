import { readFileSync, readdirSync, statSync } from "node:fs";
import { extname, join, relative } from "node:path";

const root = process.cwd();
const roots = [
    "README.md",
    "SECURITY.md",
    "CHANGELOG.md",
    "CONTRIBUTING.md",
    "docs",
    "include",
    "src",
    "tests",
    "tools",
    "scripts",
];
const allowed = new Set([".c", ".h", ".js", ".json", ".md", ".mjs", ".yaml", ".yml"]);
const forbidden = [
    /\bctf\b/iu,
    /capture[\s-]+the[\s-]+flag/iu,
    /\bvulnerability\b/iu,
    /\bexploit\b/iu,
    /\blaboratorio\b/iu,
];
const credentials = [/BEGIN [A-Z ]+ PRIVATE KEY/u, /mnemonic\s*=/iu];
const findings = [];

function inspect(path) {
    if (path.endsWith("verify-public.mjs")) {
        return;
    }
    const text = readFileSync(path, "utf8");
    for (const pattern of [...forbidden, ...credentials]) {
        if (pattern.test(text)) {
            findings.push(`${relative(root, path)}: ${pattern}`);
        }
    }
}

function walk(path) {
    const stat = statSync(path);
    if (stat.isDirectory()) {
        for (const entry of readdirSync(path)) {
            walk(join(path, entry));
        }
        return;
    }
    if (allowed.has(extname(path))) {
        inspect(path);
    }
}

for (const entry of roots) {
    walk(join(root, entry));
}

if (findings.length > 0) {
    console.error(findings.join("\n"));
    process.exit(1);
}

console.log("Public-surface validation passed.");
