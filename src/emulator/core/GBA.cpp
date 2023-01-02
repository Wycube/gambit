#include "GBA.hpp"
#include "common/Log.hpp"


namespace emu {

GBA::GBA(VideoDevice &video_device, InputDevice &input_device, AudioDevice &audio_device) 
        : video_device(video_device), input_device(input_device), audio_device(audio_device),
        debugger(*this), keypad(*this), timer(*this), dma(*this), sio(*this), ppu(*this), apu(*this), bus(*this), cpu(*this) {
    scheduler.attachDebugger(debugger);
}

void GBA::reset() {
    scheduler.reset();
    keypad.reset();
    timer.reset();
    dma.reset();
    sio.reset();
    ppu.reset();
    apu.reset();
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
        if(dma.running()) {
            // if(scheduler.nextEventTime() < target) {
            //     scheduler.runToNext();
            // } else {
            //     scheduler.step(target - scheduler.getCurrentTimestamp());
            // }
            dma.step();
        } else if(cpu.halted()) {
            if(scheduler.nextEventTime() < target) {
                scheduler.runToNext();
            } else {
                scheduler.step(target - scheduler.getCurrentTimestamp());
            }

            cpu.checkForInterrupt();
        } else {
            cpu.step();
        }
         

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

void GBA::loadSave(const std::string &filename) {
    auto save = bus.getLoadedPak().getSave();
    if(save->getType() != NONE) {
        save->loadFromFile(filename);
    } else {
        LOG_INFO("No save file loaded because save type was NONE");
    }
}

void GBA::writeSave(const std::string &filename) {
    auto save = bus.getLoadedPak().getSave();
    if(save->getType() != NONE) {
        save->writeToFile(filename);
    } else {
        LOG_INFO("No save file written because save type was NONE");
    }
}

} //namespace emu