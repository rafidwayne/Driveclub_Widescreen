// SPDX-FileCopyrightText: Copyright 2026 Driveclub-PC contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <bit>
#include <gtest/gtest.h>

#include "driveclub/widescreen.h"

namespace {

using namespace DriveclubPC::Widescreen;

TEST(DriveclubWidescreen, NotWiderThan16x9IsIgnored) {
    Params p{};
    EXPECT_FALSE(Compute(1920, 1080, p));
    EXPECT_FALSE(Compute(2560, 1440, p));
    EXPECT_FALSE(Compute(3840, 2160, p));
    EXPECT_FALSE(Compute(1920, 1200, p)); // 16:10
    EXPECT_FALSE(Compute(1280, 1024, p));
}

TEST(DriveclubWidescreen, InvalidSizesAreIgnored) {
    Params p{};
    EXPECT_FALSE(Compute(0, 0, p));
    EXPECT_FALSE(Compute(2376, 0, p));
    EXPECT_FALSE(Compute(-2376, 1080, p));
    EXPECT_FALSE(Compute(100000, 1080, p));
}

TEST(DriveclubWidescreen, Ratio2_2) {
    Params p{};
    ASSERT_TRUE(Compute(2376, 1080, p));
    EXPECT_EQ(p.out_width, 2376u);
    EXPECT_EQ(std::bit_cast<u32>(p.aspect), 0x400CCCCDu); // 2.2f
    EXPECT_EQ(std::bit_cast<u32>(p.fov), 0x3F1E6666u);    // 0.61875f
}

TEST(DriveclubWidescreen, Ultrawide21x9) {
    Params p{};
    ASSERT_TRUE(Compute(2560, 1080, p));
    EXPECT_EQ(p.out_width, 2560u);
    ASSERT_TRUE(Compute(3440, 1440, p));
    EXPECT_EQ(p.out_width, 2580u); // 1080 * 3440 / 1440
    EXPECT_FLOAT_EQ(p.aspect, 2580.0f / 1080.0f);
}

TEST(DriveclubWidescreen, FovFollowsAspect) {
    // fov = 0.5 * aspect / (16/9)
    Params p{};
    for (s32 w : {2000, 2376, 2560, 3000, 3412}) {
        ASSERT_TRUE(Compute(w, 1080, p));
        EXPECT_NEAR(p.fov, 0.5f * p.aspect / (16.0f / 9.0f), 1e-5f);
    }
}

TEST(DriveclubWidescreen, SuperUltrawideIsClamped) {
    Params p{};
    ASSERT_TRUE(Compute(5120, 1440, p)); // 32:9
    EXPECT_EQ(p.out_width, kMaxWidth);
    ASSERT_TRUE(Compute(7680, 1080, p));
    EXPECT_EQ(p.out_width, kMaxWidth);
}

TEST(DriveclubWidescreen, OutputWidthIsEven) {
    Params p{};
    for (s32 w = 1921; w < 3500; ++w) {
        if (Compute(w, 1080, p)) {
            EXPECT_EQ(p.out_width % 2, 0u);
        }
    }
}

} // namespace
