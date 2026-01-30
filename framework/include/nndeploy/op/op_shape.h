/*
 *   Copyright (c) 2025
 *   All rights reserved.
 */
#ifndef _NNDEPLOY_OP_OP_SHAPE_H_
#define _NNDEPLOY_OP_OP_SHAPE_H_

#include "nndeploy/ir/ir.h"
#include "nndeploy/op/op.h"

namespace nndeploy {
namespace op {

class OpShape : public Op {
 public:
  OpShape() : Op() {}
  virtual ~OpShape() {}

  virtual base::Status inferShape();
  virtual base::Status inferDataType();
  virtual base::Status run();
};

NNDEPLOY_CC_API base::Status shape(device::Tensor* input,
                                   std::shared_ptr<ir::ShapeParam> param,
                                   device::Tensor* output);

}  // namespace op
}  // namespace nndeploy

#endif  // _NNDEPLOY_OP_OP_SHAPE_H_

