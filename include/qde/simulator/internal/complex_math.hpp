#ifndef SIMULATOR_INTERNAL_COMPLEX_MATH_HPP_
#define SIMULATOR_INTERNAL_COMPLEX_MATH_HPP_

#include <complex>

namespace qde::simulator::internal {

using Amplitude = std::complex<double>;

// Complex product without the NaN/Inf recovery of operator*, which keeps the
// compiler from inlining and vectorising the hot loops. The result differs
// from operator* only when an operand is not finite.
[[nodiscard]] inline Amplitude Mul(const Amplitude& a,
                                   const Amplitude& b) noexcept {
  return {(a.real() * b.real()) - (a.imag() * b.imag()),
          (a.real() * b.imag()) + (a.imag() * b.real())};
}

}  // namespace qde::simulator::internal

#endif  // SIMULATOR_INTERNAL_COMPLEX_MATH_HPP_
