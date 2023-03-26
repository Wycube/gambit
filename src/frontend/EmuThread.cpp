#include "EmuThread.hpp"


EmuThread::EmuThread(std::shared_ptr<emu::GBA> core) : m_core(core) {
    m_cycle_diff = 0;
    fastforward = false;
    m_running.store(false);

    m_core->debug.setCallback([this]() {
        m_mutex.lock();
        cmd_queue.clear();
        cmd_queue.push(TERMINATE);
        m_cv.notify_one();
        m_mutex.unlock();
    });
}

EmuThread::~EmuThread() {
    if(m_thread.joinable()) {
        m_thread.join();
    }
}

void EmuThread::start() {
    //Don't do anything if already running
    if(m_thread.joinable()) {
        if(!m_running.load()) {
            m_thread.join();
        } else {
            return;
        }
    }

    m_thread = std::thread([this]() {
        processCommands();
    });
}

void EmuThread::stop() {
    sendCommand(TERMINATE);
    while(m_running.load()) { }
}

void EmuThread::sendCommand(Command cmd) {
    m_mutex.lock();
    cmd_queue.push(cmd);
    m_cv.notify_one();
    m_mutex.unlock();
}

void EmuThread::setFastforward(bool enable) {
    fastforward.store(enable);
}

auto EmuThread::fastforwarding() -> bool {
    return fastforward.load();
}

auto EmuThread::running() const -> bool {
    return m_running.load();
}

void EmuThread::processCommands() {
    bool running = true;
    m_cycle_diff = 0;
    cmd_queue.clear();
    m_running.store(true);

    while(running) {
        Command cmd;

        if(fastforward.load()) {
            // auto start = std::chrono::steady_clock::now();
            m_core->run(16777216 / 64);
            // LOG_INFO("Ran {} cycles in {}ms", actual, (std::chrono::steady_clock::now() - start).count() / 1000000.0f);

            if(cmd_queue.size() == 0) {
                continue;
            }

            cmd = cmd_queue.pop();
        } else {
            std::unique_lock lock(m_mutex);
            m_cv.wait(lock, [this]() { return cmd_queue.size() > 0; });
            cmd = cmd_queue.pop();
            lock.unlock();
            // LOG_INFO("Waited for {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count());
        }

        switch(cmd) {
            case RUN : 
                if(!fastforward) {
                    m_cycle_diff += 16777216 / 64;
                    // auto start = std::chrono::steady_clock::now();
                    u32 actual = m_core->run(m_cycle_diff);
                    // LOG_INFO("Ran {} cycles in {}ms", actual, (std::chrono::steady_clock::now() - start).count() / 1000000.0f);
                    m_cycle_diff = m_cycle_diff - actual;
                }
                break;

            case TERMINATE : running = false; break;
        }
    }

    m_running.store(false);
}