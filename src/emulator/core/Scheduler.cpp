#include "Scheduler.hpp"
#include <algorithm>


namespace emu {

Scheduler::Scheduler() { }

void Scheduler::reset() {
    m_events.clear();
}

void Scheduler::addEvent(const std::string &tag, EventFunc callback, u32 cycles_from_now) {
    m_events.push_back(Event{tag, callback, m_current_timestamp + cycles_from_now});
    std::sort(m_events.begin(), m_events.end(), [](const Event &a, const Event &b) {
        return a.scheduled_timestamp > b.scheduled_timestamp;
    });
}

void Scheduler::removeEvent(const std::string &tag) {
    for(auto iter = m_events.begin(); iter != m_events.end(); iter++) {
        if(iter->tag == tag) {
            m_events.erase(iter);
            break;
        }
    }
}

void Scheduler::step(u32 cycles) {
    //TODO: Prevent overflow, possibly elsewhere
    m_current_timestamp += cycles;
    bool events_to_run = true;

    while(events_to_run) {
        if(m_events.size() > 0 && m_events.back().scheduled_timestamp <= m_current_timestamp) {
            m_events.back().callback(m_current_timestamp, m_current_timestamp - m_events.back().scheduled_timestamp);
            m_events.pop_back();
        } else {
            events_to_run = false;
        }
    }
}

void Scheduler::runToNext() {
    if(m_events.empty()) {
        return;
    }

    step(m_events.back().scheduled_timestamp - m_current_timestamp);
}

auto Scheduler::nextEventTime() -> u32 {
    if(m_events.empty()) {
        return 0;
    }

    return m_events.back().scheduled_timestamp;
}

auto Scheduler::getCurrentTimestamp() -> u32 {
    return m_current_timestamp;
}

void Scheduler::attachDebugger(dbg::Debugger &debugger) {
    debugger.attachScheduler(&m_events, &m_current_timestamp);
}

} //namespace emu