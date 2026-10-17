#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

if command -v clang-format >/dev/null 2>&1; then
    clang-format --dry-run --Werror include/*.h src/*.c src/*.h tools/*.c
fi

node scripts/check-js.mjs
node_modules/.bin/prettier --check --config .prettierrc --ignore-path .prettierignore "**/*.{js,mjs,json,md,yml,yaml}"
node scripts/verify-public.mjs
node scripts/build-native.mjs
node --test "tests/node/*.test.js"
