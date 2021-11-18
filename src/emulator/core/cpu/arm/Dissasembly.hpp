#pragma once

#include "Arm.hpp"


namespace emu {

auto registerShift(u16 shift_operand) -> std::string;
auto immediateShift(u16 shift_operand) -> std::string;
auto immediate(u16 shift_operand) -> std::string;
auto dissasembleDataProcessing(u32 instruction) -> std::string;

auto dissasembleMultiply(u32 instruction) -> std::string;
auto dissasembleMultiplyLong(u32 instruction) -> std::string;

auto dissasembleDataSwap(u32 instruction) -> std::string;

auto dissasembleBranchExchange(u32 instruction) -> std::string;

auto registerOffset(u8 rn, u16 addr_mode, bool p, bool u, bool w) -> std::string;
auto immediate8Offset(u8 rn, u16 addr_mode, bool p, bool u, bool w) -> std::string;
auto dissasembleHalfwordTransfer(u32 instruction) -> std::string;

auto scaledRegisterOffset(u8 rn, u16 addr_mode, bool p, bool u, bool w) -> std::string;
auto immediate12Offset(u8 rn, u16 addr_mode, bool p, bool u, bool w) -> std::string;
auto dissasembleSingleTransfer(u32 instruction) -> std::string;

auto dissasembleUndefined(u32 instruction) -> std::string;

auto dissasembleBlockTransfer(u32 instruction) -> std::string;

auto dissasembleBranch(u32 instruction) -> std::string;

auto addressMode5(u8 rn, u8 offset, bool p, bool u, bool w) -> std::string;
auto dissasembleCoDataTransfer(u32 instruction) -> std::string;
auto dissasembleCoDataOperation(u32 instruction) -> std::string;
auto dissasembleCoRegisterTransfer(u32 instruction) -> std::string;

auto dissasembleSoftwareInterrupt(u32 instruction) -> std::string;

} //namespace emu