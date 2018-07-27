//===- Unroll.cpp - Code to perform loop unrolling    ---------------------===//
//
// Copyright 2019 The MLIR Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// =============================================================================
//
// This file implements loop unrolling.
//
//===----------------------------------------------------------------------===//

#include "mlir/IR/Builders.h"
#include "mlir/IR/CFGFunction.h"
#include "mlir/IR/MLFunction.h"
#include "mlir/IR/Module.h"
#include "mlir/IR/OperationSet.h"
#include "mlir/IR/Statements.h"
#include "mlir/IR/StmtVisitor.h"
#include "mlir/Pass.h"
#include "mlir/Transforms/Loop.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

namespace {
struct LoopUnroll : public MLFunctionPass {
  bool runOnMLFunction(MLFunction *f);
  bool runOnForStmt(ForStmt *forStmt);
  bool runLoopUnroll(MLFunction *f);
};
} // namespace

MLFunctionPass *mlir::createLoopUnrollPass() { return new LoopUnroll(); }

/// Unrolls all the innermost loops of this Module.
bool MLFunctionPass::runOnModule(Module *m) {
  bool changed = false;
  for (auto &fn : *m) {
    if (auto *mlFunc = dyn_cast<MLFunction>(&fn))
      changed |= runOnMLFunction(mlFunc);
  }
  return changed;
}

/// Unrolls all the innermost loops of this MLFunction.
bool LoopUnroll::runOnMLFunction(MLFunction *f) {
  // Gathers all innermost loops through a post order pruned walk.
  // TODO: figure out the right reusable template here to better refactor code.
  class InnermostLoopGatherer : public StmtWalker<InnermostLoopGatherer, bool> {
  public:
    // Store innermost loops as we walk.
    std::vector<ForStmt *> loops;

    // This method specialized to encode custom return logic.
    typedef llvm::iplist<Statement> StmtListType;
    bool walk(StmtListType::iterator Start, StmtListType::iterator End) {
      while (Start != End)
        if (walk(&(*Start++)))
          return true;
      return false;
    }

    // FIXME: can't use base class method for this because that in turn would
    // need to use the derived class method above. CRTP doesn't allow it, and
    // the compiler error resulting from it is also very misleading!
    void walkMLFunction(MLFunction *f) { walk(f->begin(), f->end()); }

    bool walkForStmt(ForStmt *forStmt) {
      bool hasInnerLoops = walk(forStmt->begin(), forStmt->end());
      if (!hasInnerLoops)
        loops.push_back(forStmt);
      return true;
    }

    bool walkIfStmt(IfStmt *ifStmt) {
      if (walk(ifStmt->getThenClause()->begin(),
               ifStmt->getThenClause()->end()) ||
          walk(ifStmt->getElseClause()->begin(),
               ifStmt->getElseClause()->end()))
        return true;
      return false;
    }

    bool walkOpStmt(OperationStmt *opStmt) { return false; }

    using StmtWalker<InnermostLoopGatherer, bool>::walk;
  };

  InnermostLoopGatherer ilg;
  ilg.walkMLFunction(f);
  auto &loops = ilg.loops;
  bool changed = false;
  for (auto *forStmt : loops)
    changed |= runOnForStmt(forStmt);
  return changed;
}

/// Unrolls this loop completely. Returns true if the unrolling happens.
bool LoopUnroll::runOnForStmt(ForStmt *forStmt) {
  auto lb = forStmt->getLowerBound()->getValue();
  auto ub = forStmt->getUpperBound()->getValue();
  auto step = forStmt->getStep()->getValue();
  auto trip_count = (ub - lb + 1) / step;

  auto *block = forStmt->getBlock();

  MLFuncBuilder builder(forStmt->Statement::getFunction());
  builder.setInsertionPoint(block);

  for (int i = 0; i < trip_count; i++) {
    for (auto &stmt : forStmt->getStatements()) {
      switch (stmt.getKind()) {
      case Statement::Kind::For:
        llvm_unreachable("unrolling loops that have only operations");
        break;
      case Statement::Kind::If:
        llvm_unreachable("unrolling loops that have only operations");
        break;
      case Statement::Kind::Operation:
        auto *op = cast<OperationStmt>(&stmt);
        // TODO: clone operands and result types.
        builder.createOperation(op->getName(), /*operands*/ {},
                                /*resultTypes*/ {}, op->getAttrs());
        // TODO: loop iterator parsing not yet implemented; replace loop
        // iterator uses in unrolled body appropriately.
        break;
      }
    }
  }

  forStmt->eraseFromBlock();
  return true;
}