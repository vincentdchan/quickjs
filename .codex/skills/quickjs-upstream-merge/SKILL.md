---
name: quickjs-upstream-merge
description: Merge upstream Bellard QuickJS into this fork while preserving local Windows/MSVC compatibility. Use when resolving upstream merge conflicts, comparing Makefile/CMake changes, deciding whether to keep or delete fork files, porting upstream build parameters into CMake, or fixing platform-specific CI failures on MSVC, MinGW/MSYS2, macOS, Linux, BSD, Cosmopolitan, or qemu targets.
---

# QuickJS Upstream Merge

Use this skill to merge upstream QuickJS into this fork with conservative conflict resolution and broad platform verification.

## Operating Principles

- Treat upstream as the source of truth for core QuickJS semantics, generated output, file layout, test expectations, and portable POSIX behavior.
- Preserve fork-specific compatibility only where it is still required for local goals, especially native MSVC builds.
- Prefer deleting fork files when upstream deleted the same feature or file and no current fork requirement remains.
- Keep upstream `Makefile` behavior intact unless the fork deliberately supports an additional build path.
- Backport relevant upstream `Makefile` parameters and definitions into `CMakeLists.txt` so CMake builds do not silently diverge.
- Use platform headers narrowly. Add Windows/MSVC compatibility headers at the call site that needs them; do not blanket-include compatibility shims across every compiler.
- Verify locally before committing, then use CI logs to fix the next actual failure rather than guessing from platform names.

## Initial Orientation

1. Inspect remotes and branches:

```powershell
git remote -v
git status --short --branch
git log --oneline --decorate -n 8
```

2. Fetch upstream and compare:

```powershell
git fetch upstream
git diff --stat HEAD..upstream/master
git diff --name-status HEAD..upstream/master
```

3. Identify fork-only files before merging:

```powershell
git ls-tree -r --name-only HEAD | Sort-Object > $env:TEMP\fork-files.txt
git ls-tree -r --name-only upstream/master | Sort-Object > $env:TEMP\upstream-files.txt
Compare-Object (Get-Content $env:TEMP\fork-files.txt) (Get-Content $env:TEMP\upstream-files.txt)
```

## Conflict Strategy

- For core engine files such as `quickjs.c`, `quickjs.h`, opcode tables, atom tables, Unicode, regexp, dtoa, and test baselines, start from upstream intent and reapply only required fork compatibility.
- For files upstream removed, remove them locally unless they are needed by a fork-only target. This applied to `libbf.c` and `libbf.h`.
- For `qjs.c` and command-line tools, follow upstream behavior first. Reintroduce Windows-specific includes or replacements only when the native MSVC build proves they are needed.
- For `cutils.h` and shared utility headers, merge rather than blindly replace when the fork added platform includes. Audit every Windows header: keep only headers used by active code or required declarations.
- For generated C output, match upstream exactly. Example: `qjsc.c` should emit `const uint8_t name[size] = { ... }`, not `extern const uint8_t ... = { ... }`; Clang with `-Werror` treats initialized `extern` definitions as errors.
- For `.gitignore`, prefer current build outputs and editor/tool noise, but avoid hiding source or test artifacts that upstream tracks.

## Build-System Rules

- Keep upstream `Makefile` as the primary mainstream build description.
- Mirror meaningful `Makefile` changes into `CMakeLists.txt`, including:
  - compile definitions such as `CONFIG_VERSION`, `CONFIG_BIGNUM`, `CONFIG_ATOMICS`, `CONFIG_WERROR`, `CONFIG_CHECK_JSVALUE`, or platform feature flags;
  - include paths and generated source dependencies;
  - tool targets such as `qjs`, `qjsc`, `run-test262`, static libraries, and examples;
  - optional sanitizer, LTO, and platform-specific settings when CMake supports the same mode.
- Do not make CMake invent behavior that upstream intentionally removed.
- Under MSVC, disable unsupported POSIX/pthread-dependent features first, then add compatibility only where required.
- Add `win_patch.c` or `win_patch.h` only for MSVC unless MinGW also fails without them. MinGW/MSYS2 normally has POSIX-like headers such as `unistd.h`, `dirent.h`, `ftw.h`, and often `sys/time.h`.

## Platform Notes

### MSVC

- Native MSVC is not upstream's main target. Expect to provide replacements for POSIX APIs such as `gettimeofday`, `usleep`, `sleep`, directory walking, `getpid`, and affinity helpers.
- Prefer `_MSC_VER` guards for MSVC-only compatibility.
- Include `<process.h>` and map `getpid` to `_getpid` under MSVC.
- Disable pthread-backed features under MSVC until a Windows implementation exists. Do not fake pthread availability.
- Treat Debug CRT popups seriously. Common causes:
  - `abort()` from QuickJS assertions can reveal type/layout mismatches;
  - stack corruption often means a Windows type mismatch, such as passing `uint32_t *` where upstream expects `size_t *`.
- Keep `CONFIG_CHECK_JSVALUE` compiling with `/WX`; it catches const-correctness and value conversion regressions early.

### MinGW/MSYS2

- MinGW is Windows but not MSVC. Avoid routing it through MSVC shims unless required.
- Use `_WIN32 && !_MSC_VER` when MinGW needs Windows-specific includes plus POSIX-like headers.
- Include `<windows.h>` in files that use `DWORD_PTR`, `GetProcessAffinityMask`, `GetCurrentProcess`, or `Sleep`.
- Include `<sys/time.h>` for `gettimeofday` where MinGW provides it.
- Watch GCC format checking. Some Windows CRT format behavior differs; avoid unnecessary `%zu` in Windows-only error paths if CI treats it as fatal.

### macOS

- macOS CI commonly exposes Clang warnings promoted to errors. Pay attention to initialized `extern`, format attributes, implicit declarations, and unused static functions.
- If macOS fails after a merge, compare the affected file directly with upstream before adding a local workaround. The right fix is often to match upstream output or signature.

### Linux, BSD, Cosmopolitan, qemu

- Preserve upstream POSIX paths. Do not include Windows compatibility headers on these platforms.
- FreeBSD and Cosmopolitan can expose assumptions hidden on Linux. Keep feature guards precise.
- qemu-alpine jobs are slow; wait for logs before acting. Architecture jobs often validate portability rather than build-system logic.

## Verification Sequence

Run targeted local checks first:

```powershell
cmake --build build-msvc --config Debug --parallel -- /clp:ErrorsOnly
cl /WX /DCONFIG_CHECK_JSVALUE /D_CRT_SECURE_NO_WARNINGS /I. /c quickjs.c
cl /WX /DCONFIG_CHECK_JSVALUE /D_CRT_SECURE_NO_WARNINGS /I. /c qjs.c
cl /WX /DCONFIG_CHECK_JSVALUE /D_CRT_SECURE_NO_WARNINGS /I. /c qjsc.c
```

Run executable tests when local policy allows `qjs.exe` to execute. If Windows Application Control or Defender blocks the executable, report that as an environment block, not a QuickJS test failure.

After pushing, inspect GitHub Actions failures with `gh`:

```powershell
gh run list --repo vincentdchan/quickjs --limit 5
gh run view <run-id> --repo vincentdchan/quickjs --json status,conclusion,url,jobs
gh api /repos/vincentdchan/quickjs/actions/jobs/<job-id>/logs
```

Fix one class of CI failure at a time, commit with a narrow message, and push again.

## Known Failure Patterns From This Merge

- `CONFIG_CHECK_JSVALUE` const failures: add a narrow helper such as `JS_VALUE_CONST_TO_VALUE` and fix signatures or casts at ownership boundaries, rather than weakening checks globally.
- `qjsc.c` initialized `extern`: match upstream and emit plain `const`.
- MinGW implicit declarations: include `<process.h>`, `<sys/time.h>`, or `<windows.h>` under the right compiler guard.
- MSVC `abort()` during startup: check bit-field signedness and enum storage assumptions.
- MSVC stack corruption around a variable name in the CRT dialog: inspect pointer type sizes at API boundaries, especially `size_t *` versus `uint32_t *`.

## Commit Hygiene

- Do not commit until local build checks pass or the remaining blocker is clearly environmental.
- Keep merge-resolution commits separate from follow-up CI fix commits when possible.
- Before committing, run:

```powershell
git diff --check
git status --short
```

- In final reports, distinguish:
  - upstream-alignment fixes;
  - fork compatibility fixes;
  - local environment limitations;
  - CI status and run URL.
