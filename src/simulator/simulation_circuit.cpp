#include "qde/simulator/simulation_circuit.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "qde/circuit.hpp"
#include "qde/operation.hpp"
#include "qde/qubit_allocator.hpp"
#include "qde/simulator/compiled_operation.hpp"

namespace qde::simulator {

namespace {

// Throws unless the registers declare at most kMaxQubits qubits in total.
// Comparing each size against the remaining budget cannot overflow.
void CheckQubitLimit(const std::vector<QubitRegister>& qregs) {
  std::size_t declared_qubits = 0;
  for (const QubitRegister& qreg : qregs) {
    if (qreg.size > kMaxQubits - declared_qubits) {
      throw std::invalid_argument(
          "SimulationCircuit: too many qubits for simulation (max " +
          std::to_string(kMaxQubits) + ")");
    }
    declared_qubits += qreg.size;
  }
}

// qubit_index[reg][qubit] is the hardware index of that qubit. Looking the
// name "reg[i]" up once here spares rebuilding it for every operation.
std::vector<std::vector<std::size_t>> BuildQubitIndex(
    const std::vector<QubitRegister>& qregs,
    const std::unordered_map<std::string, std::size_t>& hardware_mapping) {
  std::vector<std::vector<std::size_t>> qubit_index(qregs.size());
  for (std::size_t r = 0; r < qregs.size(); ++r) {
    qubit_index[r].resize(qregs[r].size);
    for (std::size_t q = 0; q < qregs[r].size; ++q) {
      const std::string name = qregs[r].name + "[" + std::to_string(q) + "]";
      const std::size_t hardware_qubit = hardware_mapping.at(name);
      if (hardware_qubit >= hardware_mapping.size()) {
        throw std::invalid_argument(
            "SimulationCircuit: qubit allocation is out of range");
      }
      qubit_index[r][q] = hardware_qubit;
    }
  }
  return qubit_index;
}

// Prefix sums of the register sizes: bit_offsets[r] is the flat index of the
// first bit of register r, and the last element is the total bit count.
std::vector<std::size_t> BuildBitOffsets(
    const std::vector<BitRegister>& bregs) {
  std::vector<std::size_t> bit_offsets;
  bit_offsets.reserve(bregs.size() + 1);
  std::size_t offset = 0;
  bit_offsets.push_back(offset);
  for (const BitRegister& breg : bregs) {
    if (breg.size > kMaxBits - offset) {
      throw std::invalid_argument(
          "SimulationCircuit: too many classical bits for simulation (max " +
          std::to_string(kMaxBits) + ")");
    }
    offset += breg.size;
    bit_offsets.push_back(offset);
  }
  return bit_offsets;
}

// Resolves the register references of one circuit to flat indices, rejecting
// references to qubits and bits that were never declared.
class RegisterMap {
 public:
  explicit RegisterMap(const Circuit& circuit)
      : RegisterMap(circuit, QubitAllocator::Allocate(circuit)) {}

  [[nodiscard]] std::size_t QubitCount() const noexcept { return qubit_count_; }

  [[nodiscard]] std::size_t BitCount() const noexcept {
    return bit_offsets_.back();
  }

  [[nodiscard]] std::size_t Qubit(const QubitReference& qubit) const {
    if (qubit.reg >= qubit_index_.size() ||
        qubit.qubit >= qubit_index_[qubit.reg].size()) {
      throw std::invalid_argument(
          "SimulationCircuit: operation references a qubit that does not "
          "exist");
    }
    return qubit_index_[qubit.reg][qubit.qubit];
  }

  [[nodiscard]] std::size_t Bit(const BitReference& bit) const {
    const std::size_t next = std::size_t{bit.reg} + 1;
    if (next >= bit_offsets_.size() ||
        bit.bit >= bit_offsets_[next] - bit_offsets_[bit.reg]) {
      throw std::invalid_argument(
          "SimulationCircuit: operation references a bit that does not exist");
    }
    return bit_offsets_[bit.reg] + bit.bit;
  }

 private:
  RegisterMap(
      const Circuit& circuit,
      const std::unordered_map<std::string, std::size_t>& hardware_mapping)
      : qubit_index_(
            BuildQubitIndex(circuit.QubitRegisters(), hardware_mapping)),
        bit_offsets_(BuildBitOffsets(circuit.BitRegisters())),
        qubit_count_(hardware_mapping.size()) {}

  std::vector<std::vector<std::size_t>> qubit_index_;
  std::vector<std::size_t> bit_offsets_;
  std::size_t qubit_count_;
};

// Throws unless `op` carries a gate that can act on `qubits`.
void CheckGate(const Operation& op, const std::vector<std::size_t>& qubits) {
  if (!op.gate) {
    throw std::invalid_argument(
        "SimulationCircuit: gate operation without a gate definition");
  }
  if (op.gate->NumQubits() != qubits.size()) {
    throw std::invalid_argument(
        "SimulationCircuit: gate '" + op.gate->Name() + "' expects " +
        std::to_string(op.gate->NumQubits()) + " qubits, got " +
        std::to_string(qubits.size()));
  }
  // A gate acting twice on the same qubit is not a valid unitary.
  std::vector<std::size_t> sorted_qubits(qubits);
  std::sort(sorted_qubits.begin(), sorted_qubits.end());
  if (std::adjacent_find(sorted_qubits.begin(), sorted_qubits.end()) !=
      sorted_qubits.end()) {
    throw std::invalid_argument(
        "SimulationCircuit: gate applied to the same qubit more than once");
  }
}

// Resolves every register reference of `op` and validates the result.
CompiledOperation Compile(const Operation& op, const RegisterMap& registers) {
  CompiledOperation compiled;
  compiled.type = op.type;
  compiled.gate = op.gate;
  compiled.gate_params = op.gate_params;

  compiled.qubits.reserve(op.qubits.size());
  for (const QubitReference& qubit : op.qubits) {
    compiled.qubits.push_back(registers.Qubit(qubit));
  }
  if (op.type == OperationType::kGate) {
    CheckGate(op, compiled.qubits);
  }

  if (op.type == OperationType::kMeasure && !op.measure_target.empty() &&
      op.measure_target.size() != op.qubits.size()) {
    throw std::invalid_argument(
        "SimulationCircuit: measurement must target one bit per qubit or "
        "none");
  }
  compiled.measure_target.reserve(op.measure_target.size());
  for (const BitReference& bit : op.measure_target) {
    compiled.measure_target.push_back(registers.Bit(bit));
  }

  if (op.condition) {
    const auto& [bit, expected] = *op.condition;
    compiled.condition = {registers.Bit(bit), expected};
  }
  return compiled;
}

// Assigns operations to layers as soon as possible (ASAP) without breaking a
// dependency of the program order.
class AsapScheduler {
 public:
  AsapScheduler(std::size_t qubit_count, std::size_t bit_count)
      : qubit_free_(qubit_count, 0),
        bit_readable_(bit_count, 0),
        bit_writable_(bit_count, 0) {}

  // Returns the layer of `op` and reserves the qubits and bits it uses.
  std::size_t Place(const CompiledOperation& op) {
    std::size_t layer = 0;
    for (const std::size_t qubit : op.qubits) {
      layer = std::max(layer, qubit_free_[qubit]);
    }
    for (const std::size_t bit : op.measure_target) {
      layer = std::max(layer, bit_writable_[bit]);
    }
    if (op.condition) {
      layer = std::max(layer, bit_readable_[op.condition->first]);
    }

    for (const std::size_t qubit : op.qubits) {
      qubit_free_[qubit] = layer + 1;
    }
    if (op.condition) {
      std::size_t& writable = bit_writable_[op.condition->first];
      writable = std::max(writable, layer + 1);
    }
    for (const std::size_t bit : op.measure_target) {
      bit_readable_[bit] = layer + 1;
      bit_writable_[bit] = layer + 1;
    }
    return layer;
  }

 private:
  // First layer in which each qubit is free.
  std::vector<std::size_t> qubit_free_;
  // First layer in which each bit holds its latest value: the layer after
  // the last measurement into it.
  std::vector<std::size_t> bit_readable_;
  // First layer in which each bit may be overwritten: the layer after its
  // last use.
  std::vector<std::size_t> bit_writable_;
};

}  // namespace

SimulationCircuit::SimulationCircuit(const Circuit& circuit) {
  // Reject oversized circuits before allocating anything proportional to the
  // register sizes.
  CheckQubitLimit(circuit.QubitRegisters());

  const RegisterMap registers(circuit);
  qubit_count_ = registers.QubitCount();
  bit_count_ = registers.BitCount();

  layers_.emplace_back();
  AsapScheduler scheduler(qubit_count_, bit_count_);
  for (const Operation& op : circuit.Operations()) {
    CompiledOperation compiled = Compile(op, registers);
    const std::size_t layer = scheduler.Place(compiled);
    if (layer == layers_.size()) {
      layers_.emplace_back();
    }
    layers_[layer].push_back(std::move(compiled));
  }
}

}  // namespace qde::simulator
