/**
 * nndeploy Net Demo:
 * Demonstrates the basic functionality and usage of nndeploy Net (Default
 inference framework)
 */

#include "flag.h"
#include "nndeploy/base/status.h"
#include "nndeploy/ir/default_interpret.h"
#include "nndeploy/ir/interpret.h"
#include "nndeploy/ir/ir.h"
#include "nndeploy/net/net.h"
#include "nndeploy/op/expr.h"
#include "nndeploy/op/op.h"

using namespace nndeploy;

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);
  if (demo::FLAGS_usage) {
    demo::showUsage();
    return -1;
  }

  base::ModelType model_type = demo::getModelType();
  // 模型路径或者模型字符串
  std::vector<std::string> model_value = demo::getModelValue();

  base::Status status = base::kStatusCodeOk;

  auto interpret =
      std::shared_ptr<ir::Interpret>(ir::createInterpret(model_type));
  if (interpret == nullptr) {
    NNDEPLOY_LOGE("ir::createInterpret failed.\n");
    return -1;
  }
  status = interpret->interpret(model_value);
  if (status != base::kStatusCodeOk) {
    NNDEPLOY_LOGE("interpret failed\n");
    return -1;
  }

  auto net = std::make_shared<net::Net>();
  net->setInterpret(interpret.get());

  base::DeviceType device_type;
  device_type.code_ = base::kDeviceTypeCodeCpu;
  // device_type.code_ = base::kDeviceTypeCodeAscendCL;
  device_type.device_id_ = 0;
  net->setDeviceType(device_type);
  auto device = device::getDevice(device_type);

  net->init();
  // std::map<std::string, std::vector<int>> shape_map = {
  //   {"input_ids", {8, 1, 896}},
  //   {"attention_mask", {1, 1, 8, 8}},
  //   {"position_ids", {1, 8}},
  //   {"past_key_values", {24,2,1,0,2,64}}
  // };
  // status = net->reshape(shape_map);
  // if (status != base::kStatusCodeOk) {
  //   NNDEPLOY_LOGE("tensor_pool_ allocate failed\n");
  //   return -1;
  // }

  // net->dump(std::cout);

  std::vector<device::Tensor *> inputs = net->getAllInput();
  for (auto input : inputs) {
    input->print();
  }
  if (inputs.size() > 0) {
    inputs[0]->reshape({8, 1, 896});
    inputs[0]->allocate(device);
  }
  if (inputs.size() > 1) {
    inputs[1]->reshape({1, 1, 8, 8});
    inputs[1]->allocate(device);
  }
  if (inputs.size() > 2) {
    inputs[2]->reshape({1, 8});
    inputs[2]->allocate(device);
  }
  if (inputs.size() > 3) {
    inputs[3]->reshape({24, 2, 1, 0, 2, 64});
    inputs[3]->allocate(device);
  }

  net->preRun();
  net->run();
  net->postRun();

  net->dump(std::cout);

  net->deinit();

  return 0;
}