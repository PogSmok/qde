#include "qde/simulator/internal/eigenvalues.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include "qde/simulator/internal/complex_math.hpp"

namespace qde::simulator::internal {

namespace {

// Trailing blocks with fewer elements than this are reduced serially.
// Threading overhead dominates for smaller blocks.
constexpr std::size_t kParallelThreshold = std::size_t{1} << 14;

// A real symmetric tridiagonal matrix: `diagonal[k]` is element (k, k) and
// `coupling[k]` is element (k, k + 1).
struct Tridiagonal {
  std::vector<double> diagonal;
  std::vector<double> coupling;
};

// Euclidean norm of column k of h below the diagonal.
double SubdiagonalNorm(const std::vector<Amplitude>& h, std::size_t dim,
                       std::size_t k) {
  double norm_sq = 0.0;
  for (std::size_t i = k + 1; i < dim; i++) {
    norm_sq += std::norm(h[(i * dim) + k]);
  }
  return std::sqrt(norm_sq);
}

// Fills v[k + 1, dim) with the unit Householder vector whose reflector maps
// column k of h below the diagonal onto its first entry. `norm` is the
// non-zero SubdiagonalNorm of that column.
void BuildReflector(const std::vector<Amplitude>& h, std::size_t dim,
                    std::size_t k, double norm, std::vector<Amplitude>& v) {
  const Amplitude head = h[((k + 1) * dim) + k];
  const double head_abs = std::abs(head);
  const Amplitude phase{head_abs > 0.0 ? head / head_abs : Amplitude{1.0, 0.0}};
  const double v_norm = std::sqrt(2.0 * norm * (norm + head_abs));

  for (std::size_t i = k + 1; i < dim; i++) {
    v[i] = h[(i * dim) + k];
  }
  v[k + 1] += phase * norm;
  for (std::size_t i = k + 1; i < dim; i++) {
    v[i] /= v_norm;
  }
}

// Applies the similarity transform (I - 2vv†) A (I - 2vv†) to the trailing
// block A = h[k + 1, dim)². With w = Av - (v†Av)v this is the rank-two update
// A - 2(vw† + wv†). `w` is scratch space of dim elements.
void ApplyReflector(std::vector<Amplitude>& h, std::size_t dim, std::size_t k,
                    const std::vector<Amplitude>& v,
                    std::vector<Amplitude>& w) {
  const auto first = static_cast<std::int64_t>(k + 1);
  const auto last = static_cast<std::int64_t>(dim);
  const std::size_t block = dim - k - 1;
  const bool parallel = block * block >= kParallelThreshold;

  double kappa = 0.0;
#pragma omp parallel for schedule(static) reduction(+ : kappa) if (parallel)
  for (std::int64_t r = first; r < last; r++) {
    const auto i = static_cast<std::size_t>(r);
    const std::size_t row = i * dim;
    Amplitude sum{0.0, 0.0};
    for (std::size_t j = k + 1; j < dim; j++) {
      sum += Mul(h[row + j], v[j]);
    }
    w[i] = sum;
    kappa += Mul(std::conj(v[i]), sum).real();
  }
  for (std::size_t i = k + 1; i < dim; i++) {
    w[i] -= kappa * v[i];
  }

#pragma omp parallel for schedule(static) if (parallel)
  for (std::int64_t r = first; r < last; r++) {
    const auto i = static_cast<std::size_t>(r);
    const std::size_t row = i * dim;
    for (std::size_t j = k + 1; j < dim; j++) {
      h[row + j] -=
          2.0 * (Mul(v[i], std::conj(w[j])) + Mul(w[i], std::conj(v[j])));
    }
  }
}

// Householder tridiagonalisation of the Hermitian matrix h. The phase of each
// off-diagonal element can be dropped because a Hermitian tridiagonal matrix
// is unitarily similar to the real one built from the magnitudes.
Tridiagonal Tridiagonalize(std::vector<Amplitude> h, std::size_t dim) {
  Tridiagonal result{std::vector<double>(dim), std::vector<double>(dim, 0.0)};
  std::vector<Amplitude> v(dim);
  std::vector<Amplitude> w(dim);

  for (std::size_t k = 0; k < dim; k++) {
    result.diagonal[k] = h[(k * dim) + k].real();

    const double norm = SubdiagonalNorm(h, dim, k);
    result.coupling[k] = norm;
    if (k + 2 >= dim || norm == 0.0) {
      continue;  // column is already tridiagonal
    }
    BuildReflector(h, dim, k, norm, v);
    ApplyReflector(h, dim, k, v, w);
  }
  return result;
}

// One implicit QL sweep with Wilkinson shift over the unreduced block [l, m]
// of the tridiagonal matrix (d, e).
void QlSweep(std::vector<double>& d, std::vector<double>& e, std::size_t l,
             std::size_t m) {
  double g = (d[l + 1] - d[l]) / (2.0 * e[l]);
  double r = std::hypot(g, 1.0);
  g = d[m] - d[l] + (e[l] / (g + std::copysign(r, g)));
  double s = 1.0;
  double c = 1.0;
  double p = 0.0;

  for (std::size_t i = m; i-- > l;) {
    const double f = s * e[i];
    const double b = c * e[i];
    r = std::hypot(f, g);
    e[i + 1] = r;
    if (r == 0.0) {
      // The block split early; restart on the smaller one.
      d[i + 1] -= p;
      e[m] = 0.0;
      return;
    }
    s = f / r;
    c = g / r;
    g = d[i + 1] - p;
    r = ((d[i] - g) * s) + (2.0 * c * b);
    p = s * r;
    d[i + 1] = g + p;
    g = (c * r) - b;
  }
  d[l] -= p;
  e[l] = g;
  e[m] = 0.0;
}

// Implicit QL iteration. On return `diagonal` holds the eigenvalues in no
// particular order.
void DiagonalizeInPlace(Tridiagonal& tridiagonal) {
  constexpr int max_iterations = 100;
  std::vector<double>& d = tridiagonal.diagonal;
  std::vector<double>& e = tridiagonal.coupling;
  const std::size_t dim = d.size();

  double scale = 0.0;
  for (std::size_t i = 0; i < dim; i++) {
    scale = std::max(scale, std::abs(d[i]) + e[i]);
  }
  const double tolerance = std::numeric_limits<double>::epsilon() * scale;

  for (std::size_t l = 0; l < dim; l++) {
    for (int iteration = 0; iteration < max_iterations; iteration++) {
      // Find the first negligible coupling at or after l.
      std::size_t m = l;
      while (m + 1 < dim && std::abs(e[m]) > tolerance) {
        m++;
      }
      if (m == l) {
        break;  // d[l] has converged
      }
      QlSweep(d, e, l, m);
    }
  }
}

}  // namespace

std::vector<double> HermitianEigenvalues(
    const std::vector<std::complex<double>>& matrix, std::size_t dim) {
  if (dim == 0 ? !matrix.empty()
               : (matrix.size() % dim != 0 || matrix.size() / dim != dim)) {
    throw std::invalid_argument(
        "HermitianEigenvalues: matrix must have dim * dim elements");
  }

  Tridiagonal tridiagonal = Tridiagonalize(matrix, dim);
  DiagonalizeInPlace(tridiagonal);
  std::sort(tridiagonal.diagonal.begin(), tridiagonal.diagonal.end(),
            std::greater<>());
  return std::move(tridiagonal.diagonal);
}

}  // namespace qde::simulator::internal
