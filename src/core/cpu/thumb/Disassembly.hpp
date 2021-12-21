#pragma once

#include "common/Types.hpp"

#include <string>


namespace emu {

auto thumbDisassembleMoveShifted(u16 instruction) -> std::string;
auto thumbDisassembleAddSubtract(u16 instruction) -> std::string;
auto thumbDisassembleProcessImmediate(u16 instruction) -> std::string;
auto thumbDisassembleALUOperation(u16 instruction) -> std::string;
auto thumbDisassembleHiRegOperation(u16 instruction) -> std::string;
auto thumbDisassembleBranchExchange(u16 instruction) -> std::string;
auto thumbDisassemblePCRelativeLoad(u16 instruction) -> std::string;
auto thumbDisassembleLoadStoreRegister(u16 instruction) -> std::string;
auto thumbDisassembleLoadStoreSigned(u16 instruction) -> std::string;
auto thumbDisassembleLoadStoreImmediate(u16 instruction) -> std::string;
auto thumbDisassembleLoadStoreHalfword(u16 instruction) -> std::string;
auto thumbDisassembleSPRelativeLoadStore(u16 instruction) -> std::string;
auto thumbDisassembleLoadAddress(u16 instruction) -> std::string;
auto thumbDisassembleAdjustSP(u16 instruction) -> std::string;
auto thumbDisassemblePushPopRegisters(u16 instruction) -> std::string;
auto thumbDisassembleLoadStoreMultiple(u16 instruction) -> std::string;
auto thumbDisassembleConditionalBranch(u16 instruction) -> std::string;
auto thumbDisassembleSoftwareInterrupt(u16 instruction) -> std::string;
auto thumbDisassembleUnconditionalBranch(u16 instruction) -> std::string;
auto thumbDisassembleLongBranch(u16 instruction) -> std::string;
auto thumbDisassembleUndefined(u16 instruction) -> std::string;

}; //namespace emu