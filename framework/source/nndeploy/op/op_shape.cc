/*
 *   Copyright (c) 2025
 *   All rights reserved.
 */
#include "nndeploy/op/op_shape.h"

#include "nndeploy/base/any.h"
#include "nndeploy/base/common.h"
#include "nndeploy/base/glic_stl_include.h"
#include "nndeploy/base/log.h"
#include "nndeploy/base/macro.h"
#include "nndeploy/base/object.h"
#include "nndeploy/base/param.h"
#include "nndeploy/base/status.h"
#include "nndeploy/base/string.h"
#include "nndeploy/base/time_profiler.h"
#include "nndeploy/device/buffer.h"
#include "nndeploy/device/device.h"
#include "nndeploy/device/memory_pool.h"
#include "nndeploy/device/tensor.h"
#include "nndeploy/ir/ir.h"
#include "nndeploy/op/op.h"

namespace nndeploy {
namespace op {

base::Status OpShape::inferShape() {
  base::Status status = base::kStatusCodeOk;

  auto param = dynamic_cast<ir::ShapeParam*>(op_desc_.op_param_.get());
  int start = param ? param->start_ : 0;
  int end = param ? param->end_ : -1;

  auto input_shape = inputs_[0]->getShape();
  int input_rank = static_cast<int>(input_shape.size());

  // 处理负数索引
  if (start < 0) {
    start += input_rank;
  }
  if (end < 0) {
    end += input_rank + 1;
  }

  // 边界检查
  start = std::max(0, std::min(start, input_rank));
  end = std::max(start, std::min(end, input_rank));

  // 输出形状为一维，长度为所选维度的数量
  int output_size = end - start;
  base::IntVector output_shape = {output_size};

  outputs_[0]->reshape(output_shape);
  return status;
}

base::Status OpShape::inferDataType() {
  base::Status status = base::kStatusCodeOk;
  // Shape算子输出int64类型
  outputs_[0]->setDataType(base::dataTypeOf<int64_t>());
  return status;
}

base::Status OpShape::run() {
  base::Status status = base::kStatusCodeOk;

  auto param = dynamic_cast<ir::ShapeParam*>(op_desc_.op_param_.get());
  int start = param ? param->start_ : 0;
  int end = param ? param->end_ : -1;

  device::Tensor* input_tensor = inputs_[0];
  device::Tensor* output_tensor = outputs_[0];

  auto input_shape = input_tensor->getShape();
  int input_rank = static_cast<int>(input_shape.size());

  // 处理负数索引
  if (start < 0) {
    start += input_rank;
  }
  if (end < 0) {
    end += input_rank + 1;
  }

  // 边界检查
  start = std::max(0, std::min(start, input_rank));
  end = std::max(start, std::min(end, input_rank));

  int64_t* output_data = static_cast<int64_t*>(output_tensor->getData());

  for (int i = start; i < end; ++i) {
    output_data[i - start] = static_cast<int64_t>(input_shape[i]);
  }

  return status;
}

base::Status shape(device::Tensor* input,
                   std::shared_ptr<ir::ShapeParam> param,
                   device::Tensor* output) {
  base::Status status = base::kStatusCodeOk;

  Op* op = createOp(input->getDeviceType(), "", ir::kOpTypeShape);
  if (op == nullptr) {
    NNDEPLOY_LOGE("createOp failed");
    return base::kStatusCodeErrorNotImplement;
  }
  if (param != nullptr) {
    status = op->setParam(param);
    NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "setParam failed");
  }
  status = op->setInput(input, 0);
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "setInput failed");
  status = op->setOutput(output, 0);
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "setOutput failed");
  status = op->init();
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "init failed");
  status = op->checkOrAllocOutput();
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk,
                         "checkOrAllocOutput failed");
  status = op->preRun();
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "preRun failed");
  status = op->run();
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "run failed");
  status = op->postRun();
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "postRun failed");
  status = op->deinit();
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "deinit failed");
  delete op;

  return status;
}

REGISTER_OP_IMPLEMENTION(kDeviceTypeCodeCpu, ir::kOpTypeShape, OpShape)

}  // namespace op
}  // namespace nndeploy

