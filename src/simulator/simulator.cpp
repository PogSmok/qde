#include "qde/simulator/simulator.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <complex>
#include <execution>
#include <numeric>
#include <random>
#include <stdexcept>

#include "qde/simulator/compiled_operation.hpp"
#include "qde/simulator/simulation_circuit.hpp"

namespace qde {

namespace {

// gate_idx:     index to state of all gate qubits (MSB-first)
// non_gate_idx: index to state of all non-gate qubits (LSB-first)
std::size_t embedIndex(std::size_t gate_idx, std::size_t non_gate_idx,
                       const std::vector<std::size_t>& gate_qubits,
                       std::size_t total_qubits) {
  std::size_t result = 0;

  // Place gate qubits at their qubit positions (MSB-first).
  const std::size_t gate_arity = gate_qubits.size();
  for (std::size_t i = 0; i < gate_arity; i++) {
    if ((gate_idx >> (gate_arity - 1 - i)) & 1) {
      result |= std::size_t{1} << gate_qubits[i];
    }
  }

  // Place non-gate qubits at the remaining positions (LSB-first).
  std::size_t non_gate_pos = 0;
  for (std::size_t qubit_idx = 0; qubit_idx < total_qubits; qubit_idx++) {
    bool is_gate = false;
    for (std::size_t gate_qubit_idx : gate_qubits) {
      if (qubit_idx == gate_qubit_idx) { is_gate = true; break; }
    }
    if (is_gate) continue; // gate qubits are already placed

    if ((non_gate_idx >> non_gate_pos) & 1) {
      result |= std::size_t{1} << qubit_idx;
    }
    non_gate_pos++;
  }

  return result;
}

// Apply unitary gate U (gate_dim × gate_dim) to the given qubits.
// ρ' = U ρ U†
// Implemented as two passes: left-multiply by U, then right-multiply by U†.
void applyGate(DensityMatrix& rho, std::size_t total_qubits,
               const std::vector<std::complex<double>>& U,
               const std::vector<std::size_t>& gate_qubits) {
  const std::size_t gate_arity   = gate_qubits.size();
  const std::size_t gate_dim     = std::size_t{1} << gate_arity;
  const std::size_t dim          = std::size_t{1} << total_qubits;
  const std::size_t non_gate_dim = std::size_t{1} << (total_qubits - gate_arity);

  // Pre-compute embedIndex for all (gate_state, background_state) pairs to
  // avoid recomputing inside the parallel loops.
  std::vector<std::size_t> idx_table(gate_dim * non_gate_dim);
  for (std::size_t i = 0; i < gate_dim; i++)
    for (std::size_t j = 0; j < non_gate_dim; j++)
      idx_table[i * non_gate_dim + j] =
        embedIndex(i, j, gate_qubits, total_qubits);

  // Pass 1: A = U * ρ  (left multiply in gate subspace)
  // row mixing — each column k of A is independent, safe to parallelise.
  DensityMatrix A(dim * dim, {0.0, 0.0});
  std::vector<std::size_t> col_range(dim);
  std::iota(col_range.begin(), col_range.end(), 0);
  std::for_each(std::execution::par_unseq, col_range.begin(), col_range.end(),
      [&](std::size_t k) {
        for (std::size_t i = 0; i < gate_dim; i++) {
          for (std::size_t j = 0; j < non_gate_dim; j++) {
            std::complex<double> sum{0.0, 0.0};
            // for each gate_dim compute dot product of U row i with ρ
            for (std::size_t l = 0; l < gate_dim; l++) {
              sum += U[i * gate_dim + l] *
                     rho[idx_table[l * non_gate_dim + j] * dim + k];
            }
            A[idx_table[i * non_gate_dim + j] * dim + k] = sum;
          }
        }
      });

  // Pass 2: ρ' = A * U†  (right multiply in gate subspace)
  // column mixing — each row k of ρ' is independent, safe to parallelise.
  rho.assign(dim * dim, {0.0, 0.0});
  std::vector<std::size_t> row_range(dim);
  std::iota(row_range.begin(), row_range.end(), 0);
  std::for_each(std::execution::par_unseq, row_range.begin(), row_range.end(),
      [&](std::size_t k) {
        for (std::size_t i = 0; i < gate_dim; i++) {
          for (std::size_t j = 0; j < non_gate_dim; j++) {
            std::complex<double> sum{0.0, 0.0};
            for (std::size_t l = 0; l < gate_dim; l++) {
              sum += A[k * dim + idx_table[l * non_gate_dim + j]] *
                     std::conj(U[i * gate_dim + l]);
            }
            rho[k * dim + idx_table[i * non_gate_dim + j]] = sum;
          }
        }
      });
}

std::uint8_t measure(DensityMatrix& rho, std::size_t total_qubits,
                     std::size_t qubit, std::mt19937& rng) {
  const std::size_t dim = std::size_t{1} << total_qubits;
  const std::size_t qubit_mask = std::size_t{1} << qubit; 

  // Calculate probability of measurement as 0
  double p0 = 0.0;
  for (std::size_t i = 0; i < dim; i++) {
    if ((i & qubit_mask) == 0) p0 += rho[i * dim + i].real();
  }
  p0 = std::clamp(p0, 0.0, 1.0); // guard against floating point drift

  const std::uint8_t measurement = 
      std::uniform_real_distribution<double>(0.0, 1.0)(rng) < p0 ? 0 : 1;
  const double probability = measurement == 0 ? p0 : 1.0 - p0;

  // Collapse quantum states and renormalize
  for (std::size_t i = 0; i < dim; i++) {
    for (std::size_t j = 0; j < dim; j++) {
      // Is basis state consistent with measurement?
      const bool keep = (((i & qubit_mask) >> qubit) == measurement) &&
                        (((j & qubit_mask) >> qubit) == measurement);

      if(keep) rho[i * dim + j] /= probability;
      else rho[i * dim + j] = std::complex<double>{0.0, 0.0};
    }
  }

  return measurement;
}

// Reset qubit to |0⟩ via the Kraus channel.
void reset(DensityMatrix& rho, std::size_t total_qubits, std::size_t qubit) {
  const std::size_t dim = std::size_t{1} << total_qubits;
  const std::size_t qubit_mask = std::size_t{1} << qubit;
  DensityMatrix new_rho(dim * dim, {0.0, 0.0});

  for (std::size_t i = 0; i < dim; i++) {
    if (i & qubit_mask) continue;
    for (std::size_t j = 0; j < dim; j++) {
      if (j & qubit_mask) continue;
      // K0 term: keep existing |0> amplitude; K1 term: fold |1> back into |0>
      new_rho[i * dim + j] = rho[i * dim + j] +
                             rho[(i | qubit_mask) * dim + (j | qubit_mask)];
    }
  }

  rho = std::move(new_rho);
}

std::vector<double> computeEigenvalues(const DensityMatrix& rho,
                                       std::size_t total_qubits) {
  const std::size_t dim = std::size_t{1} << total_qubits;
  std::vector<std::complex<double>> H(rho);
  constexpr int    kMaxSweeps = 100;
  constexpr double kEps       = 1e-10;

  for (int sweep = 0; sweep < kMaxSweeps; sweep++) {
    double max_off = 0.0;
    for (std::size_t p = 0; p < dim; p++)
      for (std::size_t q = p + 1; q < dim; q++)
        max_off = std::max(max_off, std::abs(H[p * dim + q]));
    if (max_off < kEps) break;

    for (std::size_t p = 0; p < dim; p++) {
      for (std::size_t q = p + 1; q < dim; q++) {
        const std::complex<double> Hpq = H[p * dim + q];
        if (std::abs(Hpq) < kEps * kEps) continue;

        const double Hpp = H[p * dim + p].real();
        const double Hqq = H[q * dim + q].real();
        const double tau = (Hqq - Hpp) / (2.0 * std::abs(Hpq));
        const double t   = (tau >= 0 ? 1.0 : -1.0) /
                           (std::abs(tau) + std::sqrt(1.0 + tau * tau));
        const double c     = 1.0 / std::sqrt(1.0 + t * t);
        const double s     = c * t;
        const std::complex<double> phase = Hpq / std::abs(Hpq);

        // Update diagonal.
        H[p * dim + p] = Hpp * c * c + Hqq * s * s - 2.0 * std::abs(Hpq) * s * c;
        H[q * dim + q] = Hpp * s * s + Hqq * c * c + 2.0 * std::abs(Hpq) * s * c;
        H[p * dim + q] = H[q * dim + p] = {0.0, 0.0};

        // Update off-diagonal rows/columns.
        for (std::size_t r = 0; r < dim; r++) {
          if (r == p || r == q) continue;
          const std::complex<double> Hrp = H[r * dim + p];
          const std::complex<double> Hrq = H[r * dim + q];
          H[r * dim + p]  =  c * Hrp + s * std::conj(phase) * Hrq;
          H[r * dim + q]  = -s * phase * Hrp + c * Hrq;
          H[p * dim + r]  = std::conj(H[r * dim + p]);
          H[q * dim + r]  = std::conj(H[r * dim + q]);
        }
      }
    }
  }

  std::vector<double> vals(dim);
  for (std::size_t i = 0; i < dim; ++i) vals[i] = H[i * dim + i].real();
  std::sort(vals.begin(), vals.end(), std::greater<double>());
  return vals;
}

SimulationState makeSnapshot(const DensityMatrix& rho, std::size_t total_qubits,
                             const std::vector<std::uint8_t>& classical_bits,
                             std::size_t layer, double gate_fidelity) {
  const std::size_t dim = std::size_t{1} << total_qubits;

  SimulationState s;
  s.layer          = layer;
  s.qubit_count    = total_qubits;
  s.bit_count      = classical_bits.size();
  s.density_matrix = rho;
  s.classical_bits = classical_bits;
  s.gate_fidelity  = gate_fidelity;

  s.basis_probabilities.resize(dim);
  for (std::size_t i = 0; i < dim; ++i)
    s.basis_probabilities[i] = rho[i * dim + i].real();

  s.eigenvalues = computeEigenvalues(rho, total_qubits);
  return s;
}

std::vector<SimulationState> simulate(const SimulationCircuit& circuit,
                                      bool save_all) {
  const std::size_t n   = circuit.qubitCount();
  const std::size_t dim = std::size_t{1} << n;

  // Initial state ρ = |0...0><0...0|
  DensityMatrix rho(dim * dim, {0.0, 0.0});
  rho[0] = {1.0, 0.0};

  // All bits initially 0
  std::vector<std::uint8_t> classical_bits(circuit.bitCount(), 0);
  std::mt19937 rng{std::random_device{}()};

  std::vector<SimulationState> states;
  if (save_all) {
    states.reserve(circuit.layers().size() + 1);
    states.push_back(makeSnapshot(rho, n, classical_bits, 0, 1.0));
  }

  for (std::size_t layer_idx = 0; layer_idx < circuit.layers().size();
       layer_idx++) {
    for (const CompiledOperation& op : circuit.layers()[layer_idx]) {
      if (op.condition) {
        const auto [bit_idx, expected] = *op.condition;
        if (classical_bits[bit_idx] != expected) continue;
      }

      switch (op.type) {
        case OperationType::kGate:
          if (op.gate)
            applyGate(rho, n, op.gate->matrix(op.gate_params), op.qubits);
          break;
        case OperationType::kMeasure:
          for (std::size_t qi = 0; qi < op.qubits.size(); qi++) {
            const std::size_t bit_idx =
                qi < op.measure_target.size() ? op.measure_target[qi] : 0;
            classical_bits[bit_idx] = measure(rho, n, op.qubits[qi], rng);
          }
          break;
        case OperationType::kReset:
          for (const std::size_t q : op.qubits) reset(rho, n, q);
          break;
        case OperationType::kBarrier:
          break;
      }
    }

    if (save_all) {
      states.push_back(makeSnapshot(rho, n, classical_bits, layer_idx+1, 1.0));
    }
  }

  if (!save_all) {
    states.push_back(
        makeSnapshot(rho, n, classical_bits, circuit.layers().size(), 1.0));
  }

  return states;
}

}  // namespace 

// ---- Simulator public API -------------------------------------------------

void Simulator::setNoiseModel(std::shared_ptr<const NoiseModel> model) {
  noise_model_ = std::move(model);
}

std::vector<SimulationState> Simulator::run(
    const SimulationCircuit& circuit) const {
  return simulate(circuit, true);
}

SimulationState Simulator::runFinal(const SimulationCircuit& circuit) const {
  return simulate(circuit, false).back();
}

}  // namespace qde