#include <c10/util/Exception.h>
#include <torch/csrc/jit/codegen/cuda/arith.h>
#include <torch/csrc/jit/codegen/cuda/ir_base_nodes.h>
#include <torch/csrc/jit/codegen/cuda/tensor.h>

#include <sstream>

namespace torch {
namespace jit {
namespace fuser {

// Will return a new value of type val with the DataType dtype, if it's a
// tensorview it will propagate the shape information from val.
Val* newValLike(const Val* const val, DataType dtype) {
  switch (val->getValType().value()) {
    case (ValType::TensorView):
      return static_cast<const TensorView* const>(val)->newForOutput(dtype);
    case (ValType::Scalar):
      switch (dtype) {
        case (DataType::Float):
          return new Float();
        case (DataType::Int):
          return new Int();
        default:
          break;
      }
    default:
      break;
  }
  std::stringstream err_msg;
  err_msg << "Could not generate a new value of type "
          << val->getValType().value() << " with data type "
          << val->getDataType().value() << std::endl;
  TORCH_CHECK(false, err_msg.str());
}

Val* newValLike(const Val* const val) {
  return newValLike(val, val->getDataType().value());
}

Val* promoteNew(Val* v1, Val* v2) {
  // Can't promote two types if they aren't both
  // values with valid data types.
  TORCH_CHECK(v1->isVal() && v2->isVal());
  TORCH_CHECK(
      v1->getDataType() != DataType::Null &&
      v2->getDataType() != DataType::Null);

  ValType out_vtype =
      promote_type(v1->getValType().value(), v2->getValType().value());
  DataType out_dtype =
      promote_type(v1->getDataType().value(), v2->getDataType().value());

  if (out_vtype == v2->getValType().value())
    return newValLike(v2, out_dtype);

  return newValLike(v1, out_dtype);
}

Val* castOp(DataType dtype, Val* v1) {
  if (v1->getDataType().value() == dtype)
    return v1;

  if (!is_cast_legal(v1->getDataType().value(), dtype)) {
    std::stringstream err;
    err << "Illegal Cast value from  DataType: " << v1->getDataType().value()
        << " to DataType: " << dtype;
    TORCH_CHECK(false, err.str());
  }

  Val* out = newValLike(v1, dtype);
  Statement* expr = new UnaryOp(UnaryOpType::Cast, out, v1);
  return out;
}

Val* unaryOp(UnaryOpType type, Val* v1) {
  Val* out = newValLike(v1);
  Statement* expr = new UnaryOp(type, out, v1);
  return out;
}

Val* binaryOp(BinaryOpType type, Val* v1, Val* v2) {
  Val* out = promoteNew(v1, v2);
  if (type >= BinaryOpType::Mod) {
    if (out->getDataType().value() != DataType::Int)
      out = newValLike(out, DataType::Int);
  }
  Statement* expr = new BinaryOp(type, out, v1, v2);
  return out;
}

Val* add(Val* v1, Val* v2) {
  return binaryOp(BinaryOpType::Add, v1, v2);
}

Val* sub(Val* v1, Val* v2) {
  return binaryOp(BinaryOpType::Sub, v1, v2);
}

Val* mul(Val* v1, Val* v2) {
  return binaryOp(BinaryOpType::Mul, v1, v2);
}

Val* div(Val* v1, Val* v2) {
  return binaryOp(BinaryOpType::Div, v1, v2);
}

Val* mod(Val* v1, Val* v2) {
  return binaryOp(BinaryOpType::Mod, v1, v2);
}

Val* lt(Val* v1, Val* v2) {
  return binaryOp(BinaryOpType::LT, v1, v2);
}

Val* ceilDiv(Val* v1, Val* v2) {
  return binaryOp(BinaryOpType::CeilDiv, v1, v2);
}

} // namespace fuser
} // namespace jit
} // namespace torch