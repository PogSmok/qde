#ifndef GATE_REGISTRY_HPP_
#define GATE_REGISTRY_HPP_

#include <memory>
#include <string>
#include <unordered_map>

#include "qde/gate_definition.hpp"

namespace qde {

// ---------------------------------------------------------------------------
// GateRegistry
//
// Maps gate names to their definitions.
// Built-in gates are pre-loaded via GateRegistry::WithBuiltins().
// User-defined gates are added with Add().
// ---------------------------------------------------------------------------

class GateRegistry {
 public:
  // Returns a registry pre-populated with every standard OpenQASM 3.0 gate.
  static GateRegistry WithBuiltins();

  // OpenQASM 3.0 §6.4: redefining a gate is a compile-time error.
  // Throws std::invalid_argument if a gate with the same name already exists.
  void Add(GateDefinition gate);

  // Returns the definition, or nullptr if not found.
  [[nodiscard]] std::shared_ptr<const GateDefinition> Find(
      const std::string& name) const;

  [[nodiscard]] bool Contains(const std::string& name) const;

 private:
  std::unordered_map<std::string, std::shared_ptr<const GateDefinition>> gates_;
};

}  // namespace qde

#endif  // GATE_REGISTRY_HPP_
