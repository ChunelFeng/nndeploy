#include "nndeploy/op/op_swiglu.h"

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

base::Status OpSwiGLU::inferShape() {
  if (inputs_.size() == 1) {
    auto input_shape = inputs_[0]->getShape();
    if (input_shape.empty() || input_shape.back() % 2 != 0) {
      NNDEPLOY_LOGE("SwiGLU single input requires the last dimension to be even");
      return base::kStatusCodeErrorInvalidParam;
    }
    auto output_shape = input_shape;
    output_shape.back() /= 2;
    outputs_[0]->reshape(output_shape);
  } else if (inputs_.size() == 2) {
    auto input_shape1 = inputs_[0]->getShape();
    auto input_shape2 = inputs_[1]->getShape();
    if (input_shape1 != input_shape2) {
      NNDEPLOY_LOGE("SwiGLU two inputs require the same shape");
      return base::kStatusCodeErrorInvalidParam;
    }
    outputs_[0]->reshape(input_shape1);
  } else {
    NNDEPLOY_LOGE("SwiGLU requires 1 or 2 inputs");
    return base::kStatusCodeErrorInvalidParam;
  }
  return base::kStatusCodeOk;
}

base::Status OpSwiGLU::run() {
  base::Status status = base::kStatusCodeOk;

  if (inputs_.size() == 1) {
    device::Tensor *input_tensor = inputs_[0];
    device::Tensor *output_tensor = outputs_[0];

    auto input_shape = input_tensor->getShape();
    long outer_elements = 1;
    for (size_t i = 0; i < input_shape.size() - 1; ++i) {
      outer_elements *= input_shape[i];
    }
    long dim = input_shape.back() / 2;

    float *input_data = static_cast<float *>(input_tensor->getData());
    float *output_data = static_cast<float *>(output_tensor->getData());

    for (long i = 0; i < outer_elements; ++i) {
      float *x = input_data + i * (dim * 2);
      float *y = x + dim;
      float *out = output_data + i * dim;
      for (long j = 0; j < dim; ++j) {
        out[j] = (x[j] / (1.0f + expf(-x[j]))) * y[j];
      }
    }
  } else if (inputs_.size() == 2) {
    device::Tensor *input_tensor1 = inputs_[0];
    device::Tensor *input_tensor2 = inputs_[1];
    device::Tensor *output_tensor = outputs_[0];

    auto input_shape = input_tensor1->getShape();
    long input_elements = std::accumulate(input_shape.begin(), input_shape.end(),
                                          1, std::multiplies<int>());

    float *input_data1 = static_cast<float *>(input_tensor1->getData());
    float *input_data2 = static_cast<float *>(input_tensor2->getData());
    float *output_data = static_cast<float *>(output_tensor->getData());

    for (long i = 0; i < input_elements; ++i) {
      output_data[i] = (input_data1[i] / (1.0f + expf(-input_data1[i]))) * input_data2[i];
    }
  } else {
    return base::kStatusCodeErrorInvalidParam;
  }

  return status;
}

base::Status swiglu(device::Tensor *input, device::Tensor *output) {
  base::Status status = base::kStatusCodeOk;

  Op *op = createOp(input->getDeviceType(), "", ir::kOpTypeSwiGLU);
  if (op == nullptr) {
    NNDEPLOY_LOGE("createOp failed");
    return base::kStatusCodeErrorNotImplement;
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

base::Status swiglu(device::Tensor *input1, device::Tensor *input2,
                    device::Tensor *output) {
  base::Status status = base::kStatusCodeOk;

  Op *op = createOp(input1->getDeviceType(), "", ir::kOpTypeSwiGLU);
  if (op == nullptr) {
    NNDEPLOY_LOGE("createOp failed");
    return base::kStatusCodeErrorNotImplement;
  }
  status = op->setInput(input1, 0);
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "setInput failed");
  status = op->setInput(input2, 1);
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

REGISTER_OP_IMPLEMENTION(kDeviceTypeCodeCpu, ir::kOpTypeSwiGLU, OpSwiGLU)

}  // namespace op
}  // namespace nndeploy