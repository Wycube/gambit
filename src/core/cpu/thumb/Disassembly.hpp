#pragma once

#include "common/Types.hpp"

#include <string>


namespace emu {

extern std::string (*thumbDisassemblyFuncs[21])(u16, u32, u16);


auto thumbDisassembleMoveShifted(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;
auto thumbDisassembleAddSubtract(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;
auto thumbDisassembleProcessImmediate(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;
auto thumbDisassembleALUOperation(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;
auto thumbDisassembleHiRegisterOp(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;
auto thumbDisassembleBranchExchange(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;
auto thumbDisassemblePCRelativeLoad(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;
auto thumbDisassembleLoadStoreRegister(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;
auto thumbDisassembleLoadStoreSigned(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;
auto thumbDisassembleLoadStoreImmediate(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;
auto thumbDisassembleLoadStoreHalfword(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;
auto thumbDisassembleSPRelativeLoadStore(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;
auto thumbDisassembleLoadAddress(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;
auto thumbDisassembleAdjustSP(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;
auto thumbDisassemblePushPopRegisters(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;
auto thumbDisassembleLoadStoreMultiple(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;
auto thumbDisassembleConditionalBranch(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;
auto thumbDisassembleSoftwareInterrupt(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;
auto thumbDisassembleUnconditionalBranch(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;
auto thumbDisassembleLongBranch(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;
auto thumbDisassembleInvalid(u16 instruction, u32 address = 0, u16 prev = 0) -> std::string;

}; //namespace emu