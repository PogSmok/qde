#ifndef TESTS_SIMULATOR_CIRCUIT_BUILDERS_HPP_
#define TESTS_SIMULATOR_CIRCUIT_BUILDERS_HPP_

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "qde/circuit.hpp"
#include "qde/gate_registry.hpp"
#include "qde/operation.hpp"

// Shorthand for assembling circuits in simulator tests.
namespace qde::simulator::test_util {

inline Circuit MakeCircuit(std::vector<QubitRegister> qregs,
                           std::vector<BitRegister> bregs,
                           std::vector<Operation> ops) {
  return {GateRegistry::WithBuiltins(), std::move(qregs), std::move(bregs),
          std::move(ops)};
}

inline Operation GateOp(const GateRegistry& registry, const std::string& name,
                        std::vector<QubitReference> qubits,
                        std::vector<double> params = {}) {
  return {OperationType::kGate,
          registry.Find(name),
          std::move(params),
          std::move(qubits),
          {},
          std::nullopt};
}

inline Operation MeasureOp(std::vector<QubitReference> qubits,
                           std::vector<BitReference> bits) {
  return {OperationType::kMeasure, nullptr,         {},
          std::move(qubits),       std::move(bits), std::nullopt};
}

inline Operation ResetOp(std::vector<QubitReference> qubits) {
  return {OperationType::kReset, nullptr, {},
          std::move(qubits),     {},      std::nullopt};
}

inline Operation BarrierOp(std::vector<QubitReference> qubits) {
  return {OperationType::kBarrier, nullptr, {},
          std::move(qubits),       {},      std::nullopt};
}

// `op`, executed only while classical bit `bit` equals `expected`.
inline Operation If(BitReference bit, std::uint8_t expected, Operation op) {
  op.condition = std::make_pair(bit, expected);
  return op;
}

}  // namespace qde::simulator::test_util

#endif  // TESTS_SIMULATOR_CIRCUIT_BUILDERS_HPP_
