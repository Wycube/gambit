#pragma once

#include "Instruction.hpp"


namespace emu {

auto registerShift(u16 shift_operand) -> std::string;
auto immediateShift(u16 shift_operand) -> std::string;
auto immediate(u16 shift_operand) -> std::string;
auto disassembleDataProcessing(u32 instruction) -> std::string;

auto disassembleMultiply(u32 instruction) -> std::string;
auto disassembleMultiplyLong(u32 instruction) -> std::string;
auto disassembleDataSwap(u32 instruction) -> std::string;
auto disassembleBranchExchange(u32 instruction) -> std::string;

auto registerOffset(u8 rn, u16 addr_mode, bool p, bool u, bool w) -> std::string;
auto immediate8Offset(u8 rn, u16 addr_mode, bool p, bool u, bool w) -> std::string;
auto disassembleHalfwordTransfer(u32 instruction) -> std::string;

auto scaledRegisterOffset(u8 rn, u16 addr_mode, bool p, bool u, bool w) -> std::string;
auto immediate12Offset(u8 rn, u16 addr_mode, bool p, bool u, bool w) -> std::string;
auto disassembleSingleTransfer(u32 instruction) -> std::string;

auto disassembleBlockTransfer(u32 instruction) -> std::string;
auto disassembleBranch(u32 instruction) -> std::string;

auto addressMode5(u8 rn, u8 offset, bool p, bool u, bool w) -> std::string;
auto disassembleCoDataTransfer(u32 instruction) -> std::string;
auto disassembleCoDataOperation(u32 instruction) -> std::string;
auto disassembleCoRegisterTransfer(u32 instruction) -> std::string;

auto disassembleSoftwareInterrupt(u32 instruction) -> std::string;
auto disassembleUndefined(u32 instruction) -> std::string;

} //namespace emu