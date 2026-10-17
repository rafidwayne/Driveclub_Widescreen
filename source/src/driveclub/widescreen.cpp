// SPDX-FileCopyrightText: Copyright 2026 Driveclub-PC contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <bit>
#include <cstring>

#include "common/logging/log.h"
#include "driveclub/widescreen.h"

namespace DriveclubPC::Widescreen {

namespace {

// Offsets from the executable's load address (= the patch XML address minus 0x400000).
// Driveclub CUSA00093, APP_VER 01.28.
constexpr VAddr kAspectOffset = 0x729978; // float, camera aspect ratio (16/9)
constexpr VAddr kWidthOffset = 0x754c1b;  // u32 immediate, video-out width (1920)
constexpr VAddr kFovOffset = 0x12efac4;   // float, FOV factor (0.5)

constexpr u32 kOriginalAspect = std::bit_cast<u32>(16.0f / 9.0f); // 0x3FE38E39
constexpr u32 kOriginalFov = std::bit_cast<u32>(0.5f);            // 0x3F000000

u32 Read32(VAddr addr) {
    u32 value;
    std::memcpy(&value, reinterpret_cast<const void*>(addr), sizeof(value));
    return value;
}

void Write32(VAddr addr, u32 value) {
    std::memcpy(reinterpret_cast<void*>(addr), &value, sizeof(value));
}

} // namespace

bool Apply(VAddr eboot_base, s32 width, s32 height) {
    Params params{};
    if (eboot_base == 0 || !Compute(width, height, params)) {
        return false;
    }
    if (Read32(eboot_base + kAspectOffset) != kOriginalAspect ||
        Read32(eboot_base + kWidthOffset) != kBaseWidth ||
        Read32(eboot_base + kFovOffset) != kOriginalFov) {
        LOG_WARNING(Loader,
                    "Driveclub-PC widescreen: unexpected executable contents, patch skipped");
        return false;
    }
    Write32(eboot_base + kAspectOffset, std::bit_cast<u32>(params.aspect));
    Write32(eboot_base + kWidthOffset, params.out_width);
    Write32(eboot_base + kFovOffset, std::bit_cast<u32>(params.fov));
    LOG_INFO(Loader, "Driveclub-PC widescreen: {}x{} -> video-out {}x{}, aspect {:.4f}, fov {:.5f}",
             width, height, params.out_width, kBaseHeight, params.aspect, params.fov);
    return true;
}

} // namespace DriveclubPC::Widescreen
