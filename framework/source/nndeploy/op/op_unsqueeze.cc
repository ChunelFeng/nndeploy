/*
 *   Copyright (c) 2025
 *   All rights reserved.
 */
#include "nndeploy/op/op_unsqueeze.h"

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

base::Status OpUnsqueeze::inferShape() {
  base::Status status = base::kStatusCodeOk;

  // 获取输入张量的形状
  auto input_shape = inputs_[0]->getShape();
  int input_rank = static_cast<int>(input_shape.size());

  // 获取axes参数 - 从第二个输入获取
  std::vector<int64_t> axes;
  if (inputs_.size() > 1 && inputs_[1] != nullptr) {
    int64_t* axes_data = static_cast<int64_t*>(inputs_[1]->getData());
    int axes_size = inputs_[1]->getShapeIndex(0);
    for (int i = 0; i < axes_size; ++i) {
      axes.push_back(axes_data[i]);
    }
  }

  // 计算输出形状的rank
  int output_rank = input_rank + static_cast<int>(axes.size());

  // 处理负数索引
  std::vector<int> normalized_axes;
  for (auto axis : axes) {
    int normalized_axis = static_cast<int>(axis);
    if (normalized_axis < 0) {
      normalized_axis += output_rank;
    }
    normalized_axes.push_back(normalized_axis);
  }

  // 排序axes
  std::sort(normalized_axes.begin(), normalized_axes.end());

  // 构建输出形状
  base::IntVector output_shape;
  int input_idx = 0;
  int axes_idx = 0;
  for (int i = 0; i < output_rank; ++i) {
    if (axes_idx < static_cast<int>(normalized_axes.size()) &&
        normalized_axes[axes_idx] == i) {
      output_shape.push_back(1);
      axes_idx++;
    } else {
      output_shape.push_back(input_shape[input_idx++]);
    }
  }

  outputs_[0]->reshape(output_shape);
  return status;
}

base::Status OpUnsqueeze::run() {
  base::Status status = base::kStatusCodeOk;

  device::Tensor* input_tensor = inputs_[0];
  device::Tensor* output_tensor = outputs_[0];

  // Unsqueeze只是改变形状，数据不变，可以直接复制
  size_t data_size = input_tensor->getSize();
  void* input_data = input_tensor->getData();
  void* output_data = output_tensor->getData();

  if (input_data != output_data) {
    memcpy(output_data, input_data, data_size);
  }

  return status;
}

base::Status unsqueeze(device::Tensor* input, device::Tensor* axes,
                       device::Tensor* output) {
  base::Status status = base::kStatusCodeOk;

  Op* op = createOp(input->getDeviceType(), "", ir::kOpTypeUnsqueeze);
  if (op == nullptr) {
    NNDEPLOY_LOGE("createOp failed");
    return base::kStatusCodeErrorNotImplement;
  }
  status = op->setInput(input, 0);
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "setInput failed");
  status = op->setInput(axes, 1);
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

REGISTER_OP_IMPLEMENTION(kDeviceTypeCodeCpu, ir::kOpTypeUnsqueeze, OpUnsqueeze)

}  // namespace op
}  // namespace nndeploy

