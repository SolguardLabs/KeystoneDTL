import { spawnSync } from "node:child_process";

const version = process.argv[2] ?? "v1.0.0";

function git(...args) {
    const result = spawnSync("git", args, {
        encoding: "utf8",
        stdio: ["ignore", "pipe", "inherit"],
    });
    if (result.status !== 0) {
        process.exit(result.status ?? 1);
    }
    return result.stdout.trim();
}

git("fetch", "--force", "--tags", "origin", "main", "production");

const refs = {
    head: git("rev-parse", "HEAD"),
    main: git("rev-parse", "origin/main"),
    production: git("rev-parse", "origin/production"),
    tag: git("rev-list", "-n", "1", version),
};

if (git("cat-file", "-t", version) !== "tag") {
    throw new Error(`${version} must be an annotated tag`);
}
if (new Set(Object.values(refs)).size !== 1) {
    throw new Error(`Release refs diverged: ${JSON.stringify(refs)}`);
}

console.log(`Release refs converge on ${refs.head}.`);
