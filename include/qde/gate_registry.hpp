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
// Built-in gates are pre-loaded via GateRegistry::withBuiltins().
// User-defined gates are added with add().
// ---------------------------------------------------------------------------

class GateRegistry {
 public:
  // Returns a registry pre-populated with every standard OpenQASM 3.0 gate.
  static GateRegistry withBuiltins();

  // OpenQASM 3.0 §6.4: redefining a gate is a compile-time error.
  // Throws std::invalid_argument if a gate with the same name already exists.
  void add(GateDefinition gate);

  // Returns the definition, or nullptr if not found.
  [[nodiscard]] std::shared_ptr<const GateDefinition> find(
      const std::string& name) const;

  [[nodiscard]] bool contains(const std::string& name) const;

 private:
  std::unordered_map<std::string, std::shared_ptr<const GateDefinition>> gates_;
};

}  // namespace qde

#endif  // GATE_REGISTRY_HPP_
