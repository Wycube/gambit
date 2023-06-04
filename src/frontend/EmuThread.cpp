#include "EmuThread.hpp"

#include "common/Log.hpp"


EmuThread::EmuThread(std::shared_ptr<emu::GBA> core) : core(core) {
    cycle_diff = 0;
    running.store(false);
    fastforward.store(false);

    core->debug.setCallback([this]() {
        mutex.lock();
        cmd_queue.clear();
        cmd_queue.push(TERMINATE);
        cv.notify_one();
        mutex.unlock();
    });
}

EmuThread::~EmuThread() {
    if(thread.joinable()) {
        thread.join();
    }
}

void EmuThread::start() {
    //Don't do anything if already running
    if(thread.joinable()) {
        if(!running.load()) {
            thread.join();
        } else {
            return;
        }
    }

    thread = std::thread([this]() {
        processCommands();
    });
}

void EmuThread::stop() {
    sendCommand(TERMINATE);
    while(running.load()) { }
}

void EmuThread::sendCommand(Command cmd) {
    mutex.lock();
    cmd_queue.push(cmd);
    cv.notify_one();
    mutex.unlock();
}

void EmuThread::setFastforward(bool enable) {
    fastforward.store(enable);
}

auto EmuThread::fastforwarding() -> bool {
    return fastforward.load();
}

auto EmuThread::isRunning() const -> bool {
    return running.load();
}

void EmuThread::processCommands() {
    bool finished = false;
    cycle_diff = 0;
    cmd_queue.clear();
    running.store(true);

    while(!finished) {
        Command cmd;

        if(fastforward.load()) {
            core->run(16777216 / 64);

            if(cmd_queue.size() == 0) {
                continue;
            }

            cmd = cmd_queue.pop();
        } else {
            std::unique_lock lock(mutex);
            cv.wait(lock, [this]() { return cmd_queue.size() > 0; });
            cmd = cmd_queue.pop();
            lock.unlock();
        }

        switch(cmd) {
            case RUN : 
                if(!fastforward) {
                    cycle_diff += 16777216 / 64;
                    u32 actual = core->run(cycle_diff);
                    cycle_diff = cycle_diff - actual;
                }
                break;

            case TERMINATE : finished = true; break;
        }
    }

    running.store(false);
}