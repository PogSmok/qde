#include "qde/simulator/internal/quantum_state.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

#include "qde/simulator/internal/complex_math.hpp"
#include "qde/simulator/internal/memory_guard.hpp"

namespace qde::simulator::internal {

namespace {

// Loops over fewer elements than this run serially
// Threading overhead dominates for smaller loops.
constexpr std::size_t kParallelThreshold = std::size_t{1} << 14;

// Largest Gram determinant ⟨a|a⟩⟨b|b⟩ - |⟨a|b⟩|² still treated as zero.
constexpr double kEntanglementEps = 1e-12;

constexpr Amplitude kZero{0.0, 0.0};

// Probabilities of finding one qubit in |0> and in |1>.
struct OutcomeProbabilities {
  double zero{0.0};
  double one{0.0};
};

// The split ψ = |0>⊗a + |1>⊗b of a pure state by one of its qubits.
struct QubitSplit {
  double norm_a{0.0};      // <a|a>, the probability of |0>
  double norm_b{0.0};      // <b|b>, the probability of |1>
  double overlap_sq{0.0};  // |<a|b>|²
};

// Opens a zero bit at position `bit` of `index`, shifting the higher bits up.
constexpr std::size_t InsertZeroBit(std::size_t index,
                                    std::size_t bit) noexcept {
  const std::size_t low_mask = (std::size_t{1} << bit) - 1;
  return ((index >> bit) << (bit + 1)) | (index & low_mask);
}

// Value (0 or 1) of bit `bit` of `index`.
constexpr std::size_t BitOf(std::size_t index, std::size_t bit) noexcept {
  return (index >> bit) & 1U;
}

// ---- Gates ------------------------------------------------------------------

// Throws unless `matrix` is a gate on `qubits` of a register of `qubit_count`
// qubits.
void CheckGate(const std::vector<Amplitude>& matrix,
               const std::vector<std::size_t>& qubits,
               std::size_t qubit_count) {
  const std::size_t arity = qubits.size();
  if (arity == 0 || arity > qubit_count) {
    throw std::invalid_argument(
        "QuantumState: gate must act on between 1 and all qubits");
  }

  std::vector<std::size_t> sorted_qubits(qubits);
  std::sort(sorted_qubits.begin(), sorted_qubits.end());
  if (sorted_qubits.back() >= qubit_count ||
      std::adjacent_find(sorted_qubits.begin(), sorted_qubits.end()) !=
          sorted_qubits.end()) {
    throw std::invalid_argument(
        "QuantumState: gate qubits must be distinct and in range");
  }

  const std::size_t gate_dim = std::size_t{1} << arity;
  if (matrix.size() % gate_dim != 0 || matrix.size() / gate_dim != gate_dim) {
    throw std::invalid_argument(
        "QuantumState: gate matrix does not match the gate's qubit count");
  }
}

// offsets[r] spreads the bits of gate index r onto the target bits, with
// targets[0] taking the most significant bit of r.
std::vector<std::size_t> SpreadOffsets(
    const std::vector<std::size_t>& targets) {
  const std::size_t arity = targets.size();
  std::vector<std::size_t> offsets(std::size_t{1} << arity, 0);
  for (std::size_t r = 0; r < offsets.size(); r++) {
    for (std::size_t i = 0; i < arity; i++) {
      if (BitOf(r, arity - 1 - i) != 0U) {
        offsets[r] |= std::size_t{1} << targets[i];
      }
    }
  }
  return offsets;
}

// Applies the 2x2 matrix u to index bit `target` of `state`.
void ApplySingleBitMatrix(std::vector<Amplitude>& state,
                          const std::vector<Amplitude>& u, std::size_t target) {
  const bool parallel = state.size() >= kParallelThreshold;
  const auto pairs = static_cast<std::int64_t>(state.size() >> 1);
  const std::size_t bit = std::size_t{1} << target;
  const Amplitude u00 = u[0];
  const Amplitude u01 = u[1];
  const Amplitude u10 = u[2];
  const Amplitude u11 = u[3];

#pragma omp parallel for schedule(static) if (parallel)
  for (std::int64_t p = 0; p < pairs; p++) {
    const std::size_t i0 = InsertZeroBit(static_cast<std::size_t>(p), target);
    const std::size_t i1 = i0 | bit;
    const Amplitude a0 = state[i0];
    const Amplitude a1 = state[i1];
    state[i0] = Mul(u00, a0) + Mul(u01, a1);
    state[i1] = Mul(u10, a0) + Mul(u11, a1);
  }
}

// Applies the 2^k × 2^k matrix u to k bits of the index of `state`.
// targets[i] is the index bit driven by gate qubit i; gate qubit 0 is the most
// significant bit of the row/column index of u. Targets must be distinct.
void ApplyMatrix(std::vector<Amplitude>& state, const std::vector<Amplitude>& u,
                 const std::vector<std::size_t>& targets) {
  const std::size_t arity = targets.size();
  if (arity == 1) {
    ApplySingleBitMatrix(state, u, targets[0]);
    return;
  }

  const std::size_t gate_dim = std::size_t{1} << arity;
  const bool parallel = state.size() >= kParallelThreshold;
  // Each group is the set of 2^k amplitudes that differ only in target bits.
  const auto groups = static_cast<std::int64_t>(state.size() >> arity);
  const std::vector<std::size_t> offsets(SpreadOffsets(targets));

  std::vector<std::size_t> sorted_targets(targets);
  std::sort(sorted_targets.begin(), sorted_targets.end());

  // Inputs of the group being transformed.
  std::vector<Amplitude> inputs(gate_dim);

#pragma omp parallel for schedule(static) firstprivate(inputs) if (parallel)
  for (std::int64_t g = 0; g < groups; g++) {
    // Open a zero bit at every target position, lowest first.
    auto base = static_cast<std::size_t>(g);
    for (const std::size_t target : sorted_targets) {
      base = InsertZeroBit(base, target);
    }

    for (std::size_t r = 0; r < gate_dim; r++) {
      inputs[r] = state[base | offsets[r]];
    }
    for (std::size_t r = 0; r < gate_dim; r++) {
      const std::size_t row = r * gate_dim;
      Amplitude sum = kZero;
      for (std::size_t c = 0; c < gate_dim; c++) {
        sum += Mul(u[row + c], inputs[c]);
      }
      state[base | offsets[r]] = sum;
    }
  }
}

// ---- Measurement ------------------------------------------------------------

// Probabilities of both outcomes of measuring `qubit`. `data` is ψ if `pure`
// and the row-major dim × dim ρ otherwise.
OutcomeProbabilities MeasureProbabilities(const std::vector<Amplitude>& data,
                                          std::size_t dim, bool pure,
                                          std::size_t qubit) {
  const bool parallel = dim >= kParallelThreshold;
  double p0 = 0.0;
  double p1 = 0.0;

#pragma omp parallel for schedule(static) reduction(+ : p0, p1) if (parallel)
  for (std::int64_t k = 0; k < static_cast<std::int64_t>(dim); k++) {
    const auto i = static_cast<std::size_t>(k);
    const double p = pure ? std::norm(data[i]) : data[(i * dim) + i].real();
    if (BitOf(i, qubit) == 0U) {
      p0 += p;
    } else {
      p1 += p;
    }
  }
  // Guard against floating point drift.
  return {std::max(p0, 0.0), std::max(p1, 0.0)};
}

// The outcome that `sample`, uniform in [0, 1), selects.
std::size_t SelectOutcome(const OutcomeProbabilities& p, double sample) {
  const double total = p.zero + p.one;
  if (!(total > 0.0)) {
    throw std::runtime_error("QuantumState: cannot measure a zero-norm state");
  }
  if (sample * total < p.zero) {
    return 0;
  }
  // An outcome of zero probability can only be selected through rounding.
  return p.one > 0.0 ? 1 : 0;
}

// Projects ψ onto `qubit` = `outcome`, which has the given probability, and
// renormalises.
void CollapsePure(std::vector<Amplitude>& psi, std::size_t qubit,
                  std::size_t outcome, double probability) {
  const bool parallel = psi.size() >= kParallelThreshold;
  const double scale = 1.0 / std::sqrt(probability);

#pragma omp parallel for schedule(static) if (parallel)
  for (std::int64_t k = 0; k < static_cast<std::int64_t>(psi.size()); k++) {
    const auto i = static_cast<std::size_t>(k);
    psi[i] = BitOf(i, qubit) == outcome ? psi[i] * scale : kZero;
  }
}

// Projects the dim × dim ρ onto `qubit` = `outcome`, which has the given
// probability, and renormalises.
void CollapseMixed(std::vector<Amplitude>& rho, std::size_t dim,
                   std::size_t qubit, std::size_t outcome, double probability) {
  const bool parallel = rho.size() >= kParallelThreshold;
  const double scale = 1.0 / probability;

#pragma omp parallel for schedule(static) if (parallel)
  for (std::int64_t k = 0; k < static_cast<std::int64_t>(dim); k++) {
    const auto i = static_cast<std::size_t>(k);
    const std::size_t row = i * dim;
    const bool keep_row = BitOf(i, qubit) == outcome;
    for (std::size_t j = 0; j < dim; j++) {
      const bool keep = keep_row && BitOf(j, qubit) == outcome;
      rho[row + j] = keep ? rho[row + j] * scale : kZero;
    }
  }
}

// ---- Reset ------------------------------------------------------------------

QubitSplit SplitByQubit(const std::vector<Amplitude>& psi, std::size_t qubit) {
  const std::size_t bit = std::size_t{1} << qubit;
  const bool parallel = psi.size() >= kParallelThreshold;
  const auto pairs = static_cast<std::int64_t>(psi.size() >> 1);
  double norm_a = 0.0;
  double norm_b = 0.0;
  double overlap_re = 0.0;
  double overlap_im = 0.0;

#pragma omp parallel for schedule(static) \
    reduction(+ : norm_a, norm_b, overlap_re, overlap_im) if (parallel)
  for (std::int64_t p = 0; p < pairs; p++) {
    const std::size_t i0 = InsertZeroBit(static_cast<std::size_t>(p), qubit);
    const Amplitude a = psi[i0];
    const Amplitude b = psi[i0 | bit];
    norm_a += std::norm(a);
    norm_b += std::norm(b);
    overlap_re += (a.real() * b.real()) + (a.imag() * b.imag());
    overlap_im += (a.real() * b.imag()) - (a.imag() * b.real());
  }
  return {norm_a, norm_b,
          (overlap_re * overlap_re) + (overlap_im * overlap_im)};
}

// Replaces ψ = |0>⊗a + |1>⊗b by |0>⊗(scale * a), or by |0>⊗(scale * b) if
// `keep_one`.
void ProjectOntoZero(std::vector<Amplitude>& psi, std::size_t qubit,
                     bool keep_one, double scale) {
  const std::size_t bit = std::size_t{1} << qubit;
  const bool parallel = psi.size() >= kParallelThreshold;
  const auto pairs = static_cast<std::int64_t>(psi.size() >> 1);

#pragma omp parallel for schedule(static) if (parallel)
  for (std::int64_t p = 0; p < pairs; p++) {
    const std::size_t i0 = InsertZeroBit(static_cast<std::size_t>(p), qubit);
    const std::size_t i1 = i0 | bit;
    psi[i0] = (keep_one ? psi[i1] : psi[i0]) * scale;
    psi[i1] = kZero;
  }
}

}  // namespace

QuantumState::QuantumState(std::size_t qubit_count)
    : qubit_count_{qubit_count} {
  if (qubit_count_ > kMaxIndexBits) {
    throw std::length_error("QuantumState: too many qubits to index");
  }
  data_.assign(Dim(), kZero);
  data_[0] = {1.0, 0.0};
}

void QuantumState::CheckQubit(std::size_t qubit) const {
  if (qubit >= qubit_count_) {
    throw std::out_of_range("QuantumState: qubit index out of range");
  }
}

std::vector<double> QuantumState::Probabilities() const {
  const std::size_t dim = Dim();
  const bool parallel = dim >= kParallelThreshold;
  const bool pure = pure_;
  const std::vector<Amplitude>& data = data_;
  std::vector<double> probabilities(dim);

#pragma omp parallel for schedule(static) if (parallel)
  for (std::int64_t k = 0; k < static_cast<std::int64_t>(dim); k++) {
    const auto i = static_cast<std::size_t>(k);
    probabilities[i] = pure ? std::norm(data[i]) : data[(i * dim) + i].real();
  }
  return probabilities;
}

void QuantumState::ApplyGate(const std::vector<Amplitude>& matrix,
                             const std::vector<std::size_t>& qubits) {
  CheckGate(matrix, qubits, qubit_count_);
  if (pure_) {
    ApplyMatrix(data_, matrix, qubits);
    return;
  }

  // ρ is indexed by row * dim + column, so it is a vector over 2n bits: row
  // qubit q is bit q + n and column qubit q is bit q. UρU† applies U to the
  // row bits and the complex conjugate of U to the column bits.
  std::vector<std::size_t> row_bits(qubits);
  for (std::size_t& bit : row_bits) {
    bit += qubit_count_;
  }
  ApplyMatrix(data_, matrix, row_bits);

  std::vector<Amplitude> conjugate(matrix);
  for (Amplitude& element : conjugate) {
    element = std::conj(element);
  }
  ApplyMatrix(data_, conjugate, qubits);
}

std::uint8_t QuantumState::Measure(std::size_t qubit, double sample) {
  CheckQubit(qubit);
  const OutcomeProbabilities p =
      MeasureProbabilities(data_, Dim(), pure_, qubit);
  const std::size_t outcome = SelectOutcome(p, sample);
  const double probability = outcome == 0 ? p.zero : p.one;

  if (pure_) {
    CollapsePure(data_, qubit, outcome, probability);
  } else {
    CollapseMixed(data_, Dim(), qubit, outcome, probability);
  }
  return static_cast<std::uint8_t>(outcome);
}

bool QuantumState::TryReset(std::size_t qubit) {
  CheckQubit(qubit);
  if (pure_) {
    return TryResetPure(qubit);
  }
  ResetMixed(qubit);
  return true;
}

bool QuantumState::TryResetPure(std::size_t qubit) {
  // Split ψ = |0>⊗a + |1>⊗b. The qubit is unentangled iff a and b are
  // parallel, i.e. iff Cauchy-Schwarz |<a|b>|² <= <a|a><b|b> is an equality.
  const QubitSplit split = SplitByQubit(data_, qubit);
  if ((split.norm_a * split.norm_b) - split.overlap_sq > kEntanglementEps) {
    return false;
  }

  // The rest of the register is in the state a/|a| = b/|b| (up to a phase), so
  // keep whichever branch is larger for numerical accuracy.
  const bool keep_one = split.norm_b > split.norm_a;
  const double kept = keep_one ? split.norm_b : split.norm_a;
  if (!(kept > 0.0)) {
    throw std::runtime_error("QuantumState: cannot reset a zero-norm state");
  }
  ProjectOntoZero(data_, qubit, keep_one,
                  std::sqrt((split.norm_a + split.norm_b) / kept));
  return true;
}

void QuantumState::ResetMixed(std::size_t qubit) {
  std::vector<Amplitude>& rho = data_;
  const std::size_t dim = Dim();
  const std::size_t bit = std::size_t{1} << qubit;
  const bool parallel = rho.size() >= kParallelThreshold;
  const std::size_t half = dim >> 1;

  // Kraus channel: ρ splits into four blocks by the value of the qubit in the
  // row and in the column index. K0 keeps the |0><0| block, K1 folds the
  // |1><1| block onto it, and the other blocks vanish. Each iteration owns
  // one pair of rows, so the iterations are independent.
#pragma omp parallel for schedule(static) if (parallel)
  for (std::int64_t p = 0; p < static_cast<std::int64_t>(half); p++) {
    const std::size_t i0 = InsertZeroBit(static_cast<std::size_t>(p), qubit);
    const std::size_t row0 = i0 * dim;
    const std::size_t row1 = (i0 | bit) * dim;
    for (std::size_t q = 0; q < half; q++) {
      const std::size_t j0 = InsertZeroBit(q, qubit);
      const std::size_t j1 = j0 | bit;
      rho[row0 + j0] += rho[row1 + j1];
      rho[row0 + j1] = kZero;
      rho[row1 + j0] = kZero;
      rho[row1 + j1] = kZero;
    }
  }
}

void QuantumState::ConvertToDensityMatrix() {
  if (!pure_) {
    return;
  }
  if (2 * qubit_count_ > kMaxIndexBits) {
    throw std::length_error(
        "QuantumState: too many qubits to index a density matrix");
  }
  const std::size_t dim = Dim();
  const std::vector<Amplitude>& psi = data_;
  std::vector<Amplitude> rho(dim * dim);
  const bool parallel = rho.size() >= kParallelThreshold;

#pragma omp parallel for schedule(static) if (parallel)
  for (std::int64_t k = 0; k < static_cast<std::int64_t>(dim); k++) {
    const auto i = static_cast<std::size_t>(k);
    const std::size_t row = i * dim;
    for (std::size_t j = 0; j < dim; j++) {
      rho[row + j] = Mul(psi[i], std::conj(psi[j]));
    }
  }

  data_ = std::move(rho);
  pure_ = false;
}

}  // namespace qde::simulator::internal
