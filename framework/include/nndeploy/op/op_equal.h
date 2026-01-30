/*
 *   Copyright (c) 2025
 *   All rights reserved.
 */
#ifndef _NNDEPLOY_OP_OP_EQUAL_H_
#define _NNDEPLOY_OP_OP_EQUAL_H_

#include "nndeploy/ir/ir.h"
#include "nndeploy/op/op.h"

namespace nndeploy {
namespace op {

class OpEqual : public Op {
 public:
  OpEqual() : Op() {}
  virtual ~OpEqual() {}

  virtual base::Status inferShape();
  virtual base::Status inferDataType();
  virtual base::Status run();
};

NNDEPLOY_CC_API base::Status equal(device::Tensor* input_a,
                                   device::Tensor* input_b,
                                   device::Tensor* output);

}  // namespace op
}  // namespace nndeploy

#endif  // _NNDEPLOY_OP_OP_EQUAL_H_

