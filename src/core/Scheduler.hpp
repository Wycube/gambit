#pragma once

#include "common/Types.hpp"
#include "core/debug/Debugger.hpp"

#include <functional>


namespace emu {

using EventFunc = std::function<void (u32, u32)>;

struct Event {
    std::string debug_tag;
    EventFunc callback;
    u32 scheduled_timestamp;
};

class Scheduler {
private:

    std::vector<Event> m_events;
    u32 m_current_timestamp;
public:

    Scheduler();

    void reset();

    void addEvent(std::string debug_tag, EventFunc callback, u32 cycles_from_now);
    //auto removeEvent(Event event) -> bool;

    void step(u32 cycles);
    auto getCurrentTimestamp() -> u32;

    void attachDebugger(dbg::Debugger &debugger);
};

} //namespace emu