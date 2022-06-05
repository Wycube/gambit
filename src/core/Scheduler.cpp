#include "Scheduler.hpp"
#include "common/Log.hpp"

#include <algorithm>


namespace emu {

Scheduler::Scheduler() { }

void Scheduler::reset() {
    m_events.clear();
}

void Scheduler::addEvent(std::string debug_tag, EventFunc callback, u32 cycles_from_now) {
    m_events.push_back(Event{debug_tag, callback, m_current_timestamp + cycles_from_now});
    std::sort(m_events.begin(), m_events.end(), [](const Event &a, const Event &b) {
        return a.scheduled_timestamp >= b.scheduled_timestamp;
    });
}

void Scheduler::step(u32 cycles) {
    //TODO: Prevent overflow, possibly elsewhere
    m_current_timestamp += cycles;
    bool events_to_run = true;

    LOG_DEBUG("Scheduler ticked {} cycles | Current Timestamp: {}", cycles, m_current_timestamp);

    while(events_to_run) {
        if(m_events.size() > 0 && m_events.back().scheduled_timestamp <= m_current_timestamp) {
            m_events.back().callback(m_current_timestamp, m_current_timestamp - m_events.back().scheduled_timestamp);
            m_events.pop_back();
        } else {
            events_to_run = false;
        }
    }
}

auto Scheduler::getCurrentTimestamp() -> u32 {
    return m_current_timestamp;
}

void Scheduler::attachDebugger(dbg::Debugger &debugger) {
    debugger.attachScheduler(&m_events, &m_current_timestamp);
}

} //namespace emu