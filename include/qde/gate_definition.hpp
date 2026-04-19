#ifndef GATE_DEFINITION_HPP_
#define GATE_DEFINITION_HPP_

#include <complex>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace qde {

// ---------------------------------------------------------------------------
// GateDefinition
//
// Uniformly describes any gate type built-in or user-defined.
//
// Built-in instances are constructed once in GateRegistry::withBuiltins().
// User-defined gates are constructed dynamically.
// ---------------------------------------------------------------------------

class GateDefinition {
public:
  using MatrixFn = std::function<
    std::vector<std::complex<double>>(const std::vector<double>&)>;

  // Fixed-matrix gate constructor (num_params == 0)
  GateDefinition(std::string name, std::uint8_t num_qubits,
                 std::vector<std::complex<double>> matrix)
      : name_(std::move(name)), num_qubits_(num_qubits), num_params_(0) {
    if (num_qubits_ < 1) {
      throw std::invalid_argument("GateDefinition: num_qubits must be >= 1");
    }

    const std::size_t dim = std::size_t{1} << num_qubits_;
    if (matrix.size() != dim * dim) {
      throw std::invalid_argument(
        "GateDefinition: matrix size must equal " + std::to_string(dim * dim) +
        " ((2^" + std::to_string(num_qubits_) + ")^2), got " +
        std::to_string(matrix.size()));
    }

    matrix_fn_ = [m = std::move(matrix)](const std::vector<double>&) {
      return m;
    };
  }

  // Parametric gate constructor
  // matrix_fn must return a vector of size (2^num_qubits)^2.
  GateDefinition(std::string name, std::uint8_t num_qubits,
                 std::uint8_t num_params, MatrixFn matrix_fn) 
      : name_(std::move(name)), num_qubits_(num_qubits),
        num_params_(num_params), matrix_fn_(std::move(matrix_fn)) {
    if (num_qubits_ < 1) {
      throw std::invalid_argument("GateDefinition: num_qubits must be >= 1");
    }
    if (!matrix_fn_) {
      throw std::invalid_argument(
        "GateDefinition: matrix_fn must not be null");
    }
  }

  const std::string& name() const noexcept { return name_; }
  std::uint8_t numQubits()  const noexcept { return num_qubits_; }
  std::uint8_t numParams()  const noexcept { return num_params_; }

  // Evaluate the unitary for the given parameters.
  std::vector<std::complex<double>> matrix(
      const std::vector<double>& params = {}) const {
    if (params.size() != num_params_) {
      throw std::invalid_argument(
        "GateDefinition::matrix: expected " + std::to_string(num_params_) +
        " params, got " + std::to_string(params.size()));
    }
    return matrix_fn_(params);
  }

private:
  std::string name_;
  std::uint8_t num_qubits_;
  std::uint8_t num_params_;
  MatrixFn matrix_fn_;
};

}  // namespace qde

#endif  // GATE_DEFINITION_HPP_
