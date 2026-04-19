#include "nndeploy/op/op_layernorm.h"

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

namespace {

static int64_t getElementCount(const base::IntVector &shape) {
  return std::accumulate(shape.begin(), shape.end(), static_cast<int64_t>(1),
                         std::multiplies<int64_t>());
}

}  // namespace

base::Status OpLayerNorm::inferShape() {
  if (inputs_.size() < 2 || inputs_.size() > 3) {
    NNDEPLOY_LOGE("LayerNorm requires 2 or 3 inputs, but got %ld.\n",
                  inputs_.size());
    return base::kStatusCodeErrorInvalidParam;
  }

  outputs_[0]->reshape(inputs_[0]->getShape());
  return base::kStatusCodeOk;
}

base::Status OpLayerNorm::run() {
  auto param =
      dynamic_cast<ir::LayerNormalizationParam *>(op_desc_.op_param_.get());
  NNDEPLOY_CHECK_PARAM_NULL_RET_STATUS(param,
                                       "op_desc_.op_param_ is nullptr");

  const auto &input_shape = inputs_[0]->getShape();
  const auto &weight_shape = inputs_[1]->getShape();
  if (input_shape.empty() || weight_shape.empty()) {
    NNDEPLOY_LOGE("LayerNorm input/weight shape must not be empty.\n");
    return base::kStatusCodeErrorInvalidParam;
  }

  if (weight_shape.size() > input_shape.size()) {
    NNDEPLOY_LOGE("LayerNorm weight rank must not exceed input rank.\n");
    return base::kStatusCodeErrorInvalidParam;
  }
  for (size_t i = 0; i < weight_shape.size(); ++i) {
    size_t input_index = input_shape.size() - weight_shape.size() + i;
    if (input_shape[input_index] != weight_shape[i]) {
      NNDEPLOY_LOGE("LayerNorm weight shape must match input trailing dims.\n");
      return base::kStatusCodeErrorInvalidParam;
    }
  }

  const bool has_bias = inputs_.size() == 3;
  if (has_bias && inputs_[2]->getShape() != weight_shape) {
    NNDEPLOY_LOGE("LayerNorm bias shape must equal weight shape.\n");
    return base::kStatusCodeErrorInvalidParam;
  }

  const int64_t input_elements = getElementCount(input_shape);
  const int64_t normalized_elements = getElementCount(weight_shape);
  if (normalized_elements <= 0 || input_elements % normalized_elements != 0) {
    NNDEPLOY_LOGE(
        "LayerNorm normalized element count is invalid for input shape.\n");
    return base::kStatusCodeErrorInvalidParam;
  }
  const int64_t outer_elements = input_elements / normalized_elements;

  float *input_data = static_cast<float *>(inputs_[0]->getData());
  float *weight_data = static_cast<float *>(inputs_[1]->getData());
  float *bias_data = has_bias ? static_cast<float *>(inputs_[2]->getData())
                              : nullptr;
  float *output_data = static_cast<float *>(outputs_[0]->getData());

  const float epsilon = param->epsilon_;
  for (int64_t i = 0; i < outer_elements; ++i) {
    float *input_base = input_data + i * normalized_elements;
    float *output_base = output_data + i * normalized_elements;

    float mean = 0.0f;
    for (int64_t j = 0; j < normalized_elements; ++j) {
      mean += input_base[j];
    }
    mean /= static_cast<float>(normalized_elements);

    float variance = 0.0f;
    for (int64_t j = 0; j < normalized_elements; ++j) {
      float diff = input_base[j] - mean;
      variance += diff * diff;
    }
    variance /= static_cast<float>(normalized_elements);
    float inv_std = 1.0f / std::sqrt(variance + epsilon);

    for (int64_t j = 0; j < normalized_elements; ++j) {
      float normalized = (input_base[j] - mean) * inv_std;
      float bias = has_bias ? bias_data[j] : 0.0f;
      output_base[j] = normalized * weight_data[j] + bias;
    }
  }

  return base::kStatusCodeOk;
}

base::Status layerNorm(device::Tensor *input, device::Tensor *weight,
                       device::Tensor *bias, std::shared_ptr<base::Param> param,
                       device::Tensor *output) {
  base::Status status = base::kStatusCodeOk;

  Op *op = createOp(input->getDeviceType(), "", ir::kOpTypeLayerNormalization);
  if (op == nullptr) {
    NNDEPLOY_LOGE("createOp failed");
    return base::kStatusCodeErrorNotImplement;
  }
  status = op->setParam(param);
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "setParam failed");
  status = op->setInput(input, 0);
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "setInput failed");
  status = op->setInput(weight, 1);
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "setInput failed");
  if (bias != nullptr) {
    status = op->setInput(bias, 2);
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

base::Status layerNorm(device::Tensor *input, device::Tensor *weight,
                       std::shared_ptr<base::Param> param,
                       device::Tensor *output) {
  return layerNorm(input, weight, nullptr, param, output);
}

REGISTER_OP_IMPLEMENTION(kDeviceTypeCodeCpu, ir::kOpTypeLayerNormalization,
                         OpLayerNorm)

}  // namespace op
}  // namespace nndeploy
