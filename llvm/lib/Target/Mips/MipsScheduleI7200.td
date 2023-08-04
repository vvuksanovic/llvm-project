def MipsI7200Model : SchedMachineModel {
  int IssueWidth = 2;
  int MicroOpBufferSize = 48;
  int LoadLatency = 4;
  int MispredictPenalty = 8;

  let CompleteModel = 1;
  let FullInstRWOverlapCheck = 1;

  // Disable all except nanomips instructions.
  list<Predicate> UnsupportedFeatures = [
    HasMips2, HasMips3_32, HasMips3_32r2, HasMips3, HasMips4_32, HasMips4_32r2,
    HasMips32, HasMips32r2, HasMips32r5, HasMips32r6,
    HasMips64, HasMips64r2, HasMips64r5, HasMips64r6,
    IsGP64bit, IsFP64bit, IsPTR64bit, IsNotSoftFloat,
    InMicroMips, InMips16Mode, HasCnMips, HasCnMipsP,
    HasDSP, HasDSPR2, HasMips3D, HasMT,
    HasCRC, HasStdEnc, HasMSA
  ];
}

// Define processor resources.
let SchedModel = MipsI7200Model, BufferSize = 16 in {
  def i7200GpMulDiv : ProcResource<1>;

  def i7200Agen : ProcResource<1>;
  def i7200Alu1 : ProcResource<1>;
  def i7200Lsu : ProcResource<1>;

  def i7200Control : ProcResource<1>;
  def i7200Ctu : ProcResource<1>;
  def i7200Alu0 : ProcResource<1>;

  def i7200Alu : ProcResGroup<[i7200Alu0, i7200Alu1]>;
}

// Define read classes for ReadAdvance.
def I7200ReadLoad : SchedRead;
def I7200ReadStore : SchedRead;

// Define instruction schedules.
let SchedModel = MipsI7200Model in {

// Arithmetic and logical instructions.
def I7200WriteALU : SchedWriteRes<[i7200Alu]>;
def : InstRW<[I7200WriteALU], (instrs
  ADD_NM, ADDiu_NM, ADDIU48_NM, ADDu_NM, SUB_NM, SUBu_NM,
  AND_NM, AND16_NM, ANDI_NM, OR_NM, OR16_NM, ORI_NM, XOR_NM, XOR16_NM, XORI_NM, NOR_NM, NOT_NM,
  SRA, SRAV, SRL, SRLV, SLL, SLLV, SRA_NM, SRAV_NM, SRL_NM, SRLV_NM, SLL_NM, SLLV_NM, 
  SLT, SLTi, SLTiu, SLTu, SLT_NM, SLTI_NM, SLTIU_NM, SLTU_NM, SEQI_NM,
  LA_NM, Li_NM, SEB_NM, SEH_NM,
  BITREVW_NM, BYTEREVW_NM,
  MOVE_NM, MOVN_NM, MOVZ_NM,
  CLZ_NM, CLZ, CLO_NM, CLO,
  EXT_NM, INS_NM,
  LSA_NM, SAVE_NM, RESTORE_NM,
  ALUIPC_NM, ROTR_NM, ROTRV_NM
)>;

// Movep instruction.
def I7200WriteMovep : SchedWriteRes<[i7200Control, i7200Agen, i7200Alu0, i7200Alu1]> {
  let Latency = 1;
}
def : InstRW<[I7200WriteMovep], (instrs MOVEP_NM)>;

// Mul instructions.
def I7200WriteMul : SchedWriteRes<[i7200Agen, i7200GpMulDiv]> {
  let Latency = 5;
  let ResourceCycles = [1, 1];
}
def : InstRW<[I7200WriteMul], (instrs MUL_NM, MULU_NM, MUH_NM, MUHU_NM)>;

// Div instructions.
def I7200WriteDiv : SchedWriteRes<[i7200GpMulDiv]> {
  let Latency = 32;
  let ResourceCycles = [32];
}
def : InstRW<[I7200WriteDiv], (instrs DIV_NM, DIVU_NM, MOD_NM, MODU_NM)>;

// Load instructions.
def I7200WriteLoad : SchedWriteRes<[i7200Agen, i7200Lsu]> {
  let Latency = 2;
  let ResourceCycles = [1, 1];
}
def : InstRW<[I7200WriteLoad, I7200ReadLoad], (instrs
  LB_NM, LBs9_NM, LBU_NM, LBUs9_NM, LH_NM, LHs9_NM, LHU_NM, LHUs9_NM, LW_NM, LWs9_NM,
  LBX_NM, LBUX_NM, LWX_NM, LWXS_NM, LHX_NM, LHXS_NM, LHUX_NM, LHUXS_NM, LWM_NM,
  LWPC_NM, LWGP_NM, UALW_NM, UALH_NM, UALWM_NM
)>;

// Store instructions.
def I7200WriteStore : SchedWriteRes<[i7200Agen, i7200Lsu]> {
  let Latency = 1;
  let ResourceCycles = [1, 1];
}
def : InstRW<[I7200WriteStore, I7200ReadStore], (instrs 
  SB_NM, SBs9_NM, SH_NM, SHs9_NM, SW_NM, SWs9_NM,
  SBX_NM, SHX_NM, SHXS_NM, SWX_NM, SWXS_NM,
  SWPC_NM, SWM_NM, UASW_NM, UASH_NM, UASWM_NM
)>;

// Branch and jump instructions.
def I7200WriteBranch : SchedWriteRes<[i7200Control, i7200Ctu]> {
  let Latency = 1;
  let ResourceCycles = [1, 1];
}
def : InstRW<[I7200WriteBranch], (instrs
  BRSC_NM, BC_NM, BEQC_NM, BEQIC_NM, BEQZC_NM, BGEC_NM, BGEIC_NM, BGEIUC_NM, BGEUC_NM,
  BLTC_NM, BLTIC_NM, BLTIUC_NM, BLTUC_NM, BNEC_NM, BNEIC_NM, BNEZC_NM,
  BBNEZC_NM, BBEQZC_NM, JALRC_NM, JRC_NM, RESTOREJRC_NM, BALC_NM, MOVEBALC_NM
)>;

// Unclear how to schedule these. Use WriteALU for now.
def : InstRW<[I7200WriteALU], (instrs TEQ_NM, TNE_NM, RDHWR_NM)>;

// Pseudo instructions.
def : InstRW<[I7200WriteALU], (instrs COPY)>;
def : InstRW<[I7200WriteBranch], (instrs
  RetRA, PseudoReturn, PseudoReturnNM, PseudoIndirectBranchNM,
  TAILCALL_NM, TAILCALLREG_NM, JALRCPseudo, JALRHBPseudo, ERet
)>;

// These are copied from the generic model.
def I7200Atomic : ProcResource<1> { let BufferSize = 1; }
def I7200WriteAtomic : SchedWriteRes<[I7200Atomic]> { let Latency = 2; }
def : InstRW<[I7200WriteAtomic],
    (instregex "^ATOMIC_SWAP_I(8|16|32|64)_POSTRA$")>;
def : InstRW<[I7200WriteAtomic],
    (instregex "^ATOMIC_CMP_SWAP_I(8|16|32|64)_POSTRA$")>;
def : InstRW<[I7200WriteAtomic],
    (instregex "^ATOMIC_LOAD_(ADD|SUB|AND|OR|XOR|NAND|MIN|MAX|UMIN|UMAX)"
               "_I(8|16|32|64)_POSTRA$")>;


// Define bypasses.
// Load and store instructions take 1 cycle longer if their operand was
// created by a load, arithmetic or move operation.
def : ReadAdvance<I7200ReadLoad, -1, [I7200WriteLoad, I7200WriteALU, 
                                      I7200WriteMovep, I7200WriteMul]>;
def : ReadAdvance<I7200ReadStore, -1, [I7200WriteLoad, I7200WriteALU,
                                       I7200WriteMovep, I7200WriteMul]>;

}
