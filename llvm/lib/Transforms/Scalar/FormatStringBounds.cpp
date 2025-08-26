#include "llvm/Transforms/Scalar/FormatStringBounds.h"
#include "llvm/Analysis/LazyValueInfo.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/ErrorHandling.h"
#include <cstdlib>
#include <limits>
#include <optional>

using namespace llvm;

// FormatStringBoundsPass::FormatStringBoundsPass() : FunctionPass(ID) {}

// bool FormatStringBoundsPass::runOnFunction(Function &F) {
//   if (F.size() > 100) {
//     DiagnosticInfoGenericWithLoc Diag("Example warning", F, F.getSubprogram(),
//                                       llvm::DS_Warning);
//     F.getContext().diagnose(Diag);
//   }

//   return false;
// }

// char FormatStringBoundsPass::ID = 0;

FormatStringBoundsPass::FormatStringBoundsPass() {
};

static unsigned getConstantLength(long long Constant, unsigned Base, std::optional<std::pair<unsigned, unsigned>> Precision, bool PlusFlag, bool Prefix) {
  llvm::dbgs() << "getConstantLength constant " << Constant << " base " << Base << " precision " << (Precision ? Precision->first : -1) << " plus " << PlusFlag << " prefix " << Prefix << "\n";
  unsigned Length = 0;
  long long Value = Constant;

  if (Value < 0) {
    ++Length; // Add space for sign.
    Value = -Value; // Convert to absolute value.
  } else if (PlusFlag) {
    ++Length; // Also add space for sign if the value is non-negative but plus flag is specified.
  }
  
  unsigned nDigits = 0;
  while (Value != 0) {
    Value = Value / Base;
    ++nDigits;
  }
  Length += nDigits;

  if (Prefix && Constant != 0) {
    if (Base == 2 || Base == 16)  Length += 2;
    else if (Base == 8 && Precision && Precision->first <= nDigits) Length++; // TODO: Not necessary if precision adds a 0
  }
  
  return Length;
}

bool getIntRange(Value *Val, Instruction *Inst, Function *F, LazyValueInfo &LVI, long long &Min, long long &Max) {
  if (!Val) {
    // F->getParent()->getContext().
    // Min = Type::getIn
    // Val->getType().
    // LVI.getConstantRange(Value *V, Instruction *CxtI, bool UndefAllowed)
  }
  
  // Make sure this is an integer
  if (!Val->getType()->isIntegerTy())
    return false;

  bool KnownRange = false;
  unsigned Width = Val->getType()->getIntegerBitWidth();

  if (!Val) {
    // 
  }

  return true;
}

struct FormatDirective {
  // bool FlagMinus; // Right justify the result, not relevant to the length
  // bool FlagZero; // Pad using zeros instead of spaces, not relevant to the length
  bool FlagPlus = false; // Always add a sign, even if the number is positive
  bool FlagSpace = false;
  bool FlagHash = false;
  std::optional<int> Width = std::nullopt;
  std::optional<std::pair<unsigned, unsigned>> Precision = std::nullopt;

  unsigned DirNo = 0;
  unsigned ArgNo = 0;

  enum SubSpec {
    NONE, hh, h, l, ll, j, z, t, L
  };
  char Specifier = ' ';
  SubSpec SubSpecifier = NONE;

  const char *BeginPos = nullptr;
  unsigned Length = 0;
};

static FormatDirective ParseDirective(StringRef FormatStr, const char *Begin, const CallInst *CI, unsigned &ArgNo) {
  FormatDirective Dir;
  Dir.BeginPos = Begin;
  
  // First check if this is a directive, if not return as if it was a literal.
  const char *DirectiveStart = Begin;
  while (*DirectiveStart != '%' && *DirectiveStart != '\0') {
    ++DirectiveStart;
  }
  if (DirectiveStart != Begin) {
    Dir.Length = DirectiveStart - Begin;
    Dir.Specifier = '$';
    return Dir;
  }

  const char *CharIt = Begin;
  // Move from the '%'
  ++CharIt;
  // TODO: This check is needed everywhere where CharIt is incremented
  if (CharIt >= FormatStr.end()) // Check if we're still in the string, in case of invalid specifier
    return Dir;

  llvm::dbgs() << "found directive\n";

  // Interpret directive flags
  while (true) {
    switch (*CharIt) {
    case '-':
    case '0':
      // these do not affect the length.
      ++CharIt;
      continue;
    case '+':
      // the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative)
      ++CharIt;
      Dir.FlagPlus = true;
      continue;
    case ' ':
      // if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present
      ++CharIt;
      Dir.FlagSpace = true;
      continue;
    case '#':
      // alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined
      ++CharIt;
      Dir.FlagHash = true;
      continue;
    }
    // Invalid flag character, break this loop and check the next part.
    break;
  }

  llvm::dbgs() << "parsed flags. now at '" << *CharIt << "'\n";

  // Interpret minimum field width
  // The result is padded with space characters (by default)
  if (*CharIt == '*') {
    // the width is specified by an additional argument of type int, which appears before the argument to be
    // converted and the argument supplying precision if one is supplied
    ++CharIt;
    // Does this argument exist, or is it from varargs
    if (CI->getNumOperands() >= ArgNo) {
      // Try to eval the length arg. if successful, increment the current size by that much
      // Note that this is the minimum based on the format string, the actual minimum dependent on the actual argument could be more.
      Value *ArgLengthValue = CI->getArgOperand(ArgNo);
      ConstantInt *ArgLengthConst = dyn_cast<ConstantInt>(ArgLengthValue);
      if (ArgLengthConst) {
        // If this is negative then apply the '-' flag, and treat this as positive.
        // We're not interested in the - flag, so we ignore that part.
        Dir.Width = std::abs(ArgLengthConst->getSExtValue());
      }
    } else {
      Dir.Width = -1;
    }
    // Move to the next arg since this was preceeded by the length arg.
    ++ArgNo;
  } else if (*CharIt >= '0' && *CharIt <= '9') {
    // The width is specified in the fomat string
    llvm::dbgs() << "parsing literal width\n";
      unsigned ArgLength = 0;
      while(*CharIt >= '0' && *CharIt <= '9') {
        llvm::dbgs() << "parsing digit " << *CharIt << "\n";
        ArgLength = ArgLength*10 + (*CharIt - '0');
        llvm::dbgs() << "current width " << ArgLength << "\n";
        ++CharIt;
      }
    Dir.Width = ArgLength;
  } else {
    // NOOP, the width is not specified in the format string.
  }

  llvm::dbgs() << "parsed width " << Dir.Width.has_value() << " " << (Dir.Width.value_or(-1)) << ". now at '" << *CharIt << "'\n";

  // Interpret precision
  if (*CharIt == '.') {
    ++CharIt;
    if (*CharIt == '*') {
      // the precision is specified by an additional argument of type int, which appears before the argument to be converted,
      // but after the argument supplying minimum field width if one is supplied
      ++CharIt;
      if (CI->getNumOperands() >= ArgNo) {
        // Try to eval the length arg. if successful, increment the current size by that much
        // Note that this is the minimum based on the format string, the actual minimum dependent on the actual argument could be more.
        Value *ArgPrecisionValue = CI->getArgOperand(ArgNo);
        ConstantInt *ArgPrecisionConst = dyn_cast<ConstantInt>(ArgPrecisionValue);
        if (ArgPrecisionConst) { 
          // TODO: Break if this is negative, that is not a valid case.
          // if (ArgPrecisionConst->isNegative())
          //   return Dir;
          
          Dir.Precision = {ArgPrecisionConst->getZExtValue(), ArgPrecisionConst->getZExtValue()};
        }
      } else {
        // TODO: Try to determine the range using LVI
        Dir.Precision = std::nullopt;
      }
      // Move to the next arg since this was preceeded by the length arg.
      ++ArgNo;
    } else if (*CharIt >= '0' && *CharIt <= '9') {
      // The precision is specified in the fomat string
      llvm::dbgs() << "parsing literal precision\n";
      unsigned ArgLength = 0;
      while(*CharIt >= '0' && *CharIt <= '9') {
        llvm::dbgs() << "parsing digit " << *CharIt << "\n";
        ArgLength = ArgLength*10 + (*CharIt - '0');
        llvm::dbgs() << "current precision " << ArgLength << "\n";
        ++CharIt;
      }
      Dir.Precision = {ArgLength, ArgLength};
    } else {
      // NOOP, the precision is not specified in the format string.
    }
  }

  llvm::dbgs() << "parsed precision " << Dir.Precision.has_value() << " " << (Dir.Precision.has_value() ? Dir.Precision->first : -1) << ". now at '" << *CharIt << "'\n";

  // interpret length sub-specifiers: hh,h,l,ll,j,z,t,L
  switch(*CharIt) {
    case 'h':
      if (*(CharIt+1) == 'h') {
        Dir.SubSpecifier = FormatDirective::hh;
        ++CharIt;
      } else {
        Dir.SubSpecifier = FormatDirective::h;
      }
      break;
    case 'l':
      if (*(CharIt+1) == 'l') {
        Dir.SubSpecifier = FormatDirective::ll;
        ++CharIt;
      } else {
        Dir.SubSpecifier = FormatDirective::l;
      }
      ++CharIt;
      break;
    case 'j':
      Dir.SubSpecifier = FormatDirective::j;
      ++CharIt;
      break;
    case 'z':
      Dir.SubSpecifier = FormatDirective::z;
      ++CharIt;
      break;
    case 't':
      Dir.SubSpecifier = FormatDirective::t;
      ++CharIt;
      break;
    case 'L':
      Dir.SubSpecifier = FormatDirective::L;
      ++CharIt;
      break;
 
  }
  llvm::errs() << "using subspecifier " << Dir.SubSpecifier << " (NONE, hh, h, l, ll, j, z, t, L)\n";

  // Interpret format specifier
  // TODO: How to handle variadic functions? we can't just retrieve args with getArgOperand, instead we have to find the va_list ?
  
  // switch (*CharIt) {

  // }
  Dir.Specifier = *CharIt; // TODO: Check if the specifier is a valid character
  Dir.Length = CharIt - Begin + 1;
  llvm::dbgs() << "returning dir with specifier '" << Dir.Specifier << "'\n";

  return Dir;
}

struct FormatRange {
  long long Min;
  long long Max;
  long long Likely;
  long long Unlikely;

  FormatRange(long long Value) : Min(Value), Max(Value), Likely(Value), Unlikely(Value) {}

};

struct FormatResult {
  FormatRange Range;
  bool KnownRange;

  FormatResult() : Range(0) {

  }

  void adjust(long long AdjMin, long long AdjMax, Type *T, unsigned Base, unsigned Adj) {
    bool MinAdjusted = false;
    if (AdjMin >= 0) {
      if (Range.Min < AdjMin) {
        Range.Min = AdjMin;
        MinAdjusted = true;
      }
      if (Range.Likely < Range.Min)
        Range.Likely = Range.Min;
    } else if (AdjMin == std::numeric_limits<int>().min() && AdjMax == std::numeric_limits<int>().max()) {
      KnownRange = false; // Why?
    }

    if (AdjMax > 0) {
      if (Range.Max < AdjMax) {
        Range.Max = AdjMax;
        KnownRange = MinAdjusted;
      }
    }

    if (false && T) { // TODO: For warning level 2
      // unsigned Digits = getConstantLength(std::numeric_limits<int>().max(), 10);
      // if (AdjMin < Digits && Digits < AdjMax && Range.Likely < Digits) {
      //   Range.Likely = Digits + Adj;
      // }
    } else if (Range.Likely < (Range.Min ? Range.Min : 1)) {
      Range.Likely = (Range.Min ? Range.Min : Range.Max && (Range.Max < std::numeric_limits<int>().max() || false)? 1 : 0);
    }

    if (Range.Unlikely < Range.Max) {
      Range.Unlikely = Range.Max;
    }
  }


};

FormatResult formatInteger(const FormatDirective &Dir, CallInst *CI, unsigned CurrentArg, LazyValueInfo &LVI) {
  FormatResult DirRes;

  bool MaybeBase = Dir.FlagHash;
  bool MaybeSign = false;
  bool Sign = false;
  unsigned Base;

  if (Dir.Specifier == 'd' || Dir.Specifier == 'i') {
    Base = 10;
    MaybeSign = Dir.FlagSpace || Dir.FlagPlus;
    Sign = true;
  } else if (Dir.Specifier == 'x' || Dir.Specifier == 'X')
    Base = 16;
  else if (Dir.Specifier == 'o') {
    Base = 8;
  } else {
    llvm_unreachable("Unsupported numeric specifier");
  }

  Type *T;
  if (CI->getNumOperands() <= CurrentArg) {
    // Either va_list or missing arg
    // Guess the type based on the format string
    Type *FormatType = Type::getInt32Ty(CI->getFunction()->getContext());
    T = FormatType;
    llvm::dbgs() << "Missing arg, using type instead\n";
  } else {
    Value *Val = CI->getArgOperand(CurrentArg);
    Type *ArgType = Val->getType();
    T = ArgType;

    llvm::dbgs() << "Found arg\n";
    Val->print(llvm::dbgs());
    llvm::dbgs() << "\n";

    // try to evaluate this number
    ConstantRange ArgRange = LVI.getConstantRange(Val, CI, false);
    llvm::dbgs() << "determined range (u) " << ArgRange.getUnsignedMin() << " " << ArgRange.getUnsignedMax() << "\n";
    llvm::dbgs() << "determined range (s) " << ArgRange.getSignedMin() << " " << ArgRange.getSignedMax() << "\n";
    if (ArgRange.isSingleElement()) {
      unsigned ArgSize = getConstantLength(ArgRange.getLower().getSExtValue(), Base, Dir.Precision, MaybeSign, MaybeBase);
      // Special case: precision of 0 doesn't print the 0 constant.
      // TODO: Make precision and width ranges in case they are passed by arg.
      if (Dir.Precision && Dir.Precision->first == 0 && Dir.Precision->second == 0 && ArgRange.getLower().getSExtValue() == 0 && !((Base == 8 && Dir.FlagHash) || MaybeSign))
        ArgSize = 0;
      DirRes.Range.Min = DirRes.Range.Max = DirRes.Range.Likely = DirRes.Range.Unlikely = ArgSize;
      DirRes.KnownRange = true;
      llvm::dbgs() << "Adding const int with min value " << ArgRange.getSignedMin() << " and size " << ArgSize << "\n";
    } else {
      unsigned MinArgSize = getConstantLength(ArgRange.getLower().getSExtValue(), Base, Dir.Precision, MaybeSign, MaybeBase);
      unsigned MaxArgSize = getConstantLength(ArgRange.getUpper().getSExtValue(), Base, Dir.Precision, MaybeSign, MaybeBase);
      DirRes.Range.Min =  DirRes.Range.Likely = MinArgSize;
      DirRes.Range.Max = DirRes.Range.Unlikely = MaxArgSize;
      llvm::dbgs() << "Adding range int with min value " << ArgRange.getSignedMin() << " size " << MinArgSize << 
        " max value " << ArgRange.getSignedMax() << " and size " << MaxArgSize << "\n";
    }
    // TODO: skip any extend instructions
  }

  unsigned Adj = (Sign | MaybeBase) + (Base == 2 || Base == 16);
  if (Dir.Precision) {
    llvm::dbgs() << "adjusing precision" << Dir.Precision.has_value() << "\n";
    DirRes.adjust(Dir.Precision->first, Dir.Precision->second, T, Base, Adj);
  }
  if (Dir.Width) {
    llvm::dbgs() << "adjusing width " << Dir.Width.has_value() << "\n";
    DirRes.adjust(*Dir.Width, *Dir.Width, T, Base, Adj);
  }
  
  llvm::dbgs() << "adjustions finished\n";

  return DirRes;
}

PreservedAnalyses FormatStringBoundsPass::run(Function &F,
                                      FunctionAnalysisManager &AM) {
  // auto &FAM = AM.getResult<FunctionAnalysisManagerModuleProxy>(F).getManager();
  auto &TLI = AM.getResult<TargetLibraryAnalysis>(F);
  auto &LVI = AM.getResult<LazyValueAnalysis>(F);

  // F.getContext().diagnose(DiagnosticInfoGenericWithLoc("Example warning", F, F.getSubprogram(),
  //                                     llvm::DS_Warning));

  for (auto &BB : F) {
    for (auto &Inst : BB) {
      if (CallInst *CI = dyn_cast<CallInst>(&Inst)) {
        // llvm::dbgs() << "Processing call inst\n";
        // CI->dump();

        LibFunc CallFunc;
        if(!TLI.getLibFunc(*CI->getCalledFunction(), CallFunc))
          continue;
        int idx_dstptr = 0, idx_dstsize = std::numeric_limits<int>().max(), idx_format;

        switch (CallFunc) {
          case LibFunc_vsprintf:
          case LibFunc_sprintf:
          case LibFunc_sprintf_chk:

            idx_dstsize = -1;
            idx_format = 1;

            break;

          case LibFunc_snprintf:
          case LibFunc_vsnprintf:
          case LibFunc_snprintf_chk:

            idx_dstsize = 1;
            idx_format = 2;

            break;

          default:
            llvm::dbgs() << "Not a print call\n";
            // idx_dstptr = -1;
            // idx_dstsize = -1;
            // break;
            continue;
        }

        llvm::dbgs() << "Found call to print with format idx: " << idx_format << ", dstptr" << idx_dstptr << ", dstsize " << idx_dstsize << "\n";
        CI->dump();

        // Process the format string.
        Value *FormatStrValue = CI->getArgOperand(idx_format);
        // Diagnose and exit if the format str is null.
        if (isa<ConstantPointerNull>(FormatStrValue)) {
          const char *Msg = "Null format string";
          F.getContext().diagnose(DiagnosticInfoGenericWithLoc(Msg, F, CI->getDebugLoc(), llvm::DS_Warning));
          continue;
        }
        // The format string must be a constant for this check to work.
        StringRef FormatStr;
        llvm::dbgs() << "trying to parse the format string\n";
        bool HasLiteralFormatStr = getConstantStringInfo(FormatStrValue, FormatStr);
        // If the format isn't literal, there is nothing we can check.
        if (!HasLiteralFormatStr) {
          llvm::dbgs() << "failed to parse fomrmat string\n";
          continue;
        }
        llvm::dbgs() << "parsed fmt string '" << FormatStr << "'\n";
      
        bool Bounded = idx_dstsize != -1;
        uint64_t DestSize = 0;
        llvm::dbgs() << "is bounded " << Bounded << "\n";

        if (Bounded) {
          // For bounded functions try to eval the size arg
          Value *DestSizeValue = CI->getArgOperand(idx_dstsize);
          ConstantInt *DestSizeConst = dyn_cast<ConstantInt>(DestSizeValue);
          if (DestSizeConst) {
            DestSize = DestSizeConst->getZExtValue();
            llvm::dbgs() << "found constant dest size " << DestSize << "\n";
          } else {
            ConstantRange DestSizeRange = LVI.getConstantRange(DestSizeValue, CI, false);
            DestSize = DestSizeRange.getUnsignedMin().getZExtValue(); // TODO: Use min for level 1 and max for level 2.
            llvm::dbgs() << "calculated min range dest size " << DestSize << "\n";
          }
        } else {
          // For unbounded functions, try to determine the size based on the destination buffer.
          Value *DestPtrValue = CI->getArgOperand(idx_dstptr);
          DestSize = GetStringLength(DestPtrValue); // returns 0 if unsuccessful, size includes null byte.
          llvm::dbgs() << "calculated dest size " << DestSize << "\n";
        }

        // If we can't determine the size, there is no point to continue.
          if (DestSize == 0)
            continue;

        // TODO: Test DestSize is not greater than size_t/2 and int_max
        // F.getDataLayout().int

        // TODO: Compute the dest buffer origin and determine the offset. Why?


        llvm::dbgs() << "started formatting\n";
        FormatResult Res;

        // // Now process the format directives and determine the dstsize vs the actual min/max size.
        // unsigned CurrentSize = 0;
        // unsigned CurrentArg = 0; // Or idx_format + 1
        unsigned CurrentArg = idx_format + 1;
        // // for (unsigned Idx = 0; Idx < FormatStr.size(); ++Idx) {
        // //   char C = FormatStr[Idx];
        for (const char *CharIt = FormatStr.begin(); CharIt != FormatStr.end(); /*++CharIt*/) {
          FormatResult DirRes;

        FormatDirective Dir = ParseDirective(FormatStr, CharIt, CI, CurrentArg);
        llvm::dbgs() << "Parsed Directive with specifier '" << Dir.Specifier << "' and length " << Dir.Length << "\n";
        if (Dir.Specifier == ' ' || Dir.Length == 0) {
          llvm::errs() << "error: bad specifier\n";
          break;
        }
        CharIt += Dir.Length;

          // TODO interpret length sub-specifiers: hh,h,l,ll,j,z,t,L

          // Interpret format specifier
          // TODO: How to handle variadic functions? we can't just retrieve args with getArgOperand, instead we have to find the va_list ?
          switch(Dir.Specifier) {
            case 'd': 
            case 'i':
            case 'u':
            case 'o': 
            // x and X differ only in the uppercase/lowercase, the length is the same for both
            case 'x':
            case 'X': {
              llvm::dbgs() << "formatting %" << Dir.Specifier << " for arg " << CurrentArg << "\n";
              Value *Val = CI->getArgOperand(CurrentArg);
              Val->print(llvm::errs(), true);
              Val->printAsOperand(llvm::errs());

              DirRes = formatInteger(Dir, CI, CurrentArg, LVI);
              break;
            }
          
            case 's': {
              llvm::dbgs() << "formatting %s for arg " << CurrentArg << "\n";
              Value *Val = CI->getArgOperand(CurrentArg);
              Val->print(llvm::errs(), true);
              llvm::errs() << "\n";
              Val->printAsOperand(llvm::errs());
              llvm::errs() << "\n";
              // Val->uses
              Val->getType()->print(llvm::errs(), true, false);
              llvm::errs() << "\n";
              StringRef Res;
              bool Found = getConstantStringInfo(Val, Res);
              if (!Found) {
                llvm::dbgs() << "not a constant string\n";
                // TODO: Try to follow memcpys.
                continue;
              }
              // For constant string we know the exact length
              DirRes.Range = FormatRange(Res.size());
              // CurrentSize += Res.size();
              // llvm::dbgs() << "Adding int with min size " << Res.size() << "\n";
              // llvm::dbgs() << "That bring the current estimated size to " << CurrentSize << "/" << DestSize << "\n";
              // // if (CurrentSize >= DestSize) {
              // //   F.getContext().diagnose(DiagnosticInfoGenericWithLoc("Output will be truncated ", F, F.getSubprogram(),
              // //                             llvm::DS_Warning));
              // // }
              // ++CurrentArg;
              // ++CharIt;

              break;
            }
            case 'c': {
              llvm::dbgs() << "formatting %c for arg " << CurrentArg << "\n";
              // Value *Val = CI->getArgOperand(CurrentArg)
              // TODO: Test, prints 1 character
              DirRes.Range.Min = DirRes.Range.Max = DirRes.Range.Likely = DirRes.Range.Unlikely = 1;
              DirRes.KnownRange = true;
              break;
            }
            case '%': // Literal '%' char:
              llvm::dbgs() << "ignoring %s for arg " << CurrentArg << "\n";
              DirRes.Range.Min = DirRes.Range.Max = DirRes.Range.Likely = DirRes.Range.Unlikely = 1;
              DirRes.KnownRange = true;
              break;
            case 'n':
              // Nothing is printed, by the current length is stored in the current arg.
              // So we increment by 0 and move to the next arg
              llvm::dbgs() << "formatting %% and keeping arg " << CurrentArg << "\n";
              DirRes.Range.Min = DirRes.Range.Max = DirRes.Range.Likely = DirRes.Range.Unlikely = 0;
              DirRes.KnownRange = true;
              break;
            case '$':
              llvm::dbgs() << "formatting literal '" << StringRef(Dir.BeginPos, Dir.Length) << "'\n";
              DirRes.Range.Min = DirRes.Range.Max = DirRes.Range.Likely = DirRes.Range.Unlikely = Dir.Length;
              // Res.Range.Min += Dir.Length;
              // Res.Range.Min += Dir.Length;
              // Res.Range.Min += Dir.Length;
              // Res.Range.Min += Dir.Length;
              break;

            default:
              llvm::errs() << "Unknown specifier '" << Dir.Specifier << "'\n"; 
              // Don't increment the current size, and move on. This should never happen, we should handle all cases.
              // ++CharIt;
              continue;
          }

          // Update the total size based on the directive size.
          Res.KnownRange &= DirRes.KnownRange;


          llvm::dbgs() << "new directive likely " << DirRes.Range.Likely << " dest size " << DestSize << " " << " current res likely " << Res.Range.Likely << "\n";
          if (DirRes.Range.Likely >= DestSize - Res.Range.Likely) {
            F.getContext().diagnose(DiagnosticInfoGenericWithLoc("Output will be truncated", F, F.getSubprogram(),
                                      llvm::DS_Warning));
            break;
          }

          if (Res.Range.Max < std::numeric_limits<int>().max() && DirRes.Range.Max < std::numeric_limits<int>().max()) {
            Res.Range.Max += DirRes.Range.Max;
          }

          // TODO: Overflow check.
          if (DirRes.Range.Max < DirRes.Range.Unlikely) {
            Res.Range.Unlikely += DirRes.Range.Unlikely;
          } else {
            Res.Range.Unlikely += DirRes.Range.Max;
          }

          Res.Range.Min += DirRes.Range.Min;
          Res.Range.Likely += DirRes.Range.Likely;
          
          llvm::dbgs() << "That bring the current estimated size to " << Res.Range.Min << "(" << Res.Range.Likely << ")" << "/" << DestSize << "\n";
          if (Dir.Specifier != '$' && Dir.Specifier != '%') // Increment the arg if this is not a literal from the format string
            ++CurrentArg;
          // ++CharIt;
          // break;
        }
      }
    }
  }

  return PreservedAnalyses::all();
}
