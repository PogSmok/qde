#include "qde/simulator/simulation_circuit.hpp"

#include <string>
#include <unordered_map>

#include "qde/operation.hpp"
#include "qde/qubit_allocator.hpp"
#include "qde/simulator/compiled_operation.hpp"

namespace qde {

constexpr int kUnusedLayer = -1;  // Indicates no use in any of the layers

SimulationCircuit::SimulationCircuit(const Circuit& circuit) {
  const std::unordered_map<std::string, std::size_t> hardware_mapping =
      QubitAllocator::Allocate(circuit);
  qubit_count_ = hardware_mapping.size();

  const std::vector<QubitRegister>& qregs = circuit.QubitRegisters();
  const std::vector<BitRegister>& bregs = circuit.BitRegisters();

  // Pre-compute flat bit offset for each classical register.
  std::vector<std::size_t> bit_offsets(bregs.size());
  std::size_t offset = 0;
  for (std::size_t i = 0; i < bregs.size(); ++i) {
    bit_offsets[i] = offset;
    offset += bregs[i].size;
  }
  bit_count_ = offset;

  // Pre-compute qubit lookup: qubit_index[reg][qubit] to hardware index.
  // Avoids rebuilding the name string "reg[i]" for every operation.
  std::vector<std::vector<std::size_t>> qubit_index(qregs.size());
  for (std::size_t r = 0; r < qregs.size(); ++r) {
    qubit_index[r].resize(qregs[r].size);
    for (std::size_t q = 0; q < qregs[r].size; ++q) {
      const std::string name = qregs[r].name + "[" + std::to_string(q) + "]";
      qubit_index[r][q] = hardware_mapping.at(name);
    }
  }

  layers_.emplace_back();

  // Tracks the last layer each physical qubit was used in.
  std::vector<int> qubit_last_layer(qubit_count_, kUnusedLayer);

  for (const Operation& op : circuit.Operations()) {
    std::vector<std::size_t> hardware_qubits;
    hardware_qubits.reserve(op.qubits.size());
    for (const QubitReference& qubit : op.qubits) {
      hardware_qubits.push_back(qubit_index[qubit.reg][qubit.qubit]);
    }

    // Resolve classical bit references to flat indices.
    std::vector<std::size_t> bit_indices;
    bit_indices.reserve(op.measure_target.size());
    for (const BitReference& bit : op.measure_target) {
      bit_indices.push_back(bit_offsets[bit.reg] + bit.bit);
    }

    // ASAP scheduling
    int last_used_layer = kUnusedLayer;
    for (const std::size_t q : hardware_qubits) {
      last_used_layer = std::max(last_used_layer, qubit_last_layer[q]);
    }

    const int target_layer = last_used_layer + 1;
    if (target_layer == static_cast<int>(layers_.size())) {
      layers_.emplace_back();
    }

    std::optional<std::pair<std::size_t, std::uint8_t>> cond;
    if (op.condition) {
      const auto& [bit_ref, expected] =
          op.condition.value();  // NOLINT(bugprone-unchecked-optional-access)
      cond = {bit_offsets[bit_ref.reg] + bit_ref.bit, expected};
    }

    layers_[target_layer].push_back(
        {op.type, op.gate, op.gate_params, hardware_qubits, bit_indices, cond});

    for (const std::size_t q : hardware_qubits) {
      qubit_last_layer[q] = target_layer;
    }
  }
}

}  // namespace qde
