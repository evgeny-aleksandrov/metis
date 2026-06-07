#include "metis/model/ridge_regression.hpp"

#include <cmath>
#include <stdexcept>

namespace metis {
namespace {

std::vector<double> solve_linear_system(std::vector<std::vector<double>> matrix, std::vector<double> rhs) {
  const size_t n = rhs.size();
  for (size_t pivot = 0; pivot < n; ++pivot) {
    size_t best = pivot;
    for (size_t row = pivot + 1; row < n; ++row) {
      if (std::abs(matrix[row][pivot]) > std::abs(matrix[best][pivot])) {
        best = row;
      }
    }
    if (std::abs(matrix[best][pivot]) < 1e-12) {
      throw std::runtime_error("Ridge regression system is singular.");
    }
    if (best != pivot) {
      std::swap(matrix[best], matrix[pivot]);
      std::swap(rhs[best], rhs[pivot]);
    }
    const double divisor = matrix[pivot][pivot];
    for (size_t col = pivot; col < n; ++col) {
      matrix[pivot][col] /= divisor;
    }
    rhs[pivot] /= divisor;

    for (size_t row = 0; row < n; ++row) {
      if (row == pivot) {
        continue;
      }
      const double factor = matrix[row][pivot];
      for (size_t col = pivot; col < n; ++col) {
        matrix[row][col] -= factor * matrix[pivot][col];
      }
      rhs[row] -= factor * rhs[pivot];
    }
  }
  return rhs;
}

}  // namespace

RidgeRegression::RidgeRegression(double lambda) : lambda_(lambda) {
  if (lambda_ < 0.0) {
    throw std::runtime_error("Ridge lambda cannot be negative.");
  }
}

void RidgeRegression::fit(
    const std::vector<std::vector<double>>& features,
    const std::vector<double>& targets) {
  if (features.empty() || features.size() != targets.size()) {
    throw std::runtime_error("Ridge regression requires matching non-empty feature and target rows.");
  }
  const size_t feature_count = features.front().size();
  for (const auto& row : features) {
    if (row.size() != feature_count) {
      throw std::runtime_error("Ridge regression feature rows must have a consistent width.");
    }
  }

  const size_t n = feature_count + 1;  // Intercept + coefficients.
  std::vector<std::vector<double>> xtx(n, std::vector<double>(n, 0.0));
  std::vector<double> xty(n, 0.0);

  for (size_t row = 0; row < features.size(); ++row) {
    std::vector<double> x(n, 1.0);
    for (size_t col = 0; col < feature_count; ++col) {
      x[col + 1] = features[row][col];
    }
    for (size_t i = 0; i < n; ++i) {
      xty[i] += x[i] * targets[row];
      for (size_t j = 0; j < n; ++j) {
        xtx[i][j] += x[i] * x[j];
      }
    }
  }

  for (size_t i = 1; i < n; ++i) {
    xtx[i][i] += lambda_;
  }

  const std::vector<double> solution = solve_linear_system(xtx, xty);
  intercept_ = solution.front();
  coefficients_.assign(solution.begin() + 1, solution.end());
}

double RidgeRegression::predict(const std::vector<double>& features) const {
  if (features.size() != coefficients_.size()) {
    throw std::runtime_error("Ridge regression prediction feature width does not match fitted coefficients.");
  }
  double value = intercept_;
  for (size_t index = 0; index < features.size(); ++index) {
    value += coefficients_[index] * features[index];
  }
  return value;
}

const std::vector<double>& RidgeRegression::coefficients() const {
  return coefficients_;
}

double RidgeRegression::intercept() const {
  return intercept_;
}

}  // namespace metis
