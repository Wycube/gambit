#include "GBA.hpp"


namespace emu {

GBA::GBA(VideoDevice &video_device, InputDevice &input_device, AudioDevice &audio_device) 
        : video_device(video_device), input_device(input_device), audio_device(audio_device),
        debug(*this), keypad(*this), timer(*this), dma(*this), sio(*this), ppu(*this), apu(*this), bus(*this), cpu(*this) { }

void GBA::reset(bool skip_bios, bool enable_debugger) {
    this->enable_debugger = enable_debugger;

    scheduler.reset();
    debug.reset();
    keypad.reset();
    timer.reset();
    dma.reset();
    sio.reset();
    ppu.reset();
    apu.reset();
    bus.reset();
    cpu.reset(skip_bios);
    cpu.flushPipeline();
}

void GBA::step() {
    u64 start = scheduler.getCurrentTimestamp();

    if(dma.running()) {
        dma.step(1);
        cycles_active += scheduler.getCurrentTimestamp() - start;
    } else if(cpu.halted()) {
        scheduler.runToNext();
        cpu.checkForInterrupt();
    } else {
        cpu.step();
        cycles_active += scheduler.getCurrentTimestamp() - start;
    }

    if(enable_debugger) {
        debug.onStep();
    }
}

auto GBA::run(u32 cycles) -> u32 {
    u64 target = scheduler.getCurrentTimestamp() + cycles;

    while(scheduler.getCurrentTimestamp() < target) {
        u64 start = scheduler.getCurrentTimestamp();

        if(dma.running()) {
            dma.step(target - scheduler.getCurrentTimestamp());
            cycles_active += scheduler.getCurrentTimestamp() - start;
        } else if(cpu.halted()) {
            if(scheduler.nextEventTime() < target) {
                scheduler.runToNext();
            } else {
                scheduler.step(target - scheduler.getCurrentTimestamp());
            }

            cpu.checkForInterrupt();
        } else {
            cpu.step();
            cycles_active += scheduler.getCurrentTimestamp() - start;
        }

        if(enable_debugger && debug.onStep()) {
            break;
        }
    }

    return scheduler.getCurrentTimestamp() - (target - cycles);
}

void GBA::loadBIOS(const std::vector<u8> &bios) {
    bus.loadBIOS(bios);
}

void GBA::saveState(std::ofstream &file) {
    //Write a header magic used to verify file type and endianness
    u32 magic = 0x53414247; //'GBAS' in little-endian
    file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));

    //Write a version number
    file.write(reinterpret_cast<const char*>(&SAVE_STATE_VERSION), sizeof(SAVE_STATE_VERSION));

    //Serialize each component state
    scheduler.serialize(file);
    keypad.serialize(file);
    timer.serialize(file);
    dma.serialize(file);
    sio.serialize(file);
    ppu.serialize(file);
    apu.serialize(file);
    bus.serialize(file);
    cpu.serialize(file);
}

void loadState(std::ifstream &file) {

}

} //namespace emu