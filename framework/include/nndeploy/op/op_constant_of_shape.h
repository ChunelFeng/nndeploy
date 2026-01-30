/*
 *   Copyright (c) 2025
 *   All rights reserved.
 */
#ifndef _NNDEPLOY_OP_OP_CONSTANT_OF_SHAPE_H_
#define _NNDEPLOY_OP_OP_CONSTANT_OF_SHAPE_H_

#include "nndeploy/ir/ir.h"
#include "nndeploy/op/op.h"

namespace nndeploy {
namespace op {

class OpConstantOfShape : public Op {
 public:
  OpConstantOfShape() : Op() {}
  virtual ~OpConstantOfShape() {}

  virtual base::Status inferShape();
  virtual base::Status inferDataType();
  virtual base::Status run();
};

NNDEPLOY_CC_API base::Status constantOfShape(
    device::Tensor* shape,
    std::shared_ptr<ir::ConstantOfShapeParam> param,
    device::Tensor* output);

}  // namespace op
}  // namespace nndeploy

#endif  // _NNDEPLOY_OP_OP_CONSTANT_OF_SHAPE_H_

