// Ported from Mutable Instruments stmlib
// Original: Copyright 2012 Emilie Gillet (MIT License)
// Modifications for BraidsVST: GPL v3

#pragma once

#include <cstdint>
#include <algorithm>

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&) = delete;    \
    TypeName& operator=(const TypeName&) = delete

#define CLIP(x) x = std::clamp(x, static_cast<decltype(x)>(-32768), static_cast<decltype(x)>(32767))

#define CONSTRAIN(x, min, max) x = std::clamp(x, static_cast<decltype(x)>(min), static_cast<decltype(x)>(max))
