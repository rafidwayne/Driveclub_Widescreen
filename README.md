# Driveclub-PC: automatic widescreen

Any launcher resolution wider than 16:9 now gets a matching aspect ratio, video-out width
and field of view. Resolutions that are 16:9 or narrower behave exactly as before.

This package contains:

| File | What it is |
| --- | --- |
| `Driveclub-PC.exe` | Build 0.1 with the feature added by patching the binary directly (for testing now) |
| `source/src/driveclub/widescreen.h` | The maths: which values to write for a given resolution |
| `source/src/driveclub/widescreen.cpp` | Applies the values to the loaded game executable |
| `source/tests/test_driveclub_widescreen.cpp` | Unit tests for the maths |
| `widescreen-integration.patch` | Everything above wired into the 0.1 source tree (`patch -p1`) |

## Behaviour

The launcher saves the chosen resolution as the window size (for "Display resolution" it saves
the real display size). When the game's executable has been loaded, that size is read and:

| Value in the game | Original | With widescreen |
| --- | --- | --- |
| Camera aspect ratio (float) | 16/9 | `out_width / 1080` |
| Video-out width (u32) | 1920 | `out_width` (height stays 1080) |
| FOV factor (float) | 0.5 | `out_width / 3840` (= `0.5 * aspect / (16/9)`, Hor+) |

`out_width = round(1080 * width / height)`, forced to an even number, clamped to 3412.

| Launcher resolution | Video-out | Aspect | FOV factor |
| --- | --- | --- | --- |
| 1920x1080, 2560x1440, 3840x2160 | unchanged | 16:9 | 0.5 |
| 1920x1200 (16:10) | unchanged | | |
| 2376x1080 | 2376x1080 | 2.2 | 0.61875 |
| 2560x1080 | 2560x1080 | 2.370 | 0.66667 |
| 3440x1440 | 2580x1080 | 2.389 | 0.67188 |
| 5120x1440 (32:9) | 3412x1080 (clamped) | 3.159 | 0.88854 |

Notes:

* **Output only.** The game's internal render resolution is left at 1920x1080 and its output
  buffer is widened. The "internal" resolution patches in the community patch XML need extra
  DMem and are marked "[DEPTH BUG]", so they are deliberately not used. Image quality is
  therefore that of a 1080-line render stretched horizontally by the output stage; the frame
  keeps the right proportions and the host window shows it without bars or stretching.
* **Clamp.** 3412 wide at 1080 high is the pixel count of 2560x1440. Anything wider (e.g. 32:9)
  is clamped and shows bars at the sides. Change `kMaxWidth` if more is wanted and tested.
* **Only the expected executable is patched.** Each of the three locations is checked for its
  original value first; if any differs (other game version, another patch already applied) nothing
  is written. Driveclub CUSA00093, APP_VER 01.28 is the only version checked.
* **Order.** It runs before the patch XMLs, so a user's own XML patches still win.
* **FOV.** The formula reproduces the value used by the community "FOV fix for wide
  resolutions" XML at 21:9 (0.65625 = 0.5 * (21/9) / (16/9)) and scales it linearly with the
  aspect. Whether that is the most pleasing FOV for every ratio is a taste question; the single
  place to change it is `Params::fov` in `widescreen.h`.

## For the source tree

```
cd Driveclub-pc-wrapper-0.1
patch -p1 < widescreen-integration.patch
```

The patch:

* adds `src/driveclub/widescreen.{h,cpp}` and registers them in `CMakeLists.txt`;
* calls `DriveclubPC::Widescreen::Apply(base_virtual_addr, EmulatorSettings.GetWindowWidth(),
  EmulatorSettings.GetWindowHeight())` in `src/core/module.cpp`, right before
  `MemoryPatcher::OnGameLoaded()` for the main executable;
* adds `3440x1440`, `2560x1080` and `2376x1080` to the launcher's resolution list;
* adds the unit test target `shadps4_driveclub_widescreen_test`.

The addresses are offsets from the executable's load address (the XML patch address minus
`0x400000`): aspect `0x729978`, output width `0x754c1b` (immediate of a `mov`), FOV `0x12efac4`.

## How the supplied `Driveclub-PC.exe` was made

There was no way to compile the source for this package, so the same logic was added to the 0.1
binary as two small hooks (machine code in the unused padding at the end of `.text`):

1. **`WindowSDL` constructor (RVA `0xcd8060`).** The first 5 bytes (`push rbp; push r15; push
   r14`) are replaced by a jump to a stub that stores the `width`/`height` arguments
   (`edx`/`r8d`) in two ints, then runs the displaced instructions and jumps back.
2. **`MemoryPatcher::OnGameLoaded` (RVA `0xc0d080`).** Same 5 bytes, replaced by a jump to a stub
   that does the same calculation as `Compute()` + `Apply()` above, reading the base address from
   the `g_eboot_address` global (RVA `0x49a4058`), then runs the displaced instructions and
   jumps back. Registers it uses are saved and restored.

Other changes: the `.text` section's virtual size is raised to its raw size (`0x14e0600`) so the
stub area is mapped, the two ints live at RVA `0x4f3bf00`/`0x4f3bf04` (zero-filled tail of the
last `.data` page), and the launcher's resolution table (two copies) has `2560x1600`,
`1920x1200` and `1600x900` replaced by `3440x1440`, `2560x1080` and `2376x1080`. 297 bytes
differ from the original; nothing else was touched.

## What was and was not tested

* Tested: the maths (unit tests, all pass); the exe's machine code was run in an x86-64
  emulator for 392 resolution combinations (including invalid and out-of-range ones) and
  produces bit-identical aspect/width/FOV values to the C++ `Compute()`; registers and stack are
  preserved; nothing is written when the base pointer is null or the original bytes differ.
  `widescreen.cpp` compiles (checked with a stubbed logger), and the patch applies cleanly to
  the 0.1 tree.
* Reported working in the game: the earlier, fixed-2376x1080 version of this exact mechanism
  (same three values, same locations) was run on the real game at 2376x1080 and works.
* **Not tested:** the generalized exe has not been run in the game on Windows by me, and the
  source has not been built in the full project (no toolchain here). Other aspect ratios than
  2.2 are untested in the game, in particular how the HUD and menus look at very wide ratios.
