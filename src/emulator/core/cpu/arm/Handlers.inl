void armUnimplemented(u32 instruction);
void armBranchExchange(u32 instruction);
void armPSRTransfer(u32 instruction);
auto addressMode1(u32 instruction, bool &carry) -> u32;
void armDataProcessing(u32 instruction);
void armMultiply(u32 instruction);
void armMultiplyLong(u32 instruction);
void armSingleDataSwap(u32 instruction);
void armHalfwordTransfer(u32 instruction);
auto addressMode2(u16 addr_mode, bool i) -> u32;
void armSingleTransfer(u32 instruction);
void armUndefined(u32 instruction);
void armBlockTransfer(u32 instruction);
void armBranch(u32 instruction);
void armSoftwareInterrupt(u32 instruction);