#include "qde/gate_registry.hpp"

#include <cmath>
#include <string>

namespace qde {

void GateRegistry::Add(GateDefinition gate) {
  std::string name = gate.Name();  // non-const copy to allow std::move()
  if (gates_.count(name) != 0U) {
    throw std::invalid_argument(
        "GateRegistry: gate '" + name +
        "' is already defined (OpenQASM 3.0 does not allow redefinitions)");
  }
  gates_.emplace(std::move(name),
                 std::make_shared<GateDefinition>(std::move(gate)));
}

std::shared_ptr<const GateDefinition> GateRegistry::Find(
    const std::string& name) const {
  auto it = gates_.find(name);
  return it != gates_.end() ? it->second : nullptr;
}

bool GateRegistry::Contains(const std::string& name) const {
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
M Mat2(C a, C b, C c, C d) { return {a, b, c, d}; }

// 4×4 gate from sixteen entries (row-major).
M Mat4(C a00, C a01, C a02, C a03, C a10, C a11, C a12, C a13, C a20, C a21,
       C a22, C a23, C a30, C a31, C a32, C a33) {
  return {a00, a01, a02, a03, a10, a11, a12, a13,
          a20, a21, a22, a23, a30, a31, a32, a33};
}

// (2^n)x(2^n) gate from operations (row-major).
// Builds a 2^n × 2^n identity matrix,
// then exchanges row pairs and patches diagonal entries.
using Swap = std::pair<std::size_t, std::size_t>;
using DiagPatch = std::pair<std::size_t, C>;
M MakeMatrix(std::size_t n, std::initializer_list<Swap> swaps = {},
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

inline const M& MatI() {
  static const M kV = Mat2(1, 0, 0, 1);
  return kV;
}

inline const M& MatH() {
  static const M kV = Mat2(kIS2, kIS2, kIS2, -kIS2);
  return kV;
}

inline const M& MatX() {
  static const M kV = Mat2(0, 1, 1, 0);
  return kV;
}

inline const M& MatY() {
  static const M kV = Mat2(0, kMI, kI, 0);
  return kV;
}

inline const M& MatZ() {
  static const M kV = Mat2(1, 0, 0, -1);
  return kV;
}

inline const M& MatS() {
  static const M kV = Mat2(1, 0, 0, kI);
  return kV;
}

inline const M& MatSdg() {
  static const M kV = Mat2(1, 0, 0, kMI);
  return kV;
}

inline const M& MatT() {
  static const M kV = Mat2(1, 0, 0, C{kIS2, kIS2});
  return kV;
}

inline const M& MatTdg() {
  static const M kV = Mat2(1, 0, 0, C{kIS2, -kIS2});
  return kV;
}

inline const M& MatSx() {
  static const M kV =
      Mat2(C{0.5, 0.5}, C{0.5, -0.5}, C{0.5, -0.5}, C{0.5, 0.5});
  return kV;
}

inline const M& MatSXdg() {
  static const M kV =
      Mat2(C{0.5, -0.5}, C{0.5, 0.5}, C{0.5, 0.5}, C{0.5, -0.5});
  return kV;
}

// ---- fixed two-qubit gates ------------------------------------------------

// CX: |10⟩ ↔ |11⟩
inline const M& MatCx() {
  static const M kV = Mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0);
  return kV;
}

// CZ: phase flip on |11⟩
inline const M& MatCz() {
  static const M kV = Mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1);
  return kV;
}

// CY: |10⟩ ↔ |11⟩ with Y factors
inline const M& MatCy() {
  static const M kV = Mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, kMI, 0, 0, kI, 0);
  return kV;
}

// SWAP: |01⟩ ↔ |10⟩
inline const M& MatSwap() {
  static const M kV = Mat4(1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1);
  return kV;
}

// iSWAP: |01⟩ ↔ i|10⟩
inline const M& MatIswap() {
  static const M kV = Mat4(1, 0, 0, 0, 0, 0, kI, 0, 0, kI, 0, 0, 0, 0, 0, 1);
  return kV;
}

// ---- fixed three-qubit gates ----------------------------------------------

// CCX (Toffoli): |110⟩ ↔ |111⟩  (rows 6 ↔ 7)
inline const M& MatCcx() {
  static const M kV = MakeMatrix(3, {{6, 7}});
  return kV;
}

// CCZ: phase flip on |111⟩  (row 7 diagonal = -1)
inline const M& MatCcz() {
  static const M kV = MakeMatrix(3, {}, {{7, C{-1.0, 0.0}}});
  return kV;
}

// CSWAP (Fredkin): |101⟩ ↔ |110⟩  (rows 5 ↔ 6)
inline const M& MatCswap() {
  static const M kV = MakeMatrix(3, {{5, 6}});
  return kV;
}

}  // namespace

GateRegistry GateRegistry::WithBuiltins() {
  GateRegistry reg;

  // ---- fixed single-qubit -------------------------------------------------
  reg.Add({"id", 1, MatI()});
  reg.Add({"h", 1, MatH()});
  reg.Add({"x", 1, MatX()});
  reg.Add({"y", 1, MatY()});
  reg.Add({"z", 1, MatZ()});
  reg.Add({"s", 1, MatS()});
  reg.Add({"sdg", 1, MatSdg()});
  reg.Add({"t", 1, MatT()});
  reg.Add({"tdg", 1, MatTdg()});
  reg.Add({"sx", 1, MatSx()});
  reg.Add({"sxdg", 1, MatSXdg()});

  // ---- parametric single-qubit --------------------------------------------

  reg.Add({"p", 1, 1,
           [](const P& p) { return Mat2(1, 0, 0, std::exp(kI * p[0])); }});

  // legacy OpenQASM 2.0 alias for phase gate (p)
  reg.Add({"u1", 1, 1,
           [](const P& p) { return Mat2(1, 0, 0, std::exp(kI * p[0])); }});

  reg.Add({"u2", 1, 2, [](const P& p) {
             const double phi = p[0];
             const double lam = p[1];
             return Mat2(kIS2, -std::exp(kI * lam) * kIS2,
                         std::exp(kI * phi) * kIS2,
                         std::exp(kI * (phi + lam)) * kIS2);
           }});

  reg.Add({"U", 1, 3, [](const P& p) {
             const double theta = p[0];
             const double phi = p[1];
             const double lam = p[2];
             const double c = std::cos(theta / 2.0);
             const double s = std::sin(theta / 2.0);
             return Mat2(c, -std::exp(kI * lam) * s, std::exp(kI * phi) * s,
                         std::exp(kI * (phi + lam)) * c);
           }});

  // legacy OpenQASM 2.0 alias for unitary gate (U)
  reg.Add({"u3", 1, 3, [](const P& p) {
             const double theta = p[0];
             const double phi = p[1];
             const double lam = p[2];
             const double c = std::cos(theta / 2.0);
             const double s = std::sin(theta / 2.0);
             return Mat2(c, -std::exp(kI * lam) * s, std::exp(kI * phi) * s,
                         std::exp(kI * (phi + lam)) * c);
           }});

  reg.Add({"rx", 1, 1, [](const P& p) {
             const double c = std::cos(p[0] / 2.0);
             const double s = std::sin(p[0] / 2.0);
             return Mat2(c, kMI * s, kMI * s, c);
           }});

  reg.Add({"ry", 1, 1, [](const P& p) {
             const double c = std::cos(p[0] / 2.0);
             const double s = std::sin(p[0] / 2.0);
             return Mat2(c, -s, s, c);
           }});

  reg.Add({"rz", 1, 1, [](const P& p) {
             const double h = p[0] / 2.0;
             return Mat2(std::exp(kMI * h), 0, 0, std::exp(kI * h));
           }});

  // ---- fixed two-qubit ----------------------------------------------------
  reg.Add({"cx", 2, MatCx()});
  reg.Add({"cz", 2, MatCz()});
  reg.Add({"cy", 2, MatCy()});
  reg.Add({"swap", 2, MatSwap()});
  reg.Add({"iswap", 2, MatIswap()});

  // ---- parametric two-qubit -----------------------------------------------

  reg.Add({"cp", 2, 1, [](const P& p) {
             return Mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0,
                         std::exp(kI * p[0]));
           }});

  reg.Add({"crx", 2, 1, [](const P& p) {
             const double c = std::cos(p[0] / 2.0);
             const double s = std::sin(p[0] / 2.0);
             return Mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, c, kMI * s, 0, 0,
                         kMI * s, c);
           }});

  reg.Add({"cry", 2, 1, [](const P& p) {
             const double c = std::cos(p[0] / 2.0);
             const double s = std::sin(p[0] / 2.0);
             return Mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, c, -s, 0, 0, s, c);
           }});

  reg.Add({"crz", 2, 1, [](const P& p) {
             const double h = p[0] / 2.0;
             return Mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, std::exp(kMI * h), 0, 0,
                         0, 0, std::exp(kI * h));
           }});

  // ---- three-qubit --------------------------------------------------------
  reg.Add({"ccx", 3, MatCcx()});
  reg.Add({"ccz", 3, MatCcz()});
  reg.Add({"cswap", 3, MatCswap()});

  return reg;
}

}  // namespace qde
