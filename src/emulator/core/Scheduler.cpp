#include "Scheduler.hpp"
#include <algorithm>


namespace emu {

Scheduler::Scheduler() {
    reset();
}

void Scheduler::reset() {
    current_timestamp = 0;
    events.clear();
}

auto Scheduler::registerEvent(EventFunc callback) -> EventHandle {
    registered.push_back(callback);
    return registered.size() - 1;
}

void Scheduler::addEvent(const EventHandle handle, u64 cycles_from_now) {
    events.insert(Event{handle, current_timestamp + cycles_from_now});
}

void Scheduler::removeEvent(const EventHandle handle) {
    events.remove(Event{handle, 0});
}

void Scheduler::step(u32 cycles) {
    current_timestamp += cycles;

    while(true) {
        if(events.size() > 0 && events.peek().scheduled_timestamp <= current_timestamp) {
            Event event = events.extract_min();
            registered[event.handle](current_timestamp - event.scheduled_timestamp);
        } else {
            break;
        }
    }
}

void Scheduler::runToNext() {
    if(!events.empty()) {
        step(events.peek().scheduled_timestamp - current_timestamp);
    }
}

auto Scheduler::nextEventTime() -> u64 {
    if(events.empty()) {
        return 0;
    }

    return events.peek().scheduled_timestamp;
}

auto Scheduler::getCurrentTimestamp() -> u64 {
    return current_timestamp;
}

} //namespace emu