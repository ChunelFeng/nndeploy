/*
 *   Copyright (c) 2025
 *   All rights reserved.
 */
#ifndef _NNDEPLOY_OP_OP_UNSQUEEZE_H_
#define _NNDEPLOY_OP_OP_UNSQUEEZE_H_

#include "nndeploy/ir/ir.h"
#include "nndeploy/op/op.h"

namespace nndeploy {
namespace op {

class OpUnsqueeze : public Op {
 public:
  OpUnsqueeze() : Op() {}
  virtual ~OpUnsqueeze() {}

  virtual base::Status inferShape();
  virtual base::Status run();
};

NNDEPLOY_CC_API base::Status unsqueeze(device::Tensor* input,
                                       device::Tensor* axes,
                                       device::Tensor* output);

}  // namespace op
}  // namespace nndeploy

#endif  // _NNDEPLOY_OP_OP_UNSQUEEZE_H_
