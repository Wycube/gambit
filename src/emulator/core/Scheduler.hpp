#pragma once

#include "emulator/core/debug/Debugger.hpp"
#include "common/Types.hpp"
#include <functional>


namespace emu {

using EventFunc = std::function<void (u64, u64)>;
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
    void addEvent(const EventHandle handle, EventFunc callback, u64 cycles_from_now);
    void removeEvent(const EventHandle handle);

    void step(u32 cycles);
    void runToNext();
    auto nextEventTime() -> u64;
    auto getCurrentTimestamp() -> u64;

    void attachDebugger(dbg::Debugger &debugger);

private:

    //Using a 64-bit unsigned integer, means that the scheduler's global
    //timestamp should not overflow for ~35,000 years at 100% speed.
    //Since I don't think fast forwarding will affect that time very much
    //it is good enough for me.
    std::vector<Event> m_events;
    u64 m_current_timestamp;
    u32 m_handle_counter;
};

} //namespace emu