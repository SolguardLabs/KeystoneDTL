import { existsSync, mkdirSync, readdirSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { spawnSync } from "node:child_process";

const root = dirname(dirname(fileURLToPath(import.meta.url)));
const buildDir = join(root, "build");
const isWindows = process.platform === "win32";
const output = join(buildDir, isWindows ? "kdtl_scenario.exe" : "kdtl_scenario");

const sources = [
    "src/kdtl_accounts.c",
    "src/kdtl_amount.c",
    "src/kdtl_catalog.c",
    "src/kdtl_codec.c",
    "src/kdtl_engine.c",
    "src/kdtl_events.c",
    "src/kdtl_ids.c",
    "src/kdtl_limits.c",
    "src/kdtl_positions.c",
    "src/kdtl_policy.c",
    "src/kdtl_reconcile.c",
    "src/kdtl_reporting.c",
    "src/kdtl_settlement.c",
    "src/kdtl_stress.c",
    "tools/kdtl_scenario.c",
].map((path) => join(root, path));

function commandExists(command, args = ["--version"]) {
    const result = spawnSync(command, args, { cwd: root, encoding: "utf8", shell: false });
    return result.status === 0;
}

function windowsCompilerCandidates() {
    if (process.platform !== "win32") {
        return [];
    }

    const candidates = [];
    const localPrograms = process.env.LOCALAPPDATA
        ? join(process.env.LOCALAPPDATA, "Programs")
        : "";
    const roots = [
        process.env.ProgramFiles ? join(process.env.ProgramFiles, "LLVM", "bin", "clang.exe") : "",
        process.env["ProgramFiles(x86)"]
            ? join(process.env["ProgramFiles(x86)"], "LLVM", "bin", "clang.exe")
            : "",
    ];

    for (const candidate of roots) {
        if (candidate && existsSync(candidate)) {
            candidates.push(candidate);
        }
    }

    if (localPrograms && existsSync(localPrograms)) {
        for (const entry of readdirSync(localPrograms)) {
            if (!entry.startsWith("LLVM")) {
                continue;
            }
            const candidate = join(localPrograms, entry, "bin", "clang.exe");
            if (existsSync(candidate)) {
                candidates.push(candidate);
            }
        }
    }

    return candidates;
}

function visualStudioEnvironment() {
    if (!isWindows) {
        return null;
    }

    const vswhereCandidates = [
        process.env["ProgramFiles(x86)"]
            ? join(
                  process.env["ProgramFiles(x86)"],
                  "Microsoft Visual Studio",
                  "Installer",
                  "vswhere.exe",
              )
            : "",
        process.env.ProgramFiles
            ? join(process.env.ProgramFiles, "Microsoft Visual Studio", "Installer", "vswhere.exe")
            : "",
    ];

    for (const vswhere of vswhereCandidates) {
        if (!vswhere || !existsSync(vswhere)) {
            continue;
        }
        const result = spawnSync(
            vswhere,
            [
                "-latest",
                "-products",
                "*",
                "-requires",
                "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                "-property",
                "installationPath",
            ],
            { cwd: root, encoding: "utf8", shell: false },
        );
        const installation = result.status === 0 ? result.stdout.trim() : "";
        const vcvars = installation
            ? join(installation, "VC", "Auxiliary", "Build", "vcvars64.bat")
            : "";
        if (vcvars && existsSync(vcvars)) {
            return vcvars;
        }
    }

    return null;
}

function pickCompiler() {
    const requested = process.env.CC;
    const candidates = requested
        ? [requested]
        : ["cc", "gcc", "clang", ...windowsCompilerCandidates()];
    for (const candidate of candidates) {
        if (commandExists(candidate)) {
            return { kind: "gnu", command: candidate };
        }
    }

    const vcvars = visualStudioEnvironment();
    if (vcvars) {
        return { kind: "msvc", command: "cl", vcvars };
    }
    throw new Error("No se encontro un compilador C compatible. Define CC o instala gcc/clang/cc.");
}

mkdirSync(buildDir, { recursive: true });
const compiler = pickCompiler();
let result;

if (compiler.kind === "msvc") {
    const quotedSources = sources.map((source) => `"${source}"`).join(" ");
    const compile = [
        "cl /nologo /std:c11 /W4 /WX /O2 /D_CRT_SECURE_NO_WARNINGS",
        "/Iinclude /Isrc",
        `/Fo"${buildDir}\\\\"`,
        quotedSources,
        `/Fe:"${output}"`,
    ].join(" ");
    const command = `call "${compiler.vcvars}" >nul && ${compile}`;
    result = spawnSync(command, {
        cwd: root,
        stdio: "inherit",
        shell: true,
    });
} else {
    const args = [
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-pedantic",
        "-O2",
        "-Iinclude",
        "-Isrc",
        ...sources,
        "-o",
        output,
    ];
    result = spawnSync(compiler.command, args, { cwd: root, stdio: "inherit", shell: false });
}
if (result.status !== 0) {
    process.exit(result.status ?? 1);
}

console.log(output);
