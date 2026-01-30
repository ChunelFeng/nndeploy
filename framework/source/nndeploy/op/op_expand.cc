/*
 *   Copyright (c) 2025
 *   All rights reserved.
 */
#include "nndeploy/op/op_expand.h"

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

base::Status OpExpand::inferShape() {
  base::Status status = base::kStatusCodeOk;

  // 获取输入形状
  auto input_shape = inputs_[0]->getShape();
  
  // 从第二个输入获取目标形状
  device::Tensor* shape_tensor = inputs_[1];
  int64_t* shape_data = static_cast<int64_t*>(shape_tensor->getData());
  int shape_size = shape_tensor->getShapeIndex(0);

  base::IntVector target_shape;
  for (int i = 0; i < shape_size; ++i) {
    target_shape.push_back(static_cast<int>(shape_data[i]));
  }

  // 计算广播后的输出形状
  base::IntVector output_shape;
  int max_dims = std::max(input_shape.size(), target_shape.size());

  // 从右向左对齐并广播
  for (int i = 0; i < max_dims; ++i) {
    int input_dim = (i < static_cast<int>(input_shape.size()))
                        ? input_shape[input_shape.size() - 1 - i] : 1;
    int target_dim = (i < static_cast<int>(target_shape.size()))
                         ? target_shape[target_shape.size() - 1 - i] : 1;

    if (input_dim == target_dim || input_dim == 1 || target_dim == 1) {
      output_shape.insert(output_shape.begin(), std::max(input_dim, target_dim));
    } else {
      NNDEPLOY_LOGE("Incompatible shapes for expand");
      return base::kStatusCodeErrorInvalidParam;
    }
  }

  outputs_[0]->reshape(output_shape);
  return status;
}

base::Status OpExpand::run() {
  base::Status status = base::kStatusCodeOk;

  device::Tensor* input_tensor = inputs_[0];
  device::Tensor* output_tensor = outputs_[0];

  auto input_shape = input_tensor->getShape();
  auto output_shape = output_tensor->getShape();

  float* input_data = static_cast<float*>(input_tensor->getData());
  float* output_data = static_cast<float*>(output_tensor->getData());

  int input_rank = static_cast<int>(input_shape.size());
  int output_rank = static_cast<int>(output_shape.size());

  // 计算strides
  std::vector<long> input_strides(input_rank);
  std::vector<long> output_strides(output_rank);

  if (input_rank > 0) {
    input_strides[input_rank - 1] = 1;
    for (int i = input_rank - 2; i >= 0; --i) {
      input_strides[i] = input_strides[i + 1] * input_shape[i + 1];
    }
  }

  if (output_rank > 0) {
    output_strides[output_rank - 1] = 1;
    for (int i = output_rank - 2; i >= 0; --i) {
      output_strides[i] = output_strides[i + 1] * output_shape[i + 1];
    }
  }

  long output_elements = std::accumulate(output_shape.begin(), output_shape.end(),
                                         1L, std::multiplies<long>());

  // 对齐输入形状
  base::IntVector aligned_input_shape(output_rank, 1);
  for (int i = 0; i < input_rank; ++i) {
    aligned_input_shape[output_rank - input_rank + i] = input_shape[i];
  }

  std::vector<long> aligned_input_strides(output_rank, 0);
  for (int i = 0; i < input_rank; ++i) {
    aligned_input_strides[output_rank - input_rank + i] = input_strides[i];
  }

  // 遍历所有输出元素
  for (long out_idx = 0; out_idx < output_elements; ++out_idx) {
    // 计算输出多维索引
    std::vector<int> out_multi_idx(output_rank);
    long temp = out_idx;
    for (int d = output_rank - 1; d >= 0; --d) {
      out_multi_idx[d] = temp % output_shape[d];
      temp /= output_shape[d];
    }

    // 计算对应的输入索引
    long in_idx = 0;
    for (int d = 0; d < output_rank; ++d) {
      int in_dim_idx = out_multi_idx[d];
      if (aligned_input_shape[d] == 1) {
        in_dim_idx = 0;  // 广播
      }
      in_idx += in_dim_idx * aligned_input_strides[d];
    }

    output_data[out_idx] = input_data[in_idx];
  }

  return status;
}

base::Status expand(device::Tensor* input, device::Tensor* shape,
                    device::Tensor* output) {
  base::Status status = base::kStatusCodeOk;

  Op* op = createOp(input->getDeviceType(), "", ir::kOpTypeExpand);
  if (op == nullptr) {
    NNDEPLOY_LOGE("createOp failed");
    return base::kStatusCodeErrorNotImplement;
  }
  status = op->setInput(input, 0);
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "setInput failed");
  status = op->setInput(shape, 1);
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

REGISTER_OP_IMPLEMENTION(kDeviceTypeCodeCpu, ir::kOpTypeExpand, OpExpand)

}  // namespace op
}  // namespace nndeploy

