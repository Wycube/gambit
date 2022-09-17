#include "GBA.hpp"


namespace emu {

GBA::GBA(VideoDevice &video_device, InputDevice &input_device) : video_device(video_device), input_device(input_device), 
        debugger(*this), keypad(*this), timer(*this), dma(*this), ppu(*this), bus(*this), cpu(*this) {
    scheduler.attachDebugger(debugger);
}

void GBA::reset() {
    scheduler.reset();
    keypad.reset();
    timer.reset();
    dma.reset();
    ppu.reset();
    bus.reset();
    cpu.reset();
    cpu.flushPipeline();
}

void GBA::step() {
    if(dma.running() || cpu.halted()) {
        scheduler.runToNext();
        cpu.checkForInterrupt();
        
        return;
    }

    cpu.step();
}

auto GBA::run(u32 cycles) -> u32 {
    u64 target = scheduler.getCurrentTimestamp() + cycles;

    while(scheduler.getCurrentTimestamp() < target) {
        if(dma.running() || cpu.halted()) {
            if(scheduler.nextEventTime() < target) {
                scheduler.runToNext();
            } else {
                scheduler.step(target - scheduler.getCurrentTimestamp());
            }

            cpu.checkForInterrupt();
            continue;
        }
         
        cpu.step();

        if(debugger.checkBreakpoints()) {
            break;
        }
    }

    return scheduler.getCurrentTimestamp() - (target - cycles);
}

auto GBA::getGamePak() -> GamePak& {
    return bus.getLoadedPak();
}

void GBA::loadROM(std::vector<u8> &&rom) {
    bus.loadROM(std::move(rom));
    cpu.flushPipeline();
}

void GBA::loadBIOS(const std::vector<u8> &bios) {
    bus.loadBIOS(bios);
}

} //namespace emu