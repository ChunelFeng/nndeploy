
#include "nndeploy/op/op_gather.h"

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

base::Status OpGather::run() {
  base::Status status = base::kStatusCodeOk;

  auto param = dynamic_cast<ir::GatherParam*>(op_desc_.op_param_.get());
  int axis = param ? param->axis_ : 0;

  device::Tensor* data_tensor = inputs_[0];
  device::Tensor* indices_tensor = inputs_[1];
  device::Tensor* output_tensor = outputs_[0];

  auto data_shape = data_tensor->getShape();
  auto indices_shape = indices_tensor->getShape();
  auto output_shape = output_tensor->getShape();

  int data_rank = static_cast<int>(data_shape.size());
  
  // 处理负数axis
  if (axis < 0) {
    axis += data_rank;
  }

  float* data = static_cast<float*>(data_tensor->getData());
  int64_t* indices = static_cast<int64_t*>(indices_tensor->getData());
  float* output = static_cast<float*>(output_tensor->getData());

  // 计算strides
  std::vector<long> data_strides(data_rank);
  data_strides[data_rank - 1] = 1;
  for (int i = data_rank - 2; i >= 0; --i) {
    data_strides[i] = data_strides[i + 1] * data_shape[i + 1];
  }

  long indices_elements = std::accumulate(indices_shape.begin(), indices_shape.end(),
                                          1L, std::multiplies<long>());
  long output_elements = std::accumulate(output_shape.begin(), output_shape.end(),
                                         1L, std::multiplies<long>());

  // 计算outer_size和inner_size
  long outer_size = 1;
  for (int i = 0; i < axis; ++i) {
    outer_size *= data_shape[i];
  }

  long inner_size = 1;
  for (int i = axis + 1; i < data_rank; ++i) {
    inner_size *= data_shape[i];
  }

  long axis_size = data_shape[axis];

  // Gather操作
  for (long outer = 0; outer < outer_size; ++outer) {
    for (long idx = 0; idx < indices_elements; ++idx) {
      int64_t gather_idx = indices[idx];
      // 处理负数索引
      if (gather_idx < 0) {
        gather_idx += axis_size;
      }
      
      for (long inner = 0; inner < inner_size; ++inner) {
        long data_idx = outer * axis_size * inner_size + 
                       gather_idx * inner_size + inner;
        long out_idx = outer * indices_elements * inner_size + 
                       idx * inner_size + inner;
        output[out_idx] = data[data_idx];
      }
    }
  }

  return status;
}

base::Status OpGather::inferShape() {
  auto input_shape = inputs_[1]->getShape();
  outputs_[0]->reshape(input_shape);
  return base::kStatusCodeOk;
}

base::Status gather(device::Tensor* input, device::Tensor* index,
                    std::shared_ptr<ir::GatherParam> param,
                    device::Tensor* output) {
  base::Status status = base::kStatusCodeOk;

  Op* op = createOp(input->getDeviceType(), "", ir::kOpTypeGather);
  if (op == nullptr) {
    NNDEPLOY_LOGE("createOp failed");
    return base::kStatusCodeErrorNotImplement;
  }

  status = op->setParam(param);
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "setParam failed");
  status = op->setInput(input, 0);
  NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "setInput failed");
  status = op->setInput(index, 1);
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

REGISTER_OP_IMPLEMENTION(kDeviceTypeCodeCpu, ir::kOpTypeGather, OpGather)

}  // namespace op
}  // namespace nndeploy
