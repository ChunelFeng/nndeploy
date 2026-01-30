/*
 *   Copyright (c) 2025
 *   All rights reserved.
 */
#include "nndeploy/op/op_pow.h"

#include <cmath>

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

base::Status OpPow::inferShape() {
  base::Status status = base::kStatusCodeOk;
  
  // Pow支持广播，输出形状为两个输入形状的广播结果
  auto input_shape = inputs_[0]->getShape();
  auto exponent_shape = inputs_[1]->getShape();
  
  // 计算广播后的输出形状
  base::IntVector output_shape;
  int max_dims = std::max(input_shape.size(), exponent_shape.size());
  
  // 从右向左对齐并广播
  for (int i = 0; i < max_dims; ++i) {
    int input_dim = (i < static_cast<int>(input_shape.size())) 
                    ? input_shape[input_shape.size() - 1 - i] : 1;
    int exponent_dim = (i < static_cast<int>(exponent_shape.size())) 
                       ? exponent_shape[exponent_shape.size() - 1 - i] : 1;
    
    if (input_dim == exponent_dim || input_dim == 1 || exponent_dim == 1) {
      output_shape.insert(output_shape.begin(), std::max(input_dim, exponent_dim));
    } else {
      NNDEPLOY_LOGE("Incompatible shapes for broadcasting");
      return base::kStatusCodeErrorInvalidParam;
    }
  }
  
  outputs_[0]->reshape(output_shape);
  return status;
}

base::Status OpPow::run() {
  base::Status status = base::kStatusCodeOk;

  device::Tensor* input_tensor = inputs_[0];
  device::Tensor* exponent_tensor = inputs_[1];
  device::Tensor* output_tensor = outputs_[0];

  auto input_shape = input_tensor->getShape();
  auto exponent_shape = exponent_tensor->getShape();
  auto output_shape = output_tensor->getShape();

  long output_elements = std::accumulate(output_shape.begin(), output_shape.end(),
                                         1L, std::multiplies<long>());

  float* input_data = static_cast<float*>(input_tensor->getData());
  float* exponent_data = static_cast<float*>(exponent_tensor->getData());
  float* output_data = static_cast<float*>(output_tensor->getData());

  // 简化处理：假设exponent为标量或与input相同形状
  long input_elements = std::accumulate(input_shape.begin(), input_shape.end(),
                                        1L, std::multiplies<long>());
  long exponent_elements = std::accumulate(exponent_shape.begin(), exponent_shape.end(),
                                           1L, std::multiplies<long>());

  if (exponent_elements == 1) {
    // 标量指数
    float exp_val = exponent_data[0];
    for (long i = 0; i < input_elements; ++i) {
      output_data[i] = std::pow(input_data[i], exp_val);
    }
  } else if (input_elements == exponent_elements) {
    // 相同形状
    for (long i = 0; i < input_elements; ++i) {
      output_data[i] = std::pow(input_data[i], exponent_data[i]);
    }
  } else {
    // 需要广播 - 简化实现，逐元素处理
    // TODO: 完整的广播实现
    for (long i = 0; i < output_elements; ++i) {
      long input_idx = i % input_elements;
      long exp_idx = i % exponent_elements;
      output_data[i] = std::pow(input_data[input_idx], exponent_data[exp_idx]);
    }
  }

  return status;
}

base::Status pow(device::Tensor* input, device::Tensor* exponent,
                 device::Tensor* output) {
  base::Status status = base::kStatusCodeOk;

  Op* op = createOp(input->getDeviceType(), "", ir::kOpTypePow);
  if (op == nullptr) {
    NNDEPLOY_LOGE("createOp failed");
    return base::kStatusCodeErrorNotImplement;
  }
  status = op->setInput(input, 0);
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "setInput failed");
  status = op->setInput(exponent, 1);
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

REGISTER_OP_IMPLEMENTION(kDeviceTypeCodeCpu, ir::kOpTypePow, OpPow)

}  // namespace op
}  // namespace nndeploy

