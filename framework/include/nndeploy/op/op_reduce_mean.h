/*
 *   Copyright (c) 2025
 *   All rights reserved.
 */
#ifndef _NNDEPLOY_OP_OP_REDUCE_MEAN_H_
#define _NNDEPLOY_OP_OP_REDUCE_MEAN_H_

#include "nndeploy/ir/ir.h"
#include "nndeploy/op/op.h"

namespace nndeploy {
namespace op {

class OpReduceMean : public Op {
 public:
  OpReduceMean() : Op() {}
  virtual ~OpReduceMean() {}

  virtual base::Status inferShape();
  virtual base::Status run();
};

NNDEPLOY_CC_API base::Status reduceMean(device::Tensor* input,
                                        device::Tensor* axes,
                                        std::shared_ptr<ir::ReduceMeanParam> param,
                                        device::Tensor* output);

}  // namespace op
}  // namespace nndeploy

#endif  // _NNDEPLOY_OP_OP_REDUCE_MEAN_H_

