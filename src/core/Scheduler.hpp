#pragma once

#include "common/Types.hpp"

#include <functional>


namespace emu {

using EventFunc = std::function<void (u32, u32)>;

struct Event {
    EventFunc callback;
    u32 scheduled_timestamp;
};

class Scheduler {
private:

    std::vector<Event> m_events;
    u32 m_current_timestamp;
public:

    Scheduler();

    void addEvent(EventFunc callback, u32 cycles_from_now);
    //auto removeEvent(Event event) -> bool;

    void step(u32 cycles);
};

} //namespace emu