#include "Scheduler.hpp"
#include <algorithm>


namespace emu {

Scheduler::Scheduler() {
    reset();
}

void Scheduler::reset() {
    current_timestamp = 0;
    next_handle = 1;
    events.clear();
}

auto Scheduler::generateHandle() -> EventHandle {
    return next_handle++;
}

void Scheduler::addEvent(const EventHandle handle, u64 cycles_from_now, EventFunc callback) {
    events.push_back(Event{handle, callback, current_timestamp + cycles_from_now});
    std::sort(events.begin(), events.end(), [](const Event &a, const Event &b) {
        return a.scheduled_timestamp > b.scheduled_timestamp;
    });
}

void Scheduler::removeEvent(const EventHandle handle) {
    for(auto iter = events.begin(); iter != events.end(); iter++) {
        if(iter->handle == handle) {
            events.erase(iter);
            break;
        }
    }
}

void Scheduler::step(u32 cycles) {
    current_timestamp += cycles;

    while(true) {
        if(events.size() > 0 && events.back().scheduled_timestamp <= current_timestamp) {
            events.back().callback(current_timestamp - events.back().scheduled_timestamp);
            events.pop_back();
        } else {
            break;
        }
    }
}

void Scheduler::runToNext() {
    if(!events.empty()) {
        step(events.back().scheduled_timestamp - current_timestamp);
    }
}

auto Scheduler::nextEventTime() -> u64 {
    if(events.empty()) {
        return 0;
    }

    return events.back().scheduled_timestamp;
}

auto Scheduler::getCurrentTimestamp() -> u64 {
    return current_timestamp;
}

void Scheduler::attachDebugger(dbg::Debugger &debugger) {
    debugger.attachScheduler(&events, &current_timestamp);
}

} //namespace emu