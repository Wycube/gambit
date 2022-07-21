#pragma once

#include "emulator/core/debug/Debugger.hpp"
#include "common/Types.hpp"
#include <functional>


namespace emu {

using EventFunc = std::function<void (u32, u32)>;

struct Event {
    std::string tag;
    EventFunc callback;
    u32 scheduled_timestamp;
};

class Scheduler {
public:

    Scheduler();

    void reset();

    void addEvent(const std::string &tag, EventFunc callback, u32 cycles_from_now);
    void removeEvent(const std::string &tag);

    void step(u32 cycles);
    void runToNext();
    auto nextEventTime() -> u32;
    auto getCurrentTimestamp() -> u32;

    void attachDebugger(dbg::Debugger &debugger);

private:

    std::vector<Event> m_events;
    u32 m_current_timestamp;
};

} //namespace emu