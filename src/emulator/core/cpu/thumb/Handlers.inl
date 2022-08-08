void thumbUnimplemented(u16 instruction);

void thumbMoveShifted(u16 instruction);
void thumbAddSubtract(u16 instruction);
void thumbProcessImmediate(u16 instruction);
void thumbALUOperation(u16 instruction);
void thumbHiRegisterOp(u16 instruction);
void thumbBranchExchange(u16 instruction);
void thumbPCRelativeLoad(u16 instruction);
void thumbLoadStoreRegister(u16 instruction);
void thumbLoadStoreSigned(u16 instruction);
void thumbLoadStoreImmediate(u16 instruction);
void thumbLoadStoreHalfword(u16 instruction);
void thumbSPRelativeLoadStore(u16 instruction);
void thumbLoadAddress(u16 instruction);
void thumbAdjustSP(u16 instruction);
void thumbPushPopRegisters(u16 instruction);
void thumbLoadStoreMultiple(u16 instruction);
void thumbConditionalBranch(u16 instruction);
void thumbSoftwareInterrupt(u16 instruction);
void thumbUnconditionalBranch(u16 instruction);
void thumbLongBranch(u16 instruction);
void thumbUndefined(u16 instruction);

static std::array<void (CPU::*)(u16), 256> thumb_instruction_lut;