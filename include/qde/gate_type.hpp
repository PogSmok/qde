#ifndef QDE_GATE_TYPE_HPP_
#define QDE_GATE_TYPE_HPP_

#include <cstdint>

namespace qde {

enum class GateType : uint8_t {
  // Application specific utility
  kEmpty = 0,         // UI placeholder, ignored by the math engine

  // Universal primitives
  kU,                 // Universal single-qubit rotation U(theta, phi, lambda)
  kGphase,            // Global phase shift

  // Parametric rotations
  kRx, kRy, kRz,      // Rotations around specific axes
  kP,                 // Phase shift gate P(lambda)

  // Standard single qubit gates
  kId,                // Identity gate
  kX, kY, kZ,         // Pauli gates
  kH,                 // Hadamard
  kS, kSdg,           // Phase gate (pi/2) and its adjoint
  kT, kTdg,           // pi/8 gate and its adjoint
  kSx, kSxdg,         // Square root of X and its adjoint

  // Standard two qubit gates
  kCx,                // Controlled-NOT (CNOT)
  kCy, kCz,           // Controlled-Y and Controlled-Z
  kCh,                // Controlled-Hadamard
  kCp,                // Controlled-Phase
  kCrx, kCry,  kCrz,  // Controlled-Rotations
  kSwap,              // Swap two qubits
  kIswap,             // iSWAP

  // Standard three qubit gates
  kCcx,               // Toffoli (Controlled-Controlled-NOT)
  kCswap,             // Fredkin (Controlled-SWAP)
  kMct,               // Multi-Controlled Toffoli (N-controls)

  // Meta operations
  kMeasure,           // Measurement in Z-basis
  kReset,             // Force qubit to |0>
  kBarrier,           // Optimization barrier
  kDelay,             // Timing delay

  // Custom gate
  kOpaque             // Operation matrix stored in Gate object
};

} // namespace qde

#endif // QDE_GATE_TYPE_HPP_