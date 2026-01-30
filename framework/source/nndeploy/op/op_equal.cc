/*
 *   Copyright (c) 2025
 *   All rights reserved.
 */
#include "nndeploy/op/op_equal.h"

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

base::Status OpEqual::inferShape() {
  base::Status status = base::kStatusCodeOk;

  // Equal支持广播，输出形状为两个输入形状的广播结果
  auto shape_a = inputs_[0]->getShape();
  auto shape_b = inputs_[1]->getShape();

  // 计算广播后的输出形状
  base::IntVector output_shape;
  int max_dims = std::max(shape_a.size(), shape_b.size());

  // 从右向左对齐并广播
  for (int i = 0; i < max_dims; ++i) {
    int dim_a = (i < static_cast<int>(shape_a.size()))
                    ? shape_a[shape_a.size() - 1 - i] : 1;
    int dim_b = (i < static_cast<int>(shape_b.size()))
                    ? shape_b[shape_b.size() - 1 - i] : 1;

    if (dim_a == dim_b || dim_a == 1 || dim_b == 1) {
      output_shape.insert(output_shape.begin(), std::max(dim_a, dim_b));
    } else {
      NNDEPLOY_LOGE("Incompatible shapes for broadcasting");
      return base::kStatusCodeErrorInvalidParam;
    }
  }

  outputs_[0]->reshape(output_shape);
  return status;
}

base::Status OpEqual::inferDataType() {
  base::Status status = base::kStatusCodeOk;
  // Equal算子输出bool类型
  outputs_[0]->setDataType(base::dataTypeOf<bool>());
  return status;
}

base::Status OpEqual::run() {
  base::Status status = base::kStatusCodeOk;

  device::Tensor* input_a = inputs_[0];
  device::Tensor* input_b = inputs_[1];
  device::Tensor* output_tensor = outputs_[0];

  auto shape_a = input_a->getShape();
  auto shape_b = input_b->getShape();
  auto output_shape = output_tensor->getShape();

  long output_elements = std::accumulate(output_shape.begin(), output_shape.end(),
                                         1L, std::multiplies<long>());

  float* data_a = static_cast<float*>(input_a->getData());
  float* data_b = static_cast<float*>(input_b->getData());
  bool* output_data = static_cast<bool*>(output_tensor->getData());

  long elements_a = std::accumulate(shape_a.begin(), shape_a.end(),
                                    1L, std::multiplies<long>());
  long elements_b = std::accumulate(shape_b.begin(), shape_b.end(),
                                    1L, std::multiplies<long>());

  // 简化处理：假设两个输入相同形状或其中一个为标量
  if (elements_a == elements_b) {
    for (long i = 0; i < output_elements; ++i) {
      output_data[i] = (data_a[i] == data_b[i]);
    }
  } else if (elements_b == 1) {
    float val_b = data_b[0];
    for (long i = 0; i < output_elements; ++i) {
      output_data[i] = (data_a[i] == val_b);
    }
  } else if (elements_a == 1) {
    float val_a = data_a[0];
    for (long i = 0; i < output_elements; ++i) {
      output_data[i] = (val_a == data_b[i]);
    }
  } else {
    // 需要广播
    for (long i = 0; i < output_elements; ++i) {
      long idx_a = i % elements_a;
      long idx_b = i % elements_b;
      output_data[i] = (data_a[idx_a] == data_b[idx_b]);
    }
  }

  return status;
}

base::Status equal(device::Tensor* input_a, device::Tensor* input_b,
                   device::Tensor* output) {
  base::Status status = base::kStatusCodeOk;

  Op* op = createOp(input_a->getDeviceType(), "", ir::kOpTypeEqual);
  if (op == nullptr) {
    NNDEPLOY_LOGE("createOp failed");
    return base::kStatusCodeErrorNotImplement;
  }
  status = op->setInput(input_a, 0);
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "setInput failed");
  status = op->setInput(input_b, 1);
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

REGISTER_OP_IMPLEMENTION(kDeviceTypeCodeCpu, ir::kOpTypeEqual, OpEqual)

}  // namespace op
}  // namespace nndeploy

