void armUnimplemented(u32 instruction);

void armBranchExchange(u32 instruction);

void armPSRTransfer(u32 instruction);

auto addressMode1(u32 instruction) -> std::pair<u32, bool>;
void armDataProcessing(u32 instruction);

void armHalfwordTransfer(u32 instruction);

auto addressMode2(u8 rn, u16 addr_mode, bool i, bool p, bool u, bool w) -> u32;
void armSingleTransfer(u32 instruction);

void armBranch(u32 instruction);