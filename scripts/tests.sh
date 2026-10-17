#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

node scripts/check-js.mjs
node scripts/verify-public.mjs
node scripts/build-native.mjs
node --test "tests/node/*.test.js"
