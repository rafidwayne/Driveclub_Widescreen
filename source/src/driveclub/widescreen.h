// SPDX-FileCopyrightText: Copyright 2026 Driveclub-PC contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// driveclub-pc: automatic widescreen.
//
// Driveclub renders for a fixed 1920x1080 (16:9) video-out. When the resolution chosen in the
// launcher (the window size the emulator is started with) is wider than 16:9, three values in
// the game's eboot are rewritten right after it is loaded, before the patch XMLs are applied:
//
//   aspect ratio   float  16/9 -> out_width / 1080       (camera / projection aspect)
//   output width   u32    1920 -> out_width               (video-out width; height stays 1080)
//   FOV factor     float  0.5  -> out_width / 3840        (0.5 * aspect / (16/9), i.e. Hor+)
//
// The internal render resolution is deliberately left alone: the "internal" resolution patches
// need extra DMem and have a known depth-rendering bug, the output-only change has neither.
//
// The maths lives in this header (no dependencies besides types.h) so it can be unit tested.

#pragma once

#include "common/types.h"

namespace DriveclubPC::Widescreen {

/// Native video-out size of the game.
inline constexpr u32 kBaseWidth = 1920;
inline constexpr u32 kBaseHeight = 1080;

/// Largest video-out width that is applied (2560x1440 worth of pixels at a height of 1080).
/// Wider windows (e.g. 32:9) are clamped to this and end up with bars at the sides.
inline constexpr u32 kMaxWidth = 3412;

/// Sanity limit for the launcher / display size that is accepted.
inline constexpr s32 kMaxWindowDimension = 16384;

/// aspect = out_width / kBaseHeight and fov = out_width / kFovDivisor, where
/// kFovDivisor = kBaseHeight * (16/9) / 0.5 = 3840, so fov = 0.5 * aspect / (16/9).
inline constexpr float kFovDivisor = 3840.0f;

struct Params {
    u32 out_width;
    float aspect;
    float fov;
};

/// Computes the patch values for a window of `width` x `height` pixels.
/// Returns false when nothing should be changed (not wider than 16:9, or invalid).
/// All of this is integer arithmetic except the last two divisions, and it is mirrored 1:1 by the
/// machine-code hook in the binary-patched Driveclub-PC.exe.
[[nodiscard]] inline bool Compute(s32 width, s32 height, Params& out) {
    if (width <= 0 || height <= 0 || width > kMaxWindowDimension || height > kMaxWindowDimension) {
        return false;
    }
    // Strictly wider than 16:9: width / height > 16 / 9.
    if (static_cast<s64>(width) * 9 <= static_cast<s64>(height) * 16) {
        return false;
    }
    // out_width = round(kBaseHeight * width / height), forced even, clamped.
    u32 out_width = static_cast<u32>(
        (static_cast<u64>(kBaseHeight) * static_cast<u64>(width) + static_cast<u64>(height) / 2) /
        static_cast<u64>(height));
    out_width &= ~1u;
    if (out_width > kMaxWidth) {
        out_width = kMaxWidth;
    }
    if (out_width <= kBaseWidth) {
        return false;
    }
    out.out_width = out_width;
    out.aspect = static_cast<float>(out_width) / static_cast<float>(kBaseHeight);
    out.fov = static_cast<float>(out_width) / kFovDivisor;
    return true;
}

/// Applies the widescreen patch to the freshly loaded game executable.
/// `eboot_base` is the guest address the executable was loaded to (module base_virtual_addr),
/// `width`/`height` the launcher resolution (EmulatorSettings window size).
/// Does nothing if the window is not widescreen or the executable is not the expected
/// Driveclub 1.28 one (every location is checked for its original value first).
/// Returns true if the patch was applied.
bool Apply(VAddr eboot_base, s32 width, s32 height);

} // namespace DriveclubPC::Widescreen
