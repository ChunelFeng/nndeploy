#ifndef _NNDEPLOY_OP_OP_SWIGLU_H_
#define _NNDEPLOY_OP_OP_SWIGLU_H_

#include "nndeploy/ir/ir.h"
#include "nndeploy/op/op.h"

namespace nndeploy {
namespace op {

class OpSwiGLU : public Op {
 public:
  OpSwiGLU() : Op() { is_inplace_ = false; }
  virtual ~OpSwiGLU() {}

  virtual base::Status inferShape();

  virtual base::Status run();
};

NNDEPLOY_CC_API base::Status swiglu(device::Tensor *input,
                                    device::Tensor *output);

NNDEPLOY_CC_API base::Status swiglu(device::Tensor *input1,
                                    device::Tensor *input2,
                                    device::Tensor *output);

}  // namespace op
}  // namespace nndeploy

#endif  // _NNDEPLOY_OP_OP_SWIGLU_H_
