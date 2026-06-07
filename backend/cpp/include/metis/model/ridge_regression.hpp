#pragma once

#include "metis/model/model.hpp"

#include <vector>

namespace metis {

class RidgeRegression final : public PredictiveModel {
public:
  explicit RidgeRegression(double lambda = 1.0);

  void fit(
      const std::vector<std::vector<double>>& features,
      const std::vector<double>& targets) override;
  double predict(const std::vector<double>& features) const override;

  const std::vector<double>& coefficients() const;
  double intercept() const;

private:
  double lambda_ = 1.0;
  double intercept_ = 0.0;
  std::vector<double> coefficients_;
};

}  // namespace metis
