#include "Scheduler.hpp"
#include <algorithm>


namespace emu {

Scheduler::Scheduler() {
    reset();
}

void Scheduler::reset() {
    m_current_timestamp = 0;
    m_next_handle = 1;
    m_events.clear();
}

auto Scheduler::generateHandle() -> EventHandle {
    return m_next_handle++;
}

void Scheduler::addEvent(const EventHandle handle, EventFunc callback, u64 cycles_from_now) {
    m_events.push_back(Event{handle, callback, m_current_timestamp + cycles_from_now});
    std::sort(m_events.begin(), m_events.end(), [](const Event &a, const Event &b) {
        return a.scheduled_timestamp > b.scheduled_timestamp;
    });
}

void Scheduler::removeEvent(const EventHandle handle) {
    for(auto iter = m_events.begin(); iter != m_events.end(); iter++) {
        if(iter->handle == handle) {
            m_events.erase(iter);
            break;
        }
    }
}

void Scheduler::step(u32 cycles) {
    m_current_timestamp += cycles;

    while(true) {
        if(m_events.size() > 0 && m_events.back().scheduled_timestamp <= m_current_timestamp) {
            m_events.back().callback(m_current_timestamp - m_events.back().scheduled_timestamp);
            m_events.pop_back();
        } else {
            break;
        }
    }
}

void Scheduler::runToNext() {
    if(!m_events.empty()) {
        step(m_events.back().scheduled_timestamp - m_current_timestamp);
    }
}

auto Scheduler::nextEventTime() -> u64 {
    if(m_events.empty()) {
        return 0;
    }

    return m_events.back().scheduled_timestamp;
}

auto Scheduler::getCurrentTimestamp() -> u64 {
    return m_current_timestamp;
}

void Scheduler::attachDebugger(dbg::Debugger &debugger) {
    debugger.attachScheduler(&m_events, &m_current_timestamp);
}

} //namespace emu