#include "qde/gate_registry.hpp"

#include <cmath>
#include <string>

namespace qde {

void GateRegistry::add(GateDefinition gate) {
  std::string name = gate.name();  // non-const copy to allow std::move()
  if (gates_.count(name) != 0U) {
    throw std::invalid_argument(
        "GateRegistry: gate '" + name +
        "' is already defined (OpenQASM 3.0 does not allow redefinitions)");
  }
  gates_.emplace(std::move(name),
                 std::make_shared<GateDefinition>(std::move(gate)));
}

std::shared_ptr<const GateDefinition> GateRegistry::find(
    const std::string& name) const {
  auto it = gates_.find(name);
  return it != gates_.end() ? it->second : nullptr;
}

bool GateRegistry::contains(const std::string& name) const {
  return gates_.count(name) != 0U;
}

// Row-major unitary matrices for built-in gates.
// Basis ordering: |0...0⟩, |0...1⟩, ..., |1...1⟩  (first qubit = MSB).

namespace {

using C = std::complex<double>;  // complex number
using M = std::vector<C>;        // matrix of complex numbers
using P = std::vector<double>;   // gate parameters

// Imaginary units and frequently used constants
constexpr C kI{0.0, 1.0};
constexpr C kMI{0.0, -1.0};
constexpr double kIS2 = 0.7071067811865475244;  // 1/sqrt(2)

// ---- helper functions -----------------------------------------------------

// 2×2 gate from four entries (row-major).
M mat2(C a, C b, C c, C d) { return {a, b, c, d}; }

// 4×4 gate from sixteen entries (row-major).
M mat4(C a00, C a01, C a02, C a03, C a10, C a11, C a12, C a13, C a20, C a21,
       C a22, C a23, C a30, C a31, C a32, C a33) {
  return {a00, a01, a02, a03, a10, a11, a12, a13,
          a20, a21, a22, a23, a30, a31, a32, a33};
}

// (2^n)x(2^n) gate from operations (row-major).
// Builds a 2^n × 2^n identity matrix,
// then exchanges row pairs and patches diagonal entries.
using Swap = std::pair<std::size_t, std::size_t>;
using DiagPatch = std::pair<std::size_t, C>;
M makeMatrix(std::size_t n, std::initializer_list<Swap> swaps = {},
             std::initializer_list<DiagPatch> diag_patches = {}) {
  const std::size_t dim = std::size_t{1} << n;
  M m(dim * dim, C{0.0, 0.0});
  for (std::size_t i = 0; i < dim; ++i) {
    m[(i * dim) + i] = C{1.0, 0.0};
  }

  for (auto [a, b] : swaps) {
    m[(a * dim) + a] = C{0.0, 0.0};
    m[(b * dim) + b] = C{0.0, 0.0};
    m[(a * dim) + b] = C{1.0, 0.0};
    m[(b * dim) + a] = C{1.0, 0.0};
  }

  for (auto [i, v] : diag_patches) {
    m[(i * dim) + i] = v;
  }

  return m;
}

// ---- fixed single-qubit gates ---------------------------------------------

inline const M& kMatI() {
  static const M v = mat2(1, 0, 0, 1);
  return v;
}

inline const M& kMatH() {
  static const M v = mat2(kIS2, kIS2, kIS2, -kIS2);
  return v;
}

inline const M& kMatX() {
  static const M v = mat2(0, 1, 1, 0);
  return v;
}

inline const M& kMatY() {
  static const M v = mat2(0, kMI, kI, 0);
  return v;
}

inline const M& kMatZ() {
  static const M v = mat2(1, 0, 0, -1);
  return v;
}

inline const M& kMatS() {
  static const M v = mat2(1, 0, 0, kI);
  return v;
}

inline const M& kMatSdg() {
  static const M v = mat2(1, 0, 0, kMI);
  return v;
}

inline const M& kMatT() {
  static const M v = mat2(1, 0, 0, C{kIS2, kIS2});
  return v;
}

inline const M& kMatTdg() {
  static const M v = mat2(1, 0, 0, C{kIS2, -kIS2});
  return v;
}

inline const M& kMatSX() {
  static const M v = mat2(C{0.5, 0.5}, C{0.5, -0.5}, C{0.5, -0.5}, C{0.5, 0.5});
  return v;
}

inline const M& kMatSXdg() {
  static const M v = mat2(C{0.5, -0.5}, C{0.5, 0.5}, C{0.5, 0.5}, C{0.5, -0.5});
  return v;
}

// ---- fixed two-qubit gates ------------------------------------------------

// CX: |10⟩ ↔ |11⟩
inline const M& kMatCX() {
  static const M v = mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0);
  return v;
}

// CZ: phase flip on |11⟩
inline const M& kMatCZ() {
  static const M v = mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1);
  return v;
}

// CY: |10⟩ ↔ |11⟩ with Y factors
inline const M& kMatCY() {
  static const M v = mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, kMI, 0, 0, kI, 0);
  return v;
}

// SWAP: |01⟩ ↔ |10⟩
inline const M& kMatSWAP() {
  static const M v = mat4(1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1);
  return v;
}

// iSWAP: |01⟩ ↔ i|10⟩
inline const M& kMatISWAP() {
  static const M v = mat4(1, 0, 0, 0, 0, 0, kI, 0, 0, kI, 0, 0, 0, 0, 0, 1);
  return v;
}

// ---- fixed three-qubit gates ----------------------------------------------

// CCX (Toffoli): |110⟩ ↔ |111⟩  (rows 6 ↔ 7)
inline const M& kMatCCX() {
  static const M v = makeMatrix(3, {{6, 7}});
  return v;
}

// CCZ: phase flip on |111⟩  (row 7 diagonal = -1)
inline const M& kMatCCZ() {
  static const M v = makeMatrix(3, {}, {{7, C{-1.0, 0.0}}});
  return v;
}

// CSWAP (Fredkin): |101⟩ ↔ |110⟩  (rows 5 ↔ 6)
inline const M& kMatCSWAP() {
  static const M v = makeMatrix(3, {{5, 6}});
  return v;
}

}  // namespace

GateRegistry GateRegistry::withBuiltins() {
  GateRegistry reg;

  // ---- fixed single-qubit -------------------------------------------------
  reg.add({"id", 1, kMatI()});
  reg.add({"h", 1, kMatH()});
  reg.add({"x", 1, kMatX()});
  reg.add({"y", 1, kMatY()});
  reg.add({"z", 1, kMatZ()});
  reg.add({"s", 1, kMatS()});
  reg.add({"sdg", 1, kMatSdg()});
  reg.add({"t", 1, kMatT()});
  reg.add({"tdg", 1, kMatTdg()});
  reg.add({"sx", 1, kMatSX()});
  reg.add({"sxdg", 1, kMatSXdg()});

  // ---- parametric single-qubit --------------------------------------------

  reg.add({"p", 1, 1,
           [](const P& p) { return mat2(1, 0, 0, std::exp(kI * p[0])); }});

  // legacy OpenQASM 2.0 alias for phase gate (p)
  reg.add({"u1", 1, 1,
           [](const P& p) { return mat2(1, 0, 0, std::exp(kI * p[0])); }});

  reg.add({"u2", 1, 2, [](const P& p) {
             const double phi = p[0];
             const double lam = p[1];
             return mat2(kIS2, -std::exp(kI * lam) * kIS2,
                         std::exp(kI * phi) * kIS2,
                         std::exp(kI * (phi + lam)) * kIS2);
           }});

  reg.add({"U", 1, 3, [](const P& p) {
             const double theta = p[0];
             const double phi = p[1];
             const double lam = p[2];
             const double c = std::cos(theta / 2.0);
             const double s = std::sin(theta / 2.0);
             return mat2(c, -std::exp(kI * lam) * s, std::exp(kI * phi) * s,
                         std::exp(kI * (phi + lam)) * c);
           }});

  // legacy OpenQASM 2.0 alias for unitary gate (U)
  reg.add({"u3", 1, 3, [](const P& p) {
             const double theta = p[0];
             const double phi = p[1];
             const double lam = p[2];
             const double c = std::cos(theta / 2.0);
             const double s = std::sin(theta / 2.0);
             return mat2(c, -std::exp(kI * lam) * s, std::exp(kI * phi) * s,
                         std::exp(kI * (phi + lam)) * c);
           }});

  reg.add({"rx", 1, 1, [](const P& p) {
             const double c = std::cos(p[0] / 2.0);
             const double s = std::sin(p[0] / 2.0);
             return mat2(c, kMI * s, kMI * s, c);
           }});

  reg.add({"ry", 1, 1, [](const P& p) {
             const double c = std::cos(p[0] / 2.0);
             const double s = std::sin(p[0] / 2.0);
             return mat2(c, -s, s, c);
           }});

  reg.add({"rz", 1, 1, [](const P& p) {
             const double h = p[0] / 2.0;
             return mat2(std::exp(kMI * h), 0, 0, std::exp(kI * h));
           }});

  // ---- fixed two-qubit ----------------------------------------------------
  reg.add({"cx", 2, kMatCX()});
  reg.add({"cz", 2, kMatCZ()});
  reg.add({"cy", 2, kMatCY()});
  reg.add({"swap", 2, kMatSWAP()});
  reg.add({"iswap", 2, kMatISWAP()});

  // ---- parametric two-qubit -----------------------------------------------

  reg.add({"cp", 2, 1, [](const P& p) {
             return mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0,
                         std::exp(kI * p[0]));
           }});

  reg.add({"crx", 2, 1, [](const P& p) {
             const double c = std::cos(p[0] / 2.0);
             const double s = std::sin(p[0] / 2.0);
             return mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, c, kMI * s, 0, 0,
                         kMI * s, c);
           }});

  reg.add({"cry", 2, 1, [](const P& p) {
             const double c = std::cos(p[0] / 2.0);
             const double s = std::sin(p[0] / 2.0);
             return mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, c, -s, 0, 0, s, c);
           }});

  reg.add({"crz", 2, 1, [](const P& p) {
             const double h = p[0] / 2.0;
             return mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, std::exp(kMI * h), 0, 0,
                         0, 0, std::exp(kI * h));
           }});

  // ---- three-qubit --------------------------------------------------------
  reg.add({"ccx", 3, kMatCCX()});
  reg.add({"ccz", 3, kMatCCZ()});
  reg.add({"cswap", 3, kMatCSWAP()});

  return reg;
}

}  // namespace qde
