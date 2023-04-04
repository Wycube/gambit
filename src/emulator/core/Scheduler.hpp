#pragma once

#include "common/Types.hpp"
#include <functional>
#include <vector>


namespace emu {

using EventFunc = std::function<void (u64)>;
using EventHandle = u32;

struct Event {
    EventHandle handle;
    EventFunc callback;
    u64 scheduled_timestamp;
};

class GBA;

class Scheduler final {
public:

    Scheduler();

    void reset();

    auto generateHandle() -> EventHandle;
    void addEvent(const EventHandle handle, u64 cycles_from_now, EventFunc callback);
    void removeEvent(const EventHandle handle);

    void step(u32 cycles);
    void runToNext();
    auto nextEventTime() -> u64;
    auto getCurrentTimestamp() -> u64;

private:

    //Using a 64-bit unsigned integer, means that the scheduler's global
    //timestamp should not overflow for ~35,000 years at 100% speed,
    //which is good enough for me.
    std::vector<Event> events;
    u64 current_timestamp;
    u32 next_handle;
};

} //namespace emu