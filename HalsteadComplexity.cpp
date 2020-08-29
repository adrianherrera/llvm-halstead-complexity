//===-- HalsteadComplexity.cpp - Halstead complexity ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Computes the Halstead complexity of an LLVM function.
///
/// LLVM instructions are the operators, while the operands to these
/// instructions are the operands.
///
//===----------------------------------------------------------------------===//

#include "llvm/ADT/SmallSet.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

#define DEBUG_TYPE "halstead_complexity"

namespace {

class HalsteadComplexity : public FunctionPass {
private:
  Function *F;
  SmallVector<unsigned, 24> Operators;
  SmallVector<const Value *, 24> Operands;
  SmallSet<unsigned, 24> DistinctOperators;
  SmallPtrSet<const Value *, 24> DistinctOperands;

public:
  static char ID;
  HalsteadComplexity() : FunctionPass(ID) {}

  void getAnalysisUsage(AnalysisUsage &) const override;
  void print(raw_ostream &, const Module *) const override;
  bool runOnFunction(Function &) override;
};

} // anonymous namespace

char HalsteadComplexity::ID = 0;

void HalsteadComplexity::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

void HalsteadComplexity::print(raw_ostream &OS, const Module *) const {
  size_t NumDistinctOperators = this->DistinctOperators.size();
  size_t NumDistinctOperands = this->DistinctOperands.size();
  size_t NumTotalOperators = this->Operators.size();
  size_t NumTotalOperands = this->Operands.size();

  size_t Vocabulary = NumDistinctOperators + NumDistinctOperands;
  size_t ProgramLength = NumTotalOperators + NumTotalOperands;

  float Volume = ProgramLength * Log2(Vocabulary);
  float Difficulty =
      (NumDistinctOperators / 2) * (NumTotalOperands / NumDistinctOperands);
  float Effort = Difficulty * Volume;

  OS << "Halstead complexity of `" << this->F->getName() << ":\n";

  OS << "  # distinct operators: " << NumDistinctOperators << '\n';
  OS << "  # distinct operands: " << NumDistinctOperands << '\n';

  OS << "  # total operators: " << NumTotalOperators << '\n';
  OS << "  # total operands: " << NumTotalOperands << '\n';

  OS << "  Vocabulary: " << Vocabulary << '\n';
  OS << "  Program length: " << ProgramLength << '\n';
  OS << "  Estimated program length: "
     << (NumDistinctOperators * Log2(NumDistinctOperators) +
         NumDistinctOperands * Log2(NumDistinctOperands))
     << '\n';
  OS << "  Volume: " << Volume << '\n';
  OS << "  Difficulty: " << Difficulty << '\n';
  OS << "  Effort: " << Effort << '\n';
}

bool HalsteadComplexity::runOnFunction(Function &F) {
  for (auto I = inst_begin(F); I != inst_end(F); ++I) {
    if (isa<DbgInfoIntrinsic>(&*I)) {
      continue;
    }

    this->Operators.push_back(I->getOpcode());
    for (const auto &Op : I->operands()) {
      this->Operands.push_back(&*Op);
    }
  }

  this->DistinctOperators.insert(Operators.begin(), Operators.end());
  this->DistinctOperands.insert(Operands.begin(), Operands.end());
  this->F = &F;

  return false;
}

static RegisterPass<HalsteadComplexity>
    X("halstead-complexity", "Calculate the Halstead complexity", false, false);

static void registerHalsteadComplexity(const PassManagerBuilder &,
                                       legacy::PassManagerBase &PM) {
  PM.add(new HalsteadComplexity());
}

static RegisterStandardPasses
    RegisterHalsteadComplexity(PassManagerBuilder::EP_OptimizerLast,
                               registerHalsteadComplexity);

static RegisterStandardPasses
    RegisterHalsteadComplexity0(PassManagerBuilder::EP_EnabledOnOptLevel0,
                                registerHalsteadComplexity);
