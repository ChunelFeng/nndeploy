/*
 *   Copyright (c) 2025
 *   All rights reserved.
 */
#ifndef _NNDEPLOY_OP_OP_NEG_H_
#define _NNDEPLOY_OP_OP_NEG_H_

#include "nndeploy/ir/ir.h"
#include "nndeploy/op/op.h"
#include "nndeploy/op/op_unary.h"

namespace nndeploy {
namespace op {

class OpNeg : public OpUnary {
 public:
  OpNeg() : OpUnary() { is_inplace_ = false; }
  virtual ~OpNeg() {}

  virtual base::Status run();
};

NNDEPLOY_CC_API base::Status neg(device::Tensor* input, device::Tensor* output);

}  // namespace op
}  // namespace nndeploy

#endif  // _NNDEPLOY_OP_OP_NEG_H_

