#include "llvm/Transforms/Scalar/FormatStringBounds.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/FloatingPointMode.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Analysis/LazyValueInfo.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/ErrorHandling.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <optional>

using namespace llvm;

FormatStringBoundsPass::FormatStringBoundsPass(int Level) : Level(Level){};

static unsigned getConstantLength(
    long long Constant, unsigned Base,
    std::optional<std::pair<unsigned, unsigned>> Precision = std::nullopt,
    bool PlusFlag = false, bool Prefix = true) {
  llvm::errs() << "getConstantLength constant " << Constant << " base " << Base
               << " precision " << (Precision ? Precision->first : -1)
               << " plus " << PlusFlag << " prefix " << Prefix << "\n";
  unsigned Length = 0;
  long long Value = Constant;

  if (Value < 0) {
    ++Length;       // Add space for sign.
    Value = -Value; // Convert to absolute value.
  } else if (PlusFlag) {
    ++Length; // Also add space for sign if the value is non-negative but plus
              // flag is specified.
  }

  unsigned NumDigits = 0;
  while (Value != 0) {
    Value = Value / Base;
    ++NumDigits;
  }
  Length += NumDigits;

  if (Prefix && Constant != 0) {
    if (Base == 2 || Base == 16)
      Length += 2;
    else if (Base == 8 && Precision && Precision->first <= NumDigits)
      Length++;
  }

  return Length;
}

struct FormatDirective {
  bool FlagPlus = false; // Always add a sign, even if the number is positive
  bool FlagSpace = false;
  bool FlagHash = false;
  std::optional<std::pair<unsigned, unsigned>> Width = std::nullopt;
  std::optional<std::pair<unsigned, unsigned>> Precision = std::nullopt;

  // unsigned ArgNo = 0;

  enum Modifiers { NONE, hh, h, l, ll, j, z, t, L };
  char Specifier = ' ';
  Modifiers Modifier = NONE;

  const char *BeginPos = nullptr;
  unsigned Length = 0;
};

static unsigned parseIntegerLiteral(const char *&Val) {
  unsigned Literal = 0;
  while (isDigit(*Val)) {
    Literal = Literal * 10 + (*Val - '0');
    ++Val;
  }
  return Literal;
}

#define SPECIFIER_LITERAL '$'

static FormatDirective getLiteralDirective(const char *Begin, const char *End) {
  llvm::errs() << "literal directive '" << StringRef(Begin, End - Begin)
               << "'\n";
  FormatDirective Dir;
  Dir.BeginPos = Begin;
  Dir.Length = End - Begin;
  Dir.Specifier = SPECIFIER_LITERAL;
  return Dir;
}

static FormatDirective parseDirective(StringRef FormatStr, const char *Begin,
                                      CallInst *CI, unsigned &ArgNo,
                                      LazyValueInfo &LVI) {
  FormatDirective Dir;
  Dir.BeginPos = Begin;

  // First check if this is a directive, if not return as if it was a literal.
  const char *DirectiveStart = Begin;
  while (*DirectiveStart != '%' && DirectiveStart != FormatStr.end())
    ++DirectiveStart;
  if (DirectiveStart != Begin) {
    return getLiteralDirective(Begin, DirectiveStart);
  }

  // The directive is not a literal.
  const char *CharIt = Begin;
  ++CharIt; // Move from '%'
  if (CharIt >= FormatStr.end()) {
    llvm::errs() << "Invalid directive 1\n";
    return getLiteralDirective(DirectiveStart, CharIt);
  }

  llvm::errs() << "found directive\n";

  // Interpret directive flags.
  while (CharIt != FormatStr.end()) {
    switch (*CharIt) {
    case '-':
    case '0':
      // these do not affect the length.
      ++CharIt;
      continue;
    case '+':
      // the sign of signed conversions is always prepended to the result of the
      // conversion (by default the result is preceded by minus only when it is
      // negative)
      ++CharIt;
      Dir.FlagPlus = true;
      continue;
    case ' ':
      // if the result of a signed conversion does not start with a sign
      // character, or is empty, space is prepended to the result. It is ignored
      // if + flag is present
      ++CharIt;
      Dir.FlagSpace = true;
      continue;
    case '#':
      // alternative form of the conversion is performed.
      ++CharIt;
      Dir.FlagHash = true;
      continue;
    }
    // Invalid flag character, break this loop and check the next part.
    break;
  }
  if (CharIt >= FormatStr.end()) {
    llvm::errs() << "Invalid directive 2\n";
    return getLiteralDirective(DirectiveStart, CharIt);
  }

  // Interpret the field width.
  if (*CharIt == '*') {
    // the width is specified by an additional argument of type int, which
    // appears before the argument to be converted and the argument supplying
    // precision if one is supplied
    ++CharIt;
    // Check if the argument exists.
    if (ArgNo < CI->getNumOperands()) {
      // Note that this is the minimum based on the format string, the actual
      // minimum dependent on the actual argument could be more.
      Value *ArgWidthValue = CI->getArgOperand(ArgNo);
      ConstantInt *ArgWidthConst = dyn_cast<ConstantInt>(ArgWidthValue);
      if (ArgWidthConst) {
        // If the value is negative then '-' flag should be applied and the
        // value treated as positive. We're not interested in the '-' flag, so
        // we ignore that part.
        auto ArgWidth = std::abs(ArgWidthConst->getSExtValue());
        Dir.Width = {ArgWidth, ArgWidth};
      } else {
        // If the value is not constant use LVI to find the bounds.
        ConstantRange ArgRange = LVI.getConstantRange(ArgWidthValue, CI, false);
        Dir.Width = {ArgRange.getUnsignedMin().getZExtValue(),
                     ArgRange.getUnsignedMax().getZExtValue()};
      }
    } else {
      // The argument is supplied via varargs and we can't use it.
      Dir.Width = std::nullopt;
    }
    ++ArgNo;
  } else if (isDigit(*CharIt)) {
    // The width is specified in the format string.
    auto Width = parseIntegerLiteral(CharIt);
    Dir.Width = {Width, Width};
  }
  if (CharIt >= FormatStr.end()) {
    llvm::errs() << "Invalid directive 3\n";
    return getLiteralDirective(DirectiveStart, CharIt);
  }

  // Interpret the field precision.
  if (*CharIt == '.') {
    ++CharIt;
    if (*CharIt == '*') {
      // the precision is specified by an additional argument of type int, which
      // appears before the argument to be converted, but after the argument
      // supplying minimum field width if one is supplied
      ++CharIt;
      if (ArgNo < CI->getNumOperands()) {
        // Note that this is the minimum based on the format string, the actual
        // minimum dependent on the actual argument could be more.
        Value *ArgPrecisionValue = CI->getArgOperand(ArgNo);
        ConstantInt *ArgPrecisionConst =
            dyn_cast<ConstantInt>(ArgPrecisionValue);
        if (ArgPrecisionConst) {
          // Break if this is negative, that is not a valid case.
          if (ArgPrecisionConst->isNegative()) {
            llvm::errs() << "Invalid directive 4\n";
            return getLiteralDirective(DirectiveStart, CharIt);
          }
          auto ArgPrecision = ArgPrecisionConst->getZExtValue();
          Dir.Precision = {ArgPrecision, ArgPrecision};
        } else {
          // If the value is not constant use LVI to find the bounds.
          ConstantRange ArgRange =
              LVI.getConstantRange(ArgPrecisionValue, CI, false);
          Dir.Precision = {ArgRange.getUnsignedMin().getZExtValue(),
                           ArgRange.getUnsignedMax().getZExtValue()};
        }
      } else {
        // The width is specified in the format string.
        Dir.Precision = std::nullopt;
      }
      ++ArgNo;
    } else if (isDigit(*CharIt)) {
      // The width is specified in the format string.
      unsigned Precision = parseIntegerLiteral(CharIt);
      Dir.Precision = {Precision, Precision};
    } else {
      // Precision is implicitly zero when the radix is specified without a
      // value after it.
      Dir.Precision = {0, 0};
    }
  }
  if (CharIt >= FormatStr.end()) {
    llvm::errs() << "Invalid directive 5\n";
    return getLiteralDirective(DirectiveStart, CharIt);
  }

  // Interpret length modifier: hh,h,l,ll,j,z,t,L
  switch (*CharIt) {
  case 'h':
    if (*(CharIt + 1) == 'h') {
      Dir.Modifier = FormatDirective::hh;
      ++CharIt;
    } else {
      Dir.Modifier = FormatDirective::h;
    }
    break;
  case 'l':
    if (*(CharIt + 1) == 'l') {
      Dir.Modifier = FormatDirective::ll;
      ++CharIt;
    } else {
      Dir.Modifier = FormatDirective::l;
    }
    break;
  case 'j':
    Dir.Modifier = FormatDirective::j;
    break;
  case 'z':
    Dir.Modifier = FormatDirective::z;
    break;
  case 't':
    Dir.Modifier = FormatDirective::t;
    break;
  case 'L':
    Dir.Modifier = FormatDirective::L;
    break;
  default:
    Dir.Modifier = FormatDirective::NONE;
    break;
  }
  if (Dir.Modifier != FormatDirective::NONE)
    ++CharIt;
  if (CharIt >= FormatStr.end()) {
    llvm::errs() << "Invalid directive 6\n";
    return getLiteralDirective(DirectiveStart, CharIt);
  }

  Dir.Specifier = *CharIt;
  assert(StringRef("diuoxXfFeEgGaAcspn%").contains(Dir.Specifier) &&
         "Invalid format specifier");

  Dir.Length = CharIt - Begin + 1;
  return Dir;
}

class FormatResult {
public:
  size_t MinLength;
  bool KnownRange;

  FormatResult() : MinLength(0), KnownRange(false) {}

  void adjust(std::optional<long long> AdjMin);
};

void FormatResult::adjust(std::optional<long long> AdjMin) {
  if (AdjMin && MinLength < AdjMin) {
    MinLength = *AdjMin;
  }
}

static FormatResult formatInteger(const FormatDirective &Dir, CallInst *CI,
                                  unsigned CurrentArg, LazyValueInfo &LVI) {
  FormatResult DirRes;

  bool MaybeBase = Dir.FlagHash;
  bool MaybeSign = false;
  // bool Sign = false;
  unsigned Base;

  if (Dir.Specifier == 'd' || Dir.Specifier == 'i') {
    Base = 10;
    MaybeSign = Dir.FlagSpace || Dir.FlagPlus;
    // Sign = true;
  } else if (Dir.Specifier == 'x' || Dir.Specifier == 'X')
    Base = 16;
  else if (Dir.Specifier == 'o') {
    Base = 8;
  } else {
    llvm_unreachable("Unsupported numeric specifier");
  }

  // Type *T;
  if (CurrentArg >= CI->getNumOperands()) {
    // Argument not provided, assume it is one byte long.
    DirRes.MinLength = 1;
  } else {
    Value *Val = CI->getArgOperand(CurrentArg);
    // Type *ArgType = Val->getType();
    // T = ArgType;

    llvm::errs() << "Found arg\n";
    Val->print(llvm::errs());
    llvm::errs() << "\n";

    // std::optional<unsigned> TypeSize = std::nullopt;
    // std::optional<bool> IsSigned = std::nullopt;
    // if (const auto *Int = dyn_cast<ConstantInt>(Val)) {
    //   llvm::errs() << "arg is a constant, using that type\n";
    //   TypeSize = Int->getBitWidth();
    // } else if (const auto *ZEI = dyn_cast<ZExtInst>(Val)) {
    //   IsSigned = false;
    //   llvm::errs() << "determined type is unsigned from zext instruction\n";
    //   TypeSize = ZEI->getSrcTy()->getIntegerBitWidth();
    //   llvm::errs() << "using source type with width " << TypeSize.value()
    //                << "\n";
    // } else if (const auto *SEI = dyn_cast<SExtInst>(Val)) {
    //   IsSigned = true;
    //   llvm::errs() << "determined type is signed from sext instruction\n";
    //   TypeSize = SEI->getSrcTy()->getIntegerBitWidth();
    //   llvm::errs() << "using source type with width " << TypeSize.value()
    //                << "\n";
    // } else {
    //   llvm::errs() << "determining type based on debug info\n";
    //   SmallVector<DbgVariableIntrinsic *> DbgUsers;
    //   findDbgUsers(DbgUsers, Val);
    //   llvm::errs() << "point 1\n";
    //   // TODO: Multiple dbg values? Why in a loop, use just the last one?
    //   for (const auto *User : llvm::reverse(DbgUsers)) {
    //     llvm::errs() << "point 2\n";
    //     DILocalVariable *Var = User->getVariable();
    //     Var->getType()->print(llvm::errs(), CI->getModule(), true);
    //     Var->getRawType()->print(llvm::errs(), CI->getModule(), true);
    //     llvm::errs() << "type size is " << Var->getSizeInBits().value_or(0)
    //                  << " bits\n";
    //     llvm::errs() << "type signedness is "
    //                  << (Var->getSignedness().has_value()
    //                          ? -1
    //                          : (int)Var->getSignedness().value())
    //                  << "(Unknown, Signed, Unsigned)\n";
    //     if (Var->getSignedness().has_value())
    //       IsSigned = Var->getSignedness() == DIBasicType::Signedness::Signed;
    //     if (Var->getSizeInBits())
    //       TypeSize = Var->getSizeInBits().value();
    //     if (IsSigned && TypeSize)
    //       break;
    //   }
    //   if (!IsSigned.has_value())
    //     llvm::errs() << "unable to determine is type signed\n";
    //   if (!TypeSize.has_value())
    //     llvm::errs() << "unable to determine type size\n";
    // }
    // if (TypeSize.has_value() && IsSigned.has_value()) {
    //   llvm::errs() << "found type size and signedness\n";
    // }

    // unsigned ModifierSize;
    // bool ModifierSigned;
    // switch (Dir.Modifier) {
    // case FormatDirective::Modifiers::NONE:
    //   ModifierSize = 32;
    //   ModifierSigned = (Dir.Specifier == 'd' || Dir.Specifier == 'i');
    //   break;
    // case FormatDirective::Modifiers::z:
    //   ModifierSize = 64;
    //   ModifierSigned = false;
    //   break;
    // default:
    //   llvm::errs() << "Modifier '" << Dir.Modifier << "'\n";
    //   assert(false && "Modifier not implemented");
    //   break;
    // }

    // Try to evaluate this number.
    ConstantRange ArgRange = LVI.getConstantRange(Val, CI, false);
    llvm::errs() << "determined range (u) " << ArgRange.getUnsignedMin() << " "
                 << ArgRange.getUnsignedMax() << "\n";
    llvm::errs() << "determined range (s) " << ArgRange.getSignedMin() << " "
                 << ArgRange.getSignedMax() << "\n";
    if (ArgRange.isSingleElement()) {
      unsigned ArgSize =
          getConstantLength(ArgRange.getLower().getSExtValue(), Base,
                            Dir.Precision, MaybeSign, MaybeBase);
      // Special case: precision of 0 doesn't print the 0 constant.
      if (Dir.Precision && Dir.Precision->first == 0 &&
          Dir.Precision->second == 0 &&
          ArgRange.getLower().getSExtValue() == 0 &&
          !((Base == 8 && Dir.FlagHash) || MaybeSign))
        ArgSize = 0;
      DirRes.MinLength = ArgSize;
      DirRes.KnownRange = true;
      llvm::errs() << "Adding const int with min value "
                   << ArgRange.getSignedMin() << " and size " << ArgSize
                   << "\n";
    } else {
      // TODO: If the range covers 0 then set MinArgSize to 1.
      unsigned MinArgSize =
          getConstantLength(ArgRange.getLower().getSExtValue(), Base,
                            Dir.Precision, MaybeSign, MaybeBase);
      DirRes.MinLength = MinArgSize;
      llvm::errs() << "Adding range int with min value "
                   << ArgRange.getSignedMin() << " size " << MinArgSize
                   << " max value " << ArgRange.getSignedMax() << "\n";
    }
  }

  // Already done in getConstantLength
  // unsigned Adj = (Sign | MaybeBase) + (Base == 2 || Base == 16);
  // DirRes.MinLength += Adj;
  if (Dir.Precision) {
    llvm::errs() << "adjusting precision" << Dir.Precision.has_value() << "\n";
    DirRes.adjust(Dir.Precision->first);
  }
  if (Dir.Width) {
    llvm::errs() << "adjusting width " << Dir.Width.has_value() << "\n";
    DirRes.adjust(Dir.Width->first);
  }

  llvm::errs() << "adjustment finished\n";

  return DirRes;
}

static FormatResult formatString(const FormatDirective &Dir, CallInst *CI,
                                 unsigned CurrentArg,
                                 const TargetLibraryInfo &TLI) {
  FormatResult DirRes;

  if (CurrentArg >= CI->getNumOperands()) {
    DirRes.MinLength = 0;
    DirRes.KnownRange = false;
    return DirRes;
  }

  Value *Val = CI->getArgOperand(CurrentArg);
  Val->print(llvm::errs(), true);
  llvm::errs() << "\n";
  Val->getType()->print(llvm::errs(), true, false);
  llvm::errs() << "\n";

  if (const GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(Val)) {
    GEP->getSourceElementType()->print(llvm::errs(), true);
    llvm::errs() << "\n";

    bool Found = false;
    bool Valid = true;
    size_t CurrentLen = std::numeric_limits<size_t>().max();
    llvm::errs() << "going through GEP uses\n";
    for (const auto &U : GEP->uses()) {
      llvm::errs() << "found use\n";
      U.getUser()->print(llvm::errs(), true);
      llvm::errs() << "\n";

      if (U.getUser() == CI || U.getUser() == GEP) {
        llvm::errs() << "skipping use in the print call or gep itself\n";
        continue;
      }

      Found = true;
      if (const auto *SI = dyn_cast<StoreInst>(U.getUser())) {
        llvm::errs() << "found store inst\n";
        if (const auto *CI = dyn_cast<ConstantInt>(SI->getValueOperand())) {
          // memcpy can be optimized as an integer store. Treat the number as a
          // string.
          CurrentLen = std::min<size_t>(CurrentLen, CI->getBitWidth() + 7 / 8);
        } else {
          llvm::errs() << "not a valid const. can't determine size\n";
          Valid = false;
          break;
        }
      } else if (const auto *MCI = dyn_cast<MemCpyInst>(U.getUser())) {
        llvm::errs() << "found memcpy inst\n";
        // For memcpy find the argument that represents the number of chars to
        // copy.
        if (const auto *CI = dyn_cast<ConstantInt>(MCI->getLength())) {
          CurrentLen = std::min<size_t>(CurrentLen, CI->getZExtValue());
        } else {
          llvm::errs() << "not a valid const. can't determine size\n";
          Valid = false;
          break;
        }
      } else if (const auto *CI = dyn_cast<CallInst>(U.getUser());
                 CI && CI->getCalledFunction()) {
        LibFunc CallFunc;
        if (!TLI.getLibFunc(*CI->getCalledFunction(), CallFunc)) {
          Valid = false;
          break;
        }
        if (CallFunc == llvm::LibFunc_strcpy) {
          llvm::errs() << "found strcpy inst\n";
          llvm::errs() << "source operand type:\n";
          CI->getOperand(1)->getType()->print(llvm::errs(), true);
          llvm::errs() << "\n";
          llvm::errs() << "source operand:\n";
          CI->getOperand(1)->print(llvm::errs(), true);
          llvm::errs() << "\n";

          StringRef Str;
          if (getConstantStringInfo(CI->getOperand(1), Str)) {
            CurrentLen = std::min(CurrentLen, Str.size());
            Valid = true;
          } else {
            llvm::errs() << "not a constant string\n";
            Valid = false;
            break;
          }
        } else if (CallFunc == llvm::LibFunc_strncpy) {
          if (const auto *Len = dyn_cast<ConstantInt>(CI->getOperand(2))) {
            CurrentLen = std::min(CurrentLen, Len->getZExtValue());
            Valid = true;
          } else {
            llvm::errs() << "length not a constant int\n";
            Valid = false;
            break;
          }
        } else {
          llvm::errs() << "Found unsupported call\n";
          Valid = false;
          break;
        }
      } else {
        llvm::errs() << "Found unsupported use\n";
        Valid = false;
        // break;
      }
    }

    llvm::errs() << "done with uses\n";
    if (Found && Valid) {
      llvm::errs() << "Setting range to " << CurrentLen << "\n";
      DirRes.MinLength = CurrentLen;
      if (Dir.Precision) {
        // Range is limited by the upper precision bound
        llvm::errs() << "Limiting range to " << Dir.Precision->second
                     << " because of precision\n";
        DirRes.MinLength =
            std::min<long long>(Dir.Precision->second, DirRes.MinLength);
      }
    } else {
      llvm::errs() << "unknown string length range, trying to use type size\n";
      if (GEP->getSourceElementType()->isArrayTy()) {
        llvm::errs() << "found array type with n_elements "
                     << GEP->getSourceElementType()->getArrayNumElements()
                     << "\n";
        DirRes.MinLength = GEP->getSourceElementType()->getArrayNumElements();
        if (Dir.Precision) {
          // Range is limited by the upper precision bound.
          llvm::errs() << "Limiting range to " << Dir.Precision->second
                       << " because of precision\n";
          DirRes.MinLength =
              std::min<long long>(Dir.Precision->second, DirRes.MinLength);
        }
        DirRes.KnownRange = false;
      } else {
        llvm::errs() << "unable to find estimate, assuming size 0\n";
        DirRes.MinLength = 0;
        DirRes.KnownRange = false;
      }
    }
  } else {
    llvm::errs() << "not a GEP instr\n";
    StringRef Res;
    bool Found = getConstantStringInfo(Val, Res);
    if (!Found) {
      llvm::errs() << "not a constant string\n";
      // TODO: Try to follow memcpys.
      DirRes.MinLength = 0;
      DirRes.KnownRange = false;
    } else {
      // For constant string we know the exact length
      DirRes.MinLength = Res.size();
      if (Dir.Precision) {
        // Range is limited by the upper precision bound.
        llvm::errs() << "Limiting range to " << Dir.Precision->second
                     << " because of precision\n";
        DirRes.MinLength =
            std::min<long long>(Dir.Precision->second, DirRes.MinLength);
      }
    }
  }

  return DirRes;
}

static FormatResult formatFloat(const FormatDirective &Dir, CallInst *CI,
                                unsigned CurrentArg) {
  FormatResult DirRes;
  DirRes.KnownRange = true;

  unsigned MinIntLength = 1;
  if (CurrentArg < CI->getNumOperands()) {
    if (const auto *CFP = dyn_cast<ConstantFP>(CI->getArgOperand(CurrentArg))) {
      if (CFP->isInfinity() || CFP->isNaN()) {
        // The argument is a constant inf or nan.
        bool HasSign = Dir.FlagPlus || CFP->isNegative();
        DirRes.MinLength = StringRef("inf").size() + HasSign;
        DirRes.KnownRange = false;

        // Adjust the range for width but ignore precision.
        if (Dir.Width) {
          DirRes.adjust(Dir.Width->first);

          // Is the width dictated by the width?
          if (DirRes.MinLength > Dir.Width->first)
            DirRes.KnownRange = false;
        }

        return DirRes;
      }

      // The argument is a constant number.
      APSInt TruncatedOperand;
      bool IsExact;
      if (CFP->getValue().convertToInteger(TruncatedOperand,
                                           RoundingMode::TowardZero,
                                           &IsExact) == APFloatBase::opOK) {
        auto IntVal = TruncatedOperand.getSExtValue();
        MinIntLength = getConstantLength(IntVal, 10);
        llvm::errs() << "Found " << MinIntLength
                     << " digits before the radix\n";
      } else {
        llvm::errs() << "op failed\n";
      }
    }
  }

  std::pair<unsigned, unsigned> EffectivePrecision;
  if (Dir.Precision) {
    EffectivePrecision = *Dir.Precision;
    llvm::errs() << "using set precision " << EffectivePrecision.first << "-"
                 << EffectivePrecision.second << "\n";
  } else if (llvm::toUpper(Dir.Specifier) == 'A') {
    llvm::errs() << "precision not specified, using 1 as default\n";
    // Default precision for %a is 0.
    EffectivePrecision = {0, 0};
  } else {
    llvm::errs() << "precision not specified, using 6 as default\n";
    // Default precision for other specifiers is 6.
    EffectivePrecision = {6, 6};
  }

  // Estimate length based on specifier, width and precision.
  llvm::errs() << "estimating based on specifier\n";
  // We have no idea what the range is, set the minimum according to the
  // specifier and precision. Precision is guaranteed to be initialized
  // here
  bool HasRadix =
      EffectivePrecision.first > 0; // For the radix if precision is not 0

  DirRes.MinLength = EffectivePrecision.first + HasRadix +
                     1; // there is always a digit before the radix
  llvm::errs() << "current hasRadix " << HasRadix << " min " << DirRes.MinLength
               << "\n";

  if (Dir.Specifier == 'f' || Dir.Specifier == 'F') {
    // Use the integer value to get number of digits before the radix. One digit
    // is already included by default.
    DirRes.MinLength += MinIntLength - 1;
    if (MinIntLength > 1)
      DirRes.KnownRange = false;
    llvm::errs() << "setting F range to " << DirRes.MinLength << "\n";
  } else if (Dir.Specifier == 'e' || Dir.Specifier == 'E') {
    // Examples: 1.000000e-01, 0.000000e+00
    DirRes.MinLength += 4; // for e+00
    llvm::errs() << "setting E to " << DirRes.MinLength << "\n";
  } else if (Dir.Specifier == 'a' || Dir.Specifier == 'A') {
    // Examples: 0x1p+2, 0x1.2p+2, 0x1.47ae147ae147bp-8
    DirRes.MinLength += 5; // for 0x and p+0
    llvm::errs() << "setting A range to " << DirRes.MinLength << "\n";
  } else if (Dir.Specifier == 'g' || Dir.Specifier == 'G') {
    DirRes.MinLength = 1;
  } else {
    assert(false && "Unknown float specifier");
    DirRes.MinLength = 1;
  }

  // Adjust the range for width but ignore precision.
  if (Dir.Width) {
    DirRes.adjust(Dir.Width->first);

    // Is the width dictated by the width?
    if (DirRes.MinLength > Dir.Width->first)
      DirRes.KnownRange = false;
  }

  return DirRes;
}

static FormatResult formatPointer(const FormatDirective &Dir, CallInst *CI,
                                  unsigned CurrentArg) {
  FormatResult DirRes;

  // NOTE: Pointers printing is implementation and target defined.
  // These are just estimates.
  Value *Val = CI->getArgOperand(CurrentArg);
  if (isa<ConstantPointerNull>(Val)) {
    llvm::errs() << "pointer is null const, using range 5\n";
    // Null pointer prints "(nil)" in both clang and gcc.
    DirRes.MinLength = StringRef("(nil)").size();
    DirRes.KnownRange = false;
  } else {
    llvm::errs() << "pointer is not null, estimating based on size\n";
    unsigned PtrSize =
        CI->getFunction()->getDataLayout().getPointerSizeInBits(0);
    if (PtrSize == 32) {
      llvm::errs() << "32bit pointer has size 10\n";
      DirRes.MinLength = 10;
      DirRes.KnownRange = false;
    } else if (PtrSize == 64) {
      llvm::errs() << "64bit pointer has size 14, max 18\n";
      DirRes.MinLength = 14;
      DirRes.KnownRange = false;
    } else {
      llvm::errs() << "unknown range for pointer of size " << PtrSize
                   << ", estimating 0\n";
      // Don't estimate, treat as no characters are printed.
      // Effectively don't consider this directive in the calculation.
      DirRes.MinLength = 0;
      DirRes.KnownRange = true;
    }
  }

  return DirRes;
}

PreservedAnalyses FormatStringBoundsPass::run(Function &F,
                                              FunctionAnalysisManager &AM) {
  auto &TLI = AM.getResult<TargetLibraryAnalysis>(F);
  auto &LVI = AM.getResult<LazyValueAnalysis>(F);

  for (auto &BB : F) {
    for (auto &Inst : BB) {
      if (CallInst *CI = dyn_cast<CallInst>(&Inst)) {
        if (!CI->getCalledFunction())
          continue;

        LibFunc CallFunc;
        if (!TLI.getLibFunc(*CI->getCalledFunction(), CallFunc))
          continue;

        unsigned DestPtrIdx = 0;
        std::optional<unsigned> DestSizeIdx = std::nullopt;
        unsigned FormatStringIdx;
        bool IsOverflow = false;

        switch (CallFunc) {
        case LibFunc_sprintf:
        case LibFunc_sprintf_chk:
        case LibFunc_small_sprintf:
        case LibFunc_vsprintf:
          DestSizeIdx = std::nullopt;
          FormatStringIdx = 1;
          IsOverflow = true;
          break;
        case LibFunc_snprintf:
        case LibFunc_snprintf_chk:
        case LibFunc_vsnprintf:
          DestSizeIdx = 1;
          FormatStringIdx = 2;
          IsOverflow = false;
          break;
        default:
          // Not a printf function.
          continue;
        }

        llvm::errs() << "Found call to " << CI->getCalledFunction()->getName()
                     << " with format idx: " << FormatStringIdx
                     << ", dstptr: " << DestPtrIdx
                     << ", dstsize: " << DestSizeIdx << "\n";
        CI->dump();

        assert(FormatStringIdx < CI->getNumOperands() &&
               DestPtrIdx < CI->getNumOperands() &&
               (!DestSizeIdx || *DestSizeIdx < CI->getNumOperands()));

        // Process the format string.
        Value *FormatStrValue = CI->getArgOperand(FormatStringIdx);

        // Exit if the format string is null.
        if (isa<ConstantPointerNull>(FormatStrValue))
          continue;

        // The format string must be a constant for this check to work.
        StringRef FormatStr;
        bool HasLiteralFormatStr =
            getConstantStringInfo(FormatStrValue, FormatStr);
        if (!HasLiteralFormatStr)
          continue;

        uint64_t DestSize = 0;
        if (DestSizeIdx) {
          // For bounded functions try to evaluate the size arg.
          Value *DestSizeValue = CI->getArgOperand(*DestSizeIdx);
          if (ConstantInt *DestSizeConst =
                  dyn_cast<ConstantInt>(DestSizeValue)) {
            DestSize = DestSizeConst->getZExtValue();
          } else {
            ConstantRange DestSizeRange =
                LVI.getConstantRange(DestSizeValue, CI, false);
            // Use the range maximum for level 1 and minimum for level 2.
            if (Level <= 1) {
              DestSize = DestSizeRange.getUnsignedMax().getZExtValue();
            } else {
              DestSize = DestSizeRange.getUnsignedMin().getZExtValue();
            }
          }
        } else {
          // For unbounded functions, try to determine the size based on the
          // destination buffer.
          Value *DestPtrValue = CI->getArgOperand(DestPtrIdx);
          DestSize = GetStringLength(DestPtrValue);
        }
        llvm::errs() << "determined dest size " << DestSize << "\n";

        // If we can't determine the size, there is no point in continuing.
        if (DestSize == 0)
          continue;

        llvm::errs() << "started formatting\n";
        FormatResult Res;

        // Process the format directives and estimate the output size.
        unsigned CurrentArg = FormatStringIdx + 1;
        for (const char *CharIt = FormatStr.begin();
             CharIt != FormatStr.end();) {
          FormatResult DirRes;

          FormatDirective Dir =
              parseDirective(FormatStr, CharIt, CI, CurrentArg, LVI);
          llvm::errs() << "Parsed Directive with specifier '" << Dir.Specifier
                       << "' and length " << Dir.Length << "\n";
          if (Dir.Specifier == ' ' || Dir.Length == 0) {
            // Invalid specifier. Do not continue.
            llvm::errs() << "error: bad specifier\n";
            break;
          }
          CharIt += Dir.Length;

          switch (Dir.Specifier) {
          case 'd':
          case 'i':
          case 'u':
          case 'o':
          case 'x':
          case 'X':
            llvm::errs() << "formatting %" << Dir.Specifier << " for arg "
                         << CurrentArg << "\n";
            DirRes = formatInteger(Dir, CI, CurrentArg, LVI);
            break;
          case 's':
            llvm::errs() << "formatting %s for arg " << CurrentArg << "\n";
            DirRes = formatString(Dir, CI, CurrentArg, TLI);
            break;
          case 'c':
            llvm::errs() << "formatting %c for arg " << CurrentArg << "\n";
            DirRes.MinLength = 1;

            if (Dir.Width)
              DirRes.adjust(Dir.Width->first);

            DirRes.KnownRange = true;
            break;
          case 'p':
            llvm::errs() << "formatting %p for arg " << CurrentArg << "\n";
            DirRes = formatPointer(Dir, CI, CurrentArg);
            break;
          case 'f':
          case 'F':
          case 'e':
          case 'E':
          case 'a':
          case 'A':
          case 'g':
          case 'G':
            llvm::errs() << "formatting %" << Dir.Specifier << " for arg "
                         << CurrentArg << "\n";
            DirRes = formatFloat(Dir, CI, CurrentArg);
            break;
          case '%': // Literal '%' char:
            llvm::errs() << "ignoring %s for arg " << CurrentArg << "\n";
            DirRes.MinLength = 1;
            DirRes.KnownRange = true;
            break;
          case 'n':
            // Nothing is printed, but the current length is stored in the
            // current arg. So we increment by 0 and move to the next arg
            llvm::errs() << "formatting %n and moving from arg " << CurrentArg
                         << "\n";
            DirRes.MinLength = 0;
            DirRes.KnownRange = true;
            break;
          case '$':
            llvm::errs() << "formatting literal '"
                         << StringRef(Dir.BeginPos, Dir.Length) << "'\n";
            DirRes.MinLength = Dir.Length;
            DirRes.KnownRange = true;
            break;

          default:
            llvm::errs() << "Unknown specifier '" << Dir.Specifier << "'\n";
            assert(false && "Unknown specifier");
            continue;
          }

          // Update the total size based on the directive size.
          Res.KnownRange &= DirRes.KnownRange;

          llvm::errs() << "new directive min " << DirRes.MinLength
                       << " dest size " << DestSize << " "
                       << " current res min " << Res.MinLength << "\n";

          Res.MinLength += DirRes.MinLength;

          llvm::errs() << "That bring the current estimated size to "
                       << Res.MinLength << "/" << DestSize << "\n";
          // Increment the arg if this is not a literal from the format string
          if (Dir.Specifier != '$' && Dir.Specifier != '%')
            ++CurrentArg;
        }

        // Add one for terminating nul.
        Res.MinLength++;

        // Emit the diagnostic after all directives are parsed so we know the
        // correct estimate for the minimum length.
        if (Res.MinLength > DestSize) {
          llvm::errs() << "Output will be truncated. Writing min "
                       << Res.MinLength << " into buffer of destination "
                       << DestSize;

          F.getContext().diagnose(DiagnosticInfoFormatStringBounds(
              F, CI->getDebugLoc(), CI->getCalledFunction()->getName(),
              IsOverflow, Res.MinLength, DestSize));
        }
      }
    }
  }

  return PreservedAnalyses::all();
}
