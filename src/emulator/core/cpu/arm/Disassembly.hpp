#pragma once

#include "Instruction.hpp"


namespace emu {

auto armDisassembleBranchExchange(u32 instruction) -> std::string;
auto armDisassemblePSRTransfer(u32 instruction) -> std::string;
auto registerShift(u16 shift_operand) -> std::string;
auto immediateShift(u16 shift_operand) -> std::string;
auto immediate(u16 shift_operand) -> std::string;
auto armDisassembleDataProcessing(u32 instruction) -> std::string;
auto armDisassembleMultiply(u32 instruction) -> std::string;
auto armDisassembleMultiplyLong(u32 instruction) -> std::string;
auto armDisassembleSingleDataSwap(u32 instruction) -> std::string;
auto registerOffset(u8 rn, u16 addr_mode, bool p, bool u, bool w) -> std::string;
auto immediate8Offset(u8 rn, u16 addr_mode, bool p, bool u, bool w) -> std::string;
auto armDisassembleHalfwordTransfer(u32 instruction) -> std::string;
auto scaledRegisterOffset(u8 rn, u16 addr_mode, bool p, bool u, bool w) -> std::string;
auto immediate12Offset(u8 rn, u16 addr_mode, bool p, bool u, bool w) -> std::string;
auto armDisassembleSingleTransfer(u32 instruction) -> std::string;
auto armDisassembleUndefined() -> std::string;
auto armDisassembleBlockTransfer(u32 instruction) -> std::string;
auto armDisassembleBranch(u32 instruction, u32 address = 0) -> std::string;
auto addressMode5(u8 rn, u8 offset, bool p, bool u, bool w) -> std::string;
auto armDisassembleCoDataTransfer(u32 instruction) -> std::string;
auto armDisassembleCoDataOperation(u32 instruction) -> std::string;
auto armDisassembleCoRegisterTransfer(u32 instruction) -> std::string;
auto armDisassembleSoftwareInterrupt(u32 instruction) -> std::string;

} //namespace emu