# Kameo

Static recompilation of **Kameo: Elements of Power** (Xbox 360) for Windows
and Linux, built on the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk).

This project converts the Xbox 360 PowerPC `default.xex` into native x86_64
code at build time, then wraps it with a small host runtime (logging,
overlays, hooks) so the game runs natively and can be modded like a PC port.

> **You must own the game.** This project does **not** ship any Kameo code,
> data, or assets. You provide your own legally dumped `kameo.iso`.

## Status

The game runs end-to-end on both Windows and Linux:

- `rexglue codegen` produces recompiled C++ across ~15,000 functions with
  zero unresolved imports.
- Builds to a native `kameo.exe` (Windows) or `kameo` (Linux).
- Full gameplay is reachable. Audio, graphics, and input work.
- Language selection via `--user_language <id>` (see [Running](#running)).
- Linux: SSE FP exception masking fixed via shadowed `rex/platform/fpscr.h`.

## Prerequisites

- **OS**: Windows 10/11 or Linux (x86_64)
- **Clang**: 16+
- **CMake + Ninja**: 3.25+
- **ReXGlue SDK**: nightly `nightly-20260425-711d4d33` or newer
- **extract-xiso**: to extract `kameo.iso`

### Linux (Arch/CachyOS)
```bash
paru -S clang cmake ninja vulkan-headers extract-xiso-git
```

### Windows
```powershell
scoop install llvm cmake ninja
```

## Building from scratch

### 1. Clone

```bash
git clone https://github.com/birabittoh/kameo.git
cd kameo
```

### 2. Build and install the ReXGlue SDK

```bash
git clone https://github.com/rexglue/rexglue-sdk.git
cd rexglue-sdk
git checkout nightly-20260425-711d4d33
cmake --preset linux-amd64   # or win-amd64 on Windows
cmake --build out/build/linux-amd64 --config Release -- -j$(nproc)
cmake --install out/build/linux-amd64 --config Release --prefix out/install/linux-amd64
cd ..
```

### 3. Provide your game

Extract your legally dumped ISO directly into `assets/`:

```bash
extract-xiso -x "Kameo - Elements of Power (USA).iso" -d assets/
```

`assets/default.xex` must exist before running codegen.

### 4. Run codegen

```bash
/path/to/rexglue-sdk/out/install/linux-amd64/bin/rexglue codegen kameo_config.toml
```

### 5. Run migrate (generates `generated/rexglue.cmake`)

```bash
/path/to/rexglue-sdk/out/install/linux-amd64/bin/rexglue migrate --app_root .
```

### 6. Build

```bash
cmake --preset linux-amd64-release \
  -DCMAKE_PREFIX_PATH="/path/to/rexglue-sdk/out/install/linux-amd64"
cmake --build out/build/linux-amd64-release -- -j$(nproc)
```

Symlink assets into the build output so the binary can find them:

```bash
ln -sf "$PWD/assets" out/build/linux-amd64-release/assets
```

## Running

```bash
./out/build/linux-amd64-release/kameo
```

### Language selection

The game defaults to English. Pass `--user_language <id>` to switch:

| ID | Language   |
|----|------------|
| 1  | English    |
| 2  | Japanese   |
| 3  | German     |
| 4  | French     |
| 5  | Spanish    |
| 6  | Italian    |
| 7  | Korean     |

```bash
./out/build/linux-amd64-release/kameo --user_language 6
```

### GPU selection

If you have multiple GPUs, force a specific one:

```bash
./out/build/linux-amd64-release/kameo --vulkan_device 1
```

List available devices by running without the flag — they are printed to
the log on startup.

### Logging

```bash
./out/build/linux-amd64-release/kameo --log_file kameo.log --log_level debug
```

## Repo layout

```
kameo/
├── assets/              ← extracted ISO contents (gitignored)
├── generated/           ← produced by rexglue codegen (gitignored)
├── icon/                ← app.ico (embedded as window icon at runtime)
├── rex/platform/        ← shadowed fpscr.h (fixes Linux SSE FP exceptions)
├── src/
│   ├── main.cpp
│   ├── kameo_app.h      ← ReXApp subclass: language redirect, icon, hooks
│   ├── kameo_hooks.h
│   ├── kameo_hooks.cpp  ← midasm hooks: DLC, infinite energy/health, etc.
│   └── kameo_dlc_swap.h
├── .github/             ← CI and release workflows
├── CMakeLists.txt
├── CMakePresets.json
└── kameo_config.toml    ← codegen config (functions, midasm hooks, rexcrt)
```

## Adding a hook

1. Find the guest address in `default.xex`.
2. Add to `kameo_config.toml`:

   ```toml
   [functions]
   0x8XXXXXXX = {name = "MyFunction"}
   ```

3. Implement in `src/kameo_hooks.cpp`:

   ```cpp
   void MyFunction(PPCContext& ctx, uint8_t* base) {
       // your logic
   }
   ```

4. Re-run codegen and rebuild.

## Adding a midasm hook (inline patch)

```toml
[[midasm_hook]]
address = 0x8XXXXXXX
name = "MyHook"
registers = ["r3"]
return = true
```

Implement in `src/kameo_hooks.cpp`:

```cpp
void MyHook(PPCRegister& r3) {
    r3.u32 = 1;
}
```

## XEX overview (Kameo: Elements of Power, USA)

| Field | Value |
|---|---|
| Load base | `0x82000000` |
| Entry point | `0x8251F320` |
| Image size | `0x00BF0000` (~12 MB loaded) |
| Imports | `xboxkrnl.exe` (148 functions, 8 data), `xam.xex` (65 functions, 2 data) |
| Decoded instructions | 1,621,549 across 2,141 code regions |
| Discovered functions | ~15,000 (after call-graph, PDATA, vtable, gap-fill) |

## Credits

- [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk)
- [MaxDeadBear](https://github.com/MaxDeadBear)
- [BiRabittoh](https://github.com/birabittoh)

## License

The host-side source in `src/`, build scripts, and CI config are available
under the MIT License.

The recompiled game code produced at build time contains symbols and logic
from Kameo: Elements of Power and is **not** redistributable. Do not share
`default.xex`, the `generated/` directory, or any built binary that links
against them.
