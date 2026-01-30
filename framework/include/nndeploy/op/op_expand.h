/*
 *   Copyright (c) 2025
 *   All rights reserved.
 */
#ifndef _NNDEPLOY_OP_OP_EXPAND_H_
#define _NNDEPLOY_OP_OP_EXPAND_H_

#include "nndeploy/ir/ir.h"
#include "nndeploy/op/op.h"

namespace nndeploy {
namespace op {

class OpExpand : public Op {
 public:
  OpExpand() : Op() {}
  virtual ~OpExpand() {}

  virtual base::Status inferShape();
  virtual base::Status run();
};

NNDEPLOY_CC_API base::Status expand(device::Tensor* input,
                                    device::Tensor* shape,
                                    device::Tensor* output);

}  // namespace op
}  // namespace nndeploy

#endif  // _NNDEPLOY_OP_OP_EXPAND_H_

