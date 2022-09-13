#pragma once


namespace emu {

static const char *function_map[] = {
    "SoftReset",                       //0x00
    "RegisterRamReset",                //0x01
    "Halt",                            //0x02
    "Stop/Sleep",                      //0x03
    "IntrWait",                        //0x04
    "VBlankIntrWait",                  //0x05
    "Div",                             //0x06
    "DivArm",                          //0x07
    "Sqrt",                            //0x08
    "ArcTan",                          //0x09
    "ArcTan2",                         //0x0A
    "CpuSet",                          //0x0B
    "CpuFastSet",                      //0x0C
    "GetBiosChecksum",                 //0x0D
    "BgAffineSet",                     //0x0E
    "ObjAffineSet",                    //0x0F
    "BitUnpack",                       //0x10
    "LZ77UnCompReadNormalWrite8Bit",   //0x11
    "LZ77UnCompReadNormalWrite16Bit",  //0x12
    "HuffUnCompReadNormal",            //0x13
    "RLUncompReadNormalWrite8Bit",     //0x14
    "RLUncompReadNormalWrite16Bit",    //0x15
    "Diff8bitUnfilterWrite8bit",       //0x16
    "Diff8bitUnfilterWrite16bit",      //0x17
    "Diff16bitUnfilter",               //0x18
    "SoundBias",                       //0x19
    "SoundDriverInit",                 //0x1A
    "SoundDriverMode",                 //0x1B
    "SoundDriverMain",                 //0x1C
    "SoundDriverVsync",                //0x1D
    "SoundChannelClear",               //0x1E
    "Midikey2Freq",                    //0x1F
    "SoundWhatever0",                  //0x20
    "SoundWhatever1",                  //0x21
    "SoundWhatever2",                  //0x22
    "SoundWhatever3",                  //0x23
    "SoundWhatever4",                  //0x24
    "MultiBoot",                       //0x25
    "HardReset",                       //0x26
    "CustomHalt",                      //0x27
    "SoundDriverVsyncOff",             //0x28
    "SoundDriverVsyncOn",              //0x29
    "SoundGetJumpList",                //0x2A
    "Crash"                            //0x2B
};

} //namespace emu