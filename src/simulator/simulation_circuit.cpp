#include "qde/simulator/simulation_circuit.hpp"

#include <string>
#include <unordered_map>

#include "qde/operation.hpp"
#include "qde/qubit_allocator.hpp"
#include "qde/simulator/compiled_operation.hpp"

namespace qde {

SimulationCircuit::SimulationCircuit(const Circuit& circuit) {
  const std::unordered_map<std::string, std::size_t> hardware_mapping =
      QubitAllocator::allocate(circuit);

  const std::vector<QubitRegister>& qregs = circuit.qubitRegisters();
  const std::vector<BitRegister>& bregs   = circuit.bitRegisters();

  qubit_count_ = hardware_mapping.size();

  // Pre-compute flat bit offset for each classical register.
  std::vector<std::size_t> bit_offsets(bregs.size());
  std::size_t offset = 0;
  for (std::size_t i = 0; i < bregs.size(); ++i) {
    bit_offsets[i] = offset;
    offset += bregs[i].size;
  }
  bit_count_ = offset;

  layers_.push_back({});

  // Tracks the last layer each physical qubit was used in (-1 = never used).
  std::vector<int> qubit_last_layer(qubit_count_, -1);

  for (const Operation& op : circuit.operations()) {
    // Resolve qubit references to hardware indices.
    std::vector<std::size_t> hardware_qubits;
    for (const QubitReference& qubit : op.qubits) {
      const std::string name =
          qregs[qubit.reg].name + "[" + std::to_string(qubit.qubit) + "]";
      hardware_qubits.push_back(hardware_mapping.at(name));
    }

    // Resolve classical bit references to flat indices.
    std::vector<std::size_t> bit_indices;
    for (const BitReference& bit : op.measure_target) {
      bit_indices.push_back(bit_offsets[bit.reg] + bit.bit);
    }

    // ASAP scheduling
    int last_used_layer = -1;
    for (const std::size_t q : hardware_qubits) {
      last_used_layer = std::max(last_used_layer, qubit_last_layer[q]);
    }

    const int target_layer = last_used_layer + 1;
    if (target_layer == static_cast<int>(layers_.size())) {
      layers_.push_back({});
    }

    layers_[target_layer].push_back(
        {op.type, op.gate, op.gate_params, hardware_qubits, bit_indices});

    for (const std::size_t q : hardware_qubits) {
      qubit_last_layer[q] = target_layer;
    }
  }
}

}  // namespace qde
