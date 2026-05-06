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
  const std::size_t kDim = std::size_t{1} << n;
  M m(kDim * kDim, C{0.0, 0.0});
  for (std::size_t i = 0; i < kDim; ++i) {
    m[(i * kDim) + i] = C{1.0, 0.0};
  }

  for (auto [a, b] : swaps) {
    m[(a * kDim) + a] = C{0.0, 0.0};
    m[(b * kDim) + b] = C{0.0, 0.0};
    m[(a * kDim) + b] = C{1.0, 0.0};
    m[(b * kDim) + a] = C{1.0, 0.0};
  }

  for (auto [i, v] : diag_patches) {
    m[(i * kDim) + i] = v;
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

GateRegistry GateRegistry::withBuiltins() {
  GateRegistry reg;

  // ---- fixed single-qubit -------------------------------------------------
  reg.add({"id", 1, MatI()});
  reg.add({"h", 1, MatH()});
  reg.add({"x", 1, MatX()});
  reg.add({"y", 1, MatY()});
  reg.add({"z", 1, MatZ()});
  reg.add({"s", 1, MatS()});
  reg.add({"sdg", 1, MatSdg()});
  reg.add({"t", 1, MatT()});
  reg.add({"tdg", 1, MatTdg()});
  reg.add({"sx", 1, MatSx()});
  reg.add({"sxdg", 1, MatSXdg()});

  // ---- parametric single-qubit --------------------------------------------

  reg.add({"p", 1, 1,
           [](const P& p) { return Mat2(1, 0, 0, std::exp(kI * p[0])); }});

  // legacy OpenQASM 2.0 alias for phase gate (p)
  reg.add({"u1", 1, 1,
           [](const P& p) { return Mat2(1, 0, 0, std::exp(kI * p[0])); }});

  reg.add({"u2", 1, 2, [](const P& p) {
             const double kPhi = p[0];
             const double kLam = p[1];
             return Mat2(kIS2, -std::exp(kI * kLam) * kIS2,
                         std::exp(kI * kPhi) * kIS2,
                         std::exp(kI * (kPhi + kLam)) * kIS2);
           }});

  reg.add({"U", 1, 3, [](const P& p) {
             const double kTheta = p[0];
             const double kPhi = p[1];
             const double kLam = p[2];
             const double kC = std::cos(kTheta / 2.0);
             const double kS = std::sin(kTheta / 2.0);
             return Mat2(kC, -std::exp(kI * kLam) * kS,
                         std::exp(kI * kPhi) * kS,
                         std::exp(kI * (kPhi + kLam)) * kC);
           }});

  // legacy OpenQASM 2.0 alias for unitary gate (U)
  reg.add({"u3", 1, 3, [](const P& p) {
             const double kTheta = p[0];
             const double kPhi = p[1];
             const double kLam = p[2];
             const double kC = std::cos(kTheta / 2.0);
             const double kS = std::sin(kTheta / 2.0);
             return Mat2(kC, -std::exp(kI * kLam) * kS,
                         std::exp(kI * kPhi) * kS,
                         std::exp(kI * (kPhi + kLam)) * kC);
           }});

  reg.add({"rx", 1, 1, [](const P& p) {
             const double kC = std::cos(p[0] / 2.0);
             const double kS = std::sin(p[0] / 2.0);
             return Mat2(kC, kMI * kS, kMI * kS, kC);
           }});

  reg.add({"ry", 1, 1, [](const P& p) {
             const double kC = std::cos(p[0] / 2.0);
             const double kS = std::sin(p[0] / 2.0);
             return Mat2(kC, -kS, kS, kC);
           }});

  reg.add({"rz", 1, 1, [](const P& p) {
             const double kH = p[0] / 2.0;
             return Mat2(std::exp(kMI * kH), 0, 0, std::exp(kI * kH));
           }});

  // ---- fixed two-qubit ----------------------------------------------------
  reg.add({"cx", 2, MatCx()});
  reg.add({"cz", 2, MatCz()});
  reg.add({"cy", 2, MatCy()});
  reg.add({"swap", 2, MatSwap()});
  reg.add({"iswap", 2, MatIswap()});

  // ---- parametric two-qubit -----------------------------------------------

  reg.add({"cp", 2, 1, [](const P& p) {
             return Mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0,
                         std::exp(kI * p[0]));
           }});

  reg.add({"crx", 2, 1, [](const P& p) {
             const double kC = std::cos(p[0] / 2.0);
             const double kS = std::sin(p[0] / 2.0);
             return Mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, kC, kMI * kS, 0, 0,
                         kMI * kS, kC);
           }});

  reg.add({"cry", 2, 1, [](const P& p) {
             const double kC = std::cos(p[0] / 2.0);
             const double kS = std::sin(p[0] / 2.0);
             return Mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, kC, -kS, 0, 0, kS, kC);
           }});

  reg.add({"crz", 2, 1, [](const P& p) {
             const double kH = p[0] / 2.0;
             return Mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, std::exp(kMI * kH), 0, 0,
                         0, 0, std::exp(kI * kH));
           }});

  // ---- three-qubit --------------------------------------------------------
  reg.add({"ccx", 3, MatCcx()});
  reg.add({"ccz", 3, MatCcz()});
  reg.add({"cswap", 3, MatCswap()});

  return reg;
}

}  // namespace qde
