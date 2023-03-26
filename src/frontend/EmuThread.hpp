#pragma once

#include "emulator/core/GBA.hpp"
#include "common/Types.hpp"
#include <memory>
#include <thread>
#include <condition_variable>


enum Command {
    RUN,
    TERMINATE
};

class EmuThread final {
public:

    EmuThread(std::shared_ptr<emu::GBA> core);
    ~EmuThread();

    void start();
    void stop();
    void sendCommand(Command cmd);
    void setFastforward(bool enable);
    auto fastforwarding() -> bool;

    auto running() const -> bool;

private:

    void processCommands();

    std::shared_ptr<emu::GBA> m_core;

    std::thread m_thread;
    std::atomic<bool> m_running;

    std::condition_variable m_cv;
    std::mutex m_mutex;
    s32 m_cycle_diff;

    common::ThreadSafeRingBuffer<Command, 20> cmd_queue;
    std::atomic<bool> fastforward;
};