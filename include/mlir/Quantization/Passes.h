//===- Passes.h - Quantization Passes ------ --------------------*- C++ -*-===//
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
// This file defines all of the passes owned by the quantization dialect. As
// things mature, it is expected that passes specific to certain frontend or
// backend dialects will move to those dialects directly. For now, they are
// incubated here.
//
//===----------------------------------------------------------------------===//

#ifndef MLIR_QUANTIZATION_PASSES_H
#define MLIR_QUANTIZATION_PASSES_H

namespace mlir {
class FunctionPassBase;

namespace quant {

/// Creates a pass that lowers quantization related TensorFlow ops into
/// the quantization dialect so that express and implied constraints expressed
/// at the TensorFlow source level can be represented to the quantization
/// system. This will specially handle any TensorFlow op that is useful for
/// guiding quantization.
///
/// Note that if your intent is to compile a TensorFlow graph for floating
/// point inference, you should probably not use this pass.
FunctionPassBase *createLowerTFPass();

/// Creates a pass that converts constants followed by a qbarrier to a
/// constant whose value is quantized. This is typically one of the last
/// passes done when lowering to express actual quantized arithmetic in a
/// low level representation. Because it modifies the constant, it is
/// destructive and cannot be undone.
FunctionPassBase *createConvertConstPass();

/// Creates a pass that lowers uniform-quantized real math ops to integer
/// arithmetic. This will leave unrecognized real math ops as-is and is
/// typically followed by a pass that lowers any unrecognized ops to a pure
/// floating point form.
FunctionPassBase *createLowerUniformRealMathPass();

} // namespace quant
} // namespace mlir

#endif // MLIR_QUANTIZATION_PASSES_H