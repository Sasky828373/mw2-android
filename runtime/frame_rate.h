#pragma once
// MW2_FPS_LIMIT: how many frames a second the title draws at most.
//
// Unset or 60 is the console's pacing: D3D retires a frame at the display's
// blank and the title's own limiter (com_maxfps) is at its 60. Any other
// number takes the title off the blank the way the console's D3D offers it
// (d3d_hooks.cpp), with that number as com_maxfps; 0 is no limit. The title's
// limiter counts whole milliseconds a frame, so the rate it keeps is 1000
// over a whole number: 120 gives 125, 144 gives 166.
#include <algorithm>
#include <cstdint>
#include <chrono>
#include <thread>
#include "env.h"

namespace frame_rate
{
    constexpr uint32_t kConsole = 60;

    // com_maxfps: 0 is no limit, and 1000 is the most the title takes.
    inline uint32_t Limit()
    {
        static const uint32_t limit = uint32_t(std::min<uint64_t>(env::Number("MW2_FPS_LIMIT", 120), 1000));
        return limit;
    }

    // The title's millisecond-resolution com_maxfps rounds 120 to 125.
    // Pace the host's completed frames using nanosecond deadlines, without
    // spinning a CPU core. Reset deadlines after pauses or missed frames.
    inline void PaceHost()
    {
        const uint32_t fps = Limit();
        if (fps == 0 || fps == kConsole) return;
        using Clock = std::chrono::steady_clock;
        using Ns = std::chrono::nanoseconds;
        static thread_local Clock::time_point next{};
        const Ns period(1000000000ll / fps);
        const auto now = Clock::now();
        if (next.time_since_epoch().count() == 0 || now > next + period * 4)
        {
            next = now + period;
            return;
        }
        if (now < next)
            std::this_thread::sleep_until(next);
        const auto after = Clock::now();
        // Never create a burst of catch-up frames after the OS wakes us late.
        // Restart the cadence when a deadline has already passed.
        next = (after > next) ? after + period : next + period;
    }

    // Frames are retired at the blank, one a blank, as on the console.
    inline bool Console() { return Limit() == kConsole; }
}
