#ifndef _NNDEPLOY_OP_OP_SILU_H_
#define _NNDEPLOY_OP_OP_SILU_H_

#include "nndeploy/ir/ir.h"
#include "nndeploy/op/op.h"
#include "nndeploy/op/op_unary.h"

namespace nndeploy {
namespace op {

class OpSilu : public OpUnary {
 public:
  OpSilu() : OpUnary() { is_inplace_ = true; }
  virtual ~OpSilu() {}

  virtual base::Status run();
};

NNDEPLOY_CC_API base::Status silu(device::Tensor *input,
                                  device::Tensor *output);

}  // namespace op
}  // namespace nndeploy

#endif  // _NNDEPLOY_OP_OP_SILU_H_
