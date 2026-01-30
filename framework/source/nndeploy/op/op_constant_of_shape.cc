/*
 *   Copyright (c) 2025
 *   All rights reserved.
 */
#include "nndeploy/op/op_constant_of_shape.h"

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

base::Status OpConstantOfShape::inferShape() {
  base::Status status = base::kStatusCodeOk;

  device::Tensor* shape_tensor = inputs_[0];
  int64_t* shape_data = static_cast<int64_t*>(shape_tensor->getData());
  int shape_size = shape_tensor->getShapeIndex(0);

  base::IntVector output_shape;
  for (int i = 0; i < shape_size; ++i) {
    output_shape.push_back(static_cast<int>(shape_data[i]));
  }

  outputs_[0]->reshape(output_shape);
  return status;
}

base::Status OpConstantOfShape::inferDataType() {
  base::Status status = base::kStatusCodeOk;

  auto param = dynamic_cast<ir::ConstantOfShapeParam*>(op_desc_.op_param_.get());
  if (param != nullptr) {
    outputs_[0]->setDataType(param->datatype_);
  } else {
    // 默认为float类型
    outputs_[0]->setDataType(base::dataTypeOf<float>());
  }

  return status;
}

base::Status OpConstantOfShape::run() {
  base::Status status = base::kStatusCodeOk;

  auto param = dynamic_cast<ir::ConstantOfShapeParam*>(op_desc_.op_param_.get());
  float value = param ? param->value_ : 0.0f;
  base::DataType datatype = param ? param->datatype_ : base::dataTypeOf<float>();

  device::Tensor* output_tensor = outputs_[0];
  auto output_shape = output_tensor->getShape();
  long output_elements = std::accumulate(output_shape.begin(), output_shape.end(),
                                         1L, std::multiplies<long>());

  void* output_data = output_tensor->getData();

  // 根据数据类型填充值
  if (datatype.code_ == base::kDataTypeCodeFp && datatype.bits_ == 32) {
    float* data = static_cast<float*>(output_data);
    for (long i = 0; i < output_elements; ++i) {
      data[i] = value;
    }
  } else if (datatype.code_ == base::kDataTypeCodeFp && datatype.bits_ == 64) {
    double* data = static_cast<double*>(output_data);
    for (long i = 0; i < output_elements; ++i) {
      data[i] = static_cast<double>(value);
    }
  } else if (datatype.code_ == base::kDataTypeCodeInt && datatype.bits_ == 32) {
    int32_t* data = static_cast<int32_t*>(output_data);
    for (long i = 0; i < output_elements; ++i) {
      data[i] = static_cast<int32_t>(value);
    }
  } else if (datatype.code_ == base::kDataTypeCodeInt && datatype.bits_ == 64) {
    int64_t* data = static_cast<int64_t*>(output_data);
    for (long i = 0; i < output_elements; ++i) {
      data[i] = static_cast<int64_t>(value);
    }
  } else {
    // 默认按float处理
    float* data = static_cast<float*>(output_data);
    for (long i = 0; i < output_elements; ++i) {
      data[i] = value;
    }
  }

  return status;
}

base::Status constantOfShape(device::Tensor* shape,
                             std::shared_ptr<ir::ConstantOfShapeParam> param,
                             device::Tensor* output) {
  base::Status status = base::kStatusCodeOk;

  Op* op = createOp(shape->getDeviceType(), "", ir::kOpTypeConstantOfShape);
  if (op == nullptr) {
    NNDEPLOY_LOGE("createOp failed");
    return base::kStatusCodeErrorNotImplement;
  }
  if (param != nullptr) {
    status = op->setParam(param);
    NNDEPLOY_RETURN_ON_NEQ(status, base::kStatusCodeOk, "setParam failed");
  }
  status = op->setInput(shape, 0);
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

REGISTER_OP_IMPLEMENTION(kDeviceTypeCodeCpu, ir::kOpTypeConstantOfShape, OpConstantOfShape)

}  // namespace op
}  // namespace nndeploy

