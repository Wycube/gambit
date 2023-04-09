#pragma once

#include "common/Types.hpp"
#include "common/Heap.hpp"
#include <functional>
#include <vector>


namespace emu {

using EventFunc = std::function<void (u64)>;
using EventHandle = u32;

struct Event {
    EventHandle handle;
    u64 scheduled_timestamp;

    auto operator==(const Event &other) -> bool {
        return handle == other.handle;
    }

    auto operator<(const Event &other) -> bool {
        return scheduled_timestamp < other.scheduled_timestamp;
    }
};

class Scheduler final {
public:

    Scheduler();

    void reset();

    auto registerEvent(EventFunc callback) -> EventHandle;
    void addEvent(EventHandle handle, u64 cycles_from_now);
    void removeEvent(EventHandle handle);

    void step(u32 cycles);
    void runToNext();
    auto nextEventTime() -> u64;
    auto getCurrentTimestamp() -> u64;

private:

    //Using a 64-bit unsigned integer, means that the scheduler's global
    //timestamp should not overflow for ~35,000 years at 100% speed,
    //which is good enough for me.
    common::MinHeap<Event> events;
    std::vector<EventFunc> registered;
    u64 current_timestamp;
};

} //namespace emu