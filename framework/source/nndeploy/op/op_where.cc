
#include "nndeploy/op/op_where.h"

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

base::Status OpWhere::run() {
  base::Status status = base::kStatusCodeOk;

  // ONNX Where: condition, X, Y -> output
  // output[i] = X[i] if condition[i] else Y[i]
  device::Tensor* condition_tensor = inputs_[0];
  device::Tensor* x_tensor = inputs_[1];
  device::Tensor* y_tensor = inputs_[2];
  device::Tensor* output_tensor = outputs_[0];

  auto condition_shape = condition_tensor->getShape();
  auto x_shape = x_tensor->getShape();
  auto y_shape = y_tensor->getShape();
  auto output_shape = output_tensor->getShape();

  bool* condition_data = static_cast<bool*>(condition_tensor->getData());
  float* x_data = static_cast<float*>(x_tensor->getData());
  float* y_data = static_cast<float*>(y_tensor->getData());
  float* output_data = static_cast<float*>(output_tensor->getData());

  long condition_elements = std::accumulate(condition_shape.begin(), condition_shape.end(),
                                            1L, std::multiplies<long>());
  long x_elements = std::accumulate(x_shape.begin(), x_shape.end(),
                                    1L, std::multiplies<long>());
  long y_elements = std::accumulate(y_shape.begin(), y_shape.end(),
                                    1L, std::multiplies<long>());
  long output_elements = std::accumulate(output_shape.begin(), output_shape.end(),
                                         1L, std::multiplies<long>());

  // 简化处理：假设所有输入相同形状或支持简单广播
  for (long i = 0; i < output_elements; ++i) {
    long cond_idx = i % condition_elements;
    long x_idx = i % x_elements;
    long y_idx = i % y_elements;
    
    output_data[i] = condition_data[cond_idx] ? x_data[x_idx] : y_data[y_idx];
  }

  return status;
}

base::Status OpWhere::inferShape() {
  base::Status status = base::kStatusCodeOk;

  // Where支持广播，输出形状为三个输入形状的广播结果
  auto condition_shape = inputs_[0]->getShape();
  auto x_shape = inputs_[1]->getShape();
  auto y_shape = inputs_[2]->getShape();

  // 找到最大维度
  size_t max_dims = std::max({condition_shape.size(), x_shape.size(), y_shape.size()});

  // 计算广播后的输出形状
  base::IntVector output_shape;
  for (size_t i = 0; i < max_dims; ++i) {
    int cond_dim = (i < condition_shape.size())
                       ? condition_shape[condition_shape.size() - 1 - i] : 1;
    int x_dim = (i < x_shape.size())
                    ? x_shape[x_shape.size() - 1 - i] : 1;
    int y_dim = (i < y_shape.size())
                    ? y_shape[y_shape.size() - 1 - i] : 1;

    int max_dim = std::max({cond_dim, x_dim, y_dim});
    
    // 检查广播兼容性
    if ((cond_dim != max_dim && cond_dim != 1) ||
        (x_dim != max_dim && x_dim != 1) ||
        (y_dim != max_dim && y_dim != 1)) {
      NNDEPLOY_LOGE("Incompatible shapes for Where broadcasting");
      return base::kStatusCodeErrorInvalidParam;
    }
    
    output_shape.insert(output_shape.begin(), max_dim);
  }

  outputs_[0]->reshape(output_shape);
  return status;
}

base::Status where(device::Tensor* input1, device::Tensor* input2, device::Tensor* condition, 
                       device::Tensor* output) {
  base::Status status = base::kStatusCodeOk;

  Op* op = createOp(input1->getDeviceType(), "", ir::kOpTypeWhere);
  if (op == nullptr) {
    NNDEPLOY_LOGE("createOp failed");
    return base::kStatusCodeErrorNotImplement;
  }

  status = op->setInput(input1, 0);
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "setInput failed");
  status = op->setInput(input2, 1);
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "setInput failed");
  status = op->setInput(condition, 2);
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

REGISTER_OP_IMPLEMENTION(kDeviceTypeCodeCpu, ir::kOpTypeWhere, OpWhere)

}  // namespace op
}  // namespace nndeploy
