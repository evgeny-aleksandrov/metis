#pragma once

#include <vector>

namespace metis {

class PredictiveModel {
public:
  virtual ~PredictiveModel() = default;

  virtual void fit(
      const std::vector<std::vector<double>>& features,
      const std::vector<double>& targets) = 0;
  virtual double predict(const std::vector<double>& features) const = 0;
};

}  // namespace metis
