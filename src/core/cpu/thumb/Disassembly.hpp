#pragma once

#include "common/Types.hpp"

#include <string>


namespace emu {

auto disassembleMoveShifted(u16 instruction) -> std::string;
auto disassembleAddSubtract(u16 instruction) -> std::string;
auto disassembleProcessImmediate(u16 instruction) -> std::string;
auto disassembleALUOperation(u16 instruction) -> std::string;
auto disassembleHiRegOperation(u16 instruction) -> std::string;
auto disassembleBranchExchange(u16 instruction) -> std::string;
auto disassemblePCRelativeLoad(u16 instruction) -> std::string;
auto disassembleLoadStoreRegister(u16 instruction) -> std::string;
auto disassembleLoadStoreSigned(u16 instruction) -> std::string;
auto disassembleLoadStoreImmediate(u16 instruction) -> std::string;
auto disassembleLoadStoreHalfword(u16 instruction) -> std::string;
auto disassembleSPRelativeLoadStore(u16 instruction) -> std::string;
auto disassembleLoadAddress(u16 instruction) -> std::string;
auto disassembleAdjustSP(u16 instruction) -> std::string;
auto disassemblePushPopRegisters(u16 instruction) -> std::string;
auto disassembleLoadStoreMultiple(u16 instruction) -> std::string;
auto disassembleConditionalBranch(u16 instruction) -> std::string;
auto disassembleSoftwareInterrupt(u16 instruction) -> std::string;
auto disassembleUnconditionalBranch(u16 instruction) -> std::string;
auto disassembleLongBranch(u16 instruction) -> std::string;
auto disassembleUndefined(u16 instruction) -> std::string;

}; //namespace emu