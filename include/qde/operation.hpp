#ifndef OPERATION_HPP_
#define OPERATION_HPP_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "qde/gate_definition.hpp"

namespace qde {

struct QubitReference {
  std::uint32_t reg;    // index into Circuit::QubitRegisters()
  std::uint32_t qubit;  // index within that register
};

struct BitReference {
  std::uint32_t reg;  // index into Circuit::BitRegisters()
  std::uint32_t bit;  // index within that register
};

// A boolean function of some classical bits, given by its truth table.
struct Condition {
  static constexpr std::size_t kMaxBits = 10;

  std::vector<BitReference> bits;
  // The truth table of the condition: truth[i] is its value when the bits
  // read as the binary number i, with bits[0] as the least significant digit.
  //
  // For example, if bits = {c[0], c[1]} and truth = {0, 1, 0, 0}, the
  // condition is c[0] AND NOT c[1]
  std::vector<std::uint8_t> truth;

  Condition() = default;

  Condition(std::vector<BitReference> bits, std::vector<std::uint8_t> truth)
      : bits(std::move(bits)), truth(std::move(truth)) {}

  // True iff `bit` equals `expected`.
  Condition(BitReference bit, std::uint8_t expected)
      : bits{bit},
        truth{static_cast<std::uint8_t>(expected == 0),
              static_cast<std::uint8_t>(expected != 0)} {}

  // The constant `value`.
  [[nodiscard]] static Condition Constant(bool value) {
    return Condition(
        std::vector<BitReference>{},
        std::vector<std::uint8_t>{static_cast<std::uint8_t>(value)});
  }
};

enum class OperationType : std::uint8_t {
  kGate,     // unitary gate application
  kMeasure,  // measurement, writes result to measure_target
  kReset,    // reset qubit to |0>
  kBarrier,  // prevents gate reordering across this point
  kAssign,   // classical assignment, writes assign_sources to measure_target
  kBlock,    // a part of the program with a header of its own, see BlockKind
};

enum class BlockKind : std::uint8_t {
  kIf,
  kElse,  // follows the kIf it belongs to
  kWhile,
  kFor,
  kSwitch,
  kCase,  // one case of a switch, or its default
  kCall,  // the body of a subroutine, where it is called
  kGate,  // the body of a gate defined in the program, where it is applied
  kBox,
  kScope,  // statements in braces of their own: { }
  kStatement,
};

struct Operation {
  Operation() = default;

  Operation(OperationType type, std::shared_ptr<const GateDefinition> gate,
            std::vector<double> gate_params, std::vector<QubitReference> qubits,
            std::vector<BitReference> measure_target,
            std::optional<Condition> condition)
      : type(type),
        gate(std::move(gate)),
        gate_params(std::move(gate_params)),
        qubits(std::move(qubits)),
        measure_target(std::move(measure_target)),
        condition(std::move(condition)) {}

  OperationType type = OperationType::kGate;
  std::shared_ptr<const GateDefinition> gate;  // non-null if type == Gate
  std::vector<double> gate_params;
  std::vector<QubitReference> qubits;
  std::vector<BitReference> measure_target;
  // if set, the operation is skipped unless the condition holds
  std::optional<Condition> condition;
  // kAssign: the value written to measure_target[i].
  std::vector<Condition> assign_sources;

  // ---- kBlock ---------------------------------------------------------------
  BlockKind block_kind = BlockKind::kIf;
  // The header of the block, such as "while (!c)".
  std::string label;
  std::vector<Operation> body;
  // Block is considered dynamic if its body is not known at compile time
  bool is_dynamic = false;
};

}  // namespace qde

#endif  // OPERATION_HPP_
