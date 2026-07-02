
# QuickJS CMake

A fork of Bellard's QuickJS.

CMake support of QuickJS, use submodule to the official mirror.

Available on Windows(32-bit) **without** mingw.

# Nodex extensions

This fork exposes a small `JS_CaptureCallSites(ctx, skip_count)` C API so embedders such as Nodex can capture structured stack frames without parsing formatted `Error.stack` strings. The API returns frame metadata such as function name, filename, line/column, and whether the frame is native; higher-level Node/V8-compatible `CallSite` behavior remains implemented by the embedder.

# Usage

Clone the repo:

```shell
git clone https://github.com/vincentdchan/quickjs.git
```

or use this repo as submodule

```shell
git submodule add https://github.com/vincentdchan/quickjs.git
git submodule update --init --recursive
```

# Targets

- libquickjs
- qjs
- qjsc
- run-test262
