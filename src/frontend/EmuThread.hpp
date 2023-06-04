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

    explicit EmuThread(std::shared_ptr<emu::GBA> core);
    ~EmuThread();

    void start();
    void stop();
    void sendCommand(Command cmd);
    void setFastforward(bool enable);
    auto fastforwarding() -> bool;

    auto isRunning() const -> bool;

private:

    void processCommands();

    std::shared_ptr<emu::GBA> core;

    std::thread thread;
    std::atomic<bool> running;

    std::condition_variable cv;
    std::mutex mutex;
    s32 cycle_diff;

    common::ThreadSafeRingBuffer<Command, 20> cmd_queue;
    std::atomic<bool> fastforward;
};