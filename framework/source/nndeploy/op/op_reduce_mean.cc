/*
 *   Copyright (c) 2025
 *   All rights reserved.
 */
#include "nndeploy/op/op_reduce_mean.h"

#include <algorithm>
#include <numeric>
#include <set>

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

base::Status OpReduceMean::inferShape() {
  base::Status status = base::kStatusCodeOk;

  auto param = dynamic_cast<ir::ReduceMeanParam*>(op_desc_.op_param_.get());
  int keepdims = param ? param->keepdims_ : 1;

  auto input_shape = inputs_[0]->getShape();
  int input_rank = static_cast<int>(input_shape.size());

  // 获取axes - 从第二个输入获取
  std::set<int> axes_set;
  if (inputs_.size() > 1 && inputs_[1] != nullptr) {
    int64_t* axes_data = static_cast<int64_t*>(inputs_[1]->getData());
    int axes_size = inputs_[1]->getShapeIndex(0);
    for (int i = 0; i < axes_size; ++i) {
      int axis = static_cast<int>(axes_data[i]);
      if (axis < 0) {
        axis += input_rank;
      }
      axes_set.insert(axis);
    }
  } else {
    // 如果没有axes输入，对所有维度进行reduce
    for (int i = 0; i < input_rank; ++i) {
      axes_set.insert(i);
    }
  }

  // 计算输出形状
  base::IntVector output_shape;
  for (int i = 0; i < input_rank; ++i) {
    if (axes_set.find(i) != axes_set.end()) {
      if (keepdims) {
        output_shape.push_back(1);
      }
    } else {
      output_shape.push_back(input_shape[i]);
    }
  }

  if (output_shape.empty()) {
    output_shape.push_back(1);
  }

  outputs_[0]->reshape(output_shape);
  return status;
}

base::Status OpReduceMean::run() {
  base::Status status = base::kStatusCodeOk;

  auto param = dynamic_cast<ir::ReduceMeanParam*>(op_desc_.op_param_.get());
  int keepdims = param ? param->keepdims_ : 1;

  device::Tensor* input_tensor = inputs_[0];
  device::Tensor* output_tensor = outputs_[0];

  auto input_shape = input_tensor->getShape();
  int input_rank = static_cast<int>(input_shape.size());

  // 获取axes
  std::set<int> axes_set;
  if (inputs_.size() > 1 && inputs_[1] != nullptr) {
    int64_t* axes_data = static_cast<int64_t*>(inputs_[1]->getData());
    int axes_size = inputs_[1]->getShapeIndex(0);
    for (int i = 0; i < axes_size; ++i) {
      int axis = static_cast<int>(axes_data[i]);
      if (axis < 0) {
        axis += input_rank;
      }
      axes_set.insert(axis);
    }
  } else {
    for (int i = 0; i < input_rank; ++i) {
      axes_set.insert(i);
    }
  }

  float* input_data = static_cast<float*>(input_tensor->getData());
  float* output_data = static_cast<float*>(output_tensor->getData());

  auto output_shape = output_tensor->getShape();
  long output_elements = std::accumulate(output_shape.begin(), output_shape.end(),
                                         1L, std::multiplies<long>());

  // 计算strides
  std::vector<long> input_strides(input_rank);
  input_strides[input_rank - 1] = 1;
  for (int i = input_rank - 2; i >= 0; --i) {
    input_strides[i] = input_strides[i + 1] * input_shape[i + 1];
  }

  // 计算reduce的元素数量
  long reduce_size = 1;
  for (int axis : axes_set) {
    reduce_size *= input_shape[axis];
  }

  // 初始化输出
  for (long i = 0; i < output_elements; ++i) {
    output_data[i] = 0.0f;
  }

  // 计算总元素数
  long input_elements = std::accumulate(input_shape.begin(), input_shape.end(),
                                        1L, std::multiplies<long>());

  // 遍历所有输入元素
  for (long i = 0; i < input_elements; ++i) {
    // 计算输入索引对应的多维索引
    std::vector<int> multi_idx(input_rank);
    long temp = i;
    for (int d = input_rank - 1; d >= 0; --d) {
      multi_idx[d] = temp % input_shape[d];
      temp /= input_shape[d];
    }

    // 计算输出索引
    long output_idx = 0;
    int out_d = 0;
    std::vector<long> output_strides(output_shape.size());
    if (!output_shape.empty()) {
      output_strides[output_shape.size() - 1] = 1;
      for (int d = static_cast<int>(output_shape.size()) - 2; d >= 0; --d) {
        output_strides[d] = output_strides[d + 1] * output_shape[d + 1];
      }
    }

    out_d = 0;
    for (int d = 0; d < input_rank; ++d) {
      if (axes_set.find(d) != axes_set.end()) {
        if (keepdims) {
          out_d++;
        }
      } else {
        if (out_d < static_cast<int>(output_shape.size())) {
          output_idx += multi_idx[d] * output_strides[out_d];
        }
        out_d++;
      }
    }

    output_data[output_idx] += input_data[i];
  }

  // 除以reduce的元素数量得到平均值
  for (long i = 0; i < output_elements; ++i) {
    output_data[i] /= static_cast<float>(reduce_size);
  }

  return status;
}

base::Status reduceMean(device::Tensor* input, device::Tensor* axes,
                        std::shared_ptr<ir::ReduceMeanParam> param,
                        device::Tensor* output) {
  base::Status status = base::kStatusCodeOk;

  Op* op = createOp(input->getDeviceType(), "", ir::kOpTypeReduceMean);
  if (op == nullptr) {
    NNDEPLOY_LOGE("createOp failed");
    return base::kStatusCodeErrorNotImplement;
  }
  status = op->setParam(param);
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "setParam failed");
  status = op->setInput(input, 0);
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "setInput failed");
  if (axes != nullptr) {
    status = op->setInput(axes, 1);
    NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "setInput failed");
  }
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

REGISTER_OP_IMPLEMENTION(kDeviceTypeCodeCpu, ir::kOpTypeReduceMean, OpReduceMean)

}  // namespace op
}  // namespace nndeploy

