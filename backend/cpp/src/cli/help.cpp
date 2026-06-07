#include "metis/cli/help.hpp"

#include <iostream>

namespace metis {
namespace {

void print_shared_options() {
  std::cout << "Shared options:\n";
  std::cout << "  --symbol <ticker>     Symbol label for the output (default: SPY)\n";
  std::cout << "  --csv <path>          CSV file with Date + Close/Adj Close columns\n";
  std::cout << "  --output <path>       JSON output path (default: ui/public/latest.json)\n";
  std::cout << "  --initial <amount>    Starting cash (default: 10000)\n";
  std::cout << "  --fixed-cost <amount> Fixed transaction cost per order (default: 0)\n";
  std::cout << "  --variable-cost-rate <rate> Variable transaction cost rate per order (default: 0)\n";
  std::cout << "  --bars-per-year <n>   Bars per year for CAGR/Sharpe annualization (default: 252)\n";
  std::cout << "  --walk-train-months <n>  Months optimized before each test window (default: 24)\n";
  std::cout << "  --walk-test-months <n>   Months traded after each optimization (default: 6)\n";
  std::cout << "  --training-results-dir <path> Directory for per-period training result CSVs; empty disables export\n";
  std::cout << "  --help                Show help for the current command level\n";
}

void print_discrete_grid_options() {
  std::cout << "Discrete-grid options:\n";
  std::cout << "  --x-min <value>       Min diff percent decimal (default: 0.01)\n";
  std::cout << "  --x-max <value>       Max diff percent decimal (default: 0.10)\n";
  std::cout << "  --x-step <value>      Diff sweep step (default: 0.01)\n";
  std::cout << "  --y-min <days>        Min lookback days (default: 2)\n";
  std::cout << "  --y-max <days>        Max lookback days (default: 20)\n";
  std::cout << "  --y-step <days>       Lookback sweep step (default: 1)\n";
  std::cout << "  --hold-min <days>     Min hold days after buy signal (default: 10)\n";
  std::cout << "  --hold-max <days>     Max hold days after buy signal (default: 60)\n";
  std::cout << "  --hold-step <days>    Hold-day sweep step (default: 5)\n";
  std::cout << "  --take-profit-min <value>     Min take-profit percent decimal; 0 disables it (default: 0)\n";
  std::cout << "  --take-profit-max <value>     Max take-profit percent decimal; 0 disables it (default: 0)\n";
  std::cout << "  --take-profit-step <value>    Take-profit sweep step (default: 0.01)\n";
  std::cout << "  --stop-loss-min <value>       Min stop-loss percent decimal; 0 disables it (default: 0)\n";
  std::cout << "  --stop-loss-max <value>       Max stop-loss percent decimal; 0 disables it (default: 0)\n";
  std::cout << "  --stop-loss-step <value>      Stop-loss sweep step (default: 0.01)\n";
  std::cout << "  --trailing-stop-min <value>   Min trailing stop after take-profit trigger (default: 0)\n";
  std::cout << "  --trailing-stop-max <value>   Max trailing stop after take-profit trigger (default: 0)\n";
  std::cout << "  --trailing-stop-step <value>  Trailing stop sweep step (default: 0.01)\n";
  std::cout << "  --grid-sample-pct <value> Random fraction of full grid to evaluate, 0-1 (default: 1)\n";
  std::cout << "  --grid-sample-seed <n> Deterministic random seed for sampled grids (default: 42)\n";
  std::cout << "  --grid-random-samples <n> Draw n random regime parameter sets directly; 0 uses grid loops (default: 0)\n";
}

void print_regime_options() {
  std::cout << "Regime-only discrete-grid options:\n";
  std::cout << "  --fast-lookback-min <days>  Min fast MA lookback for regime crossover (default: 2)\n";
  std::cout << "  --fast-lookback-max <days>  Max fast MA lookback for regime crossover (default: 10)\n";
  std::cout << "  --fast-lookback-step <days> Fast MA lookback sweep step (default: 1)\n";
  std::cout << "  --short-y-min <days>        Min short slow MA lookback; 0 mirrors long slow MA (default: 0)\n";
  std::cout << "  --short-y-max <days>        Max short slow MA lookback; 0 mirrors long slow MA (default: 0)\n";
  std::cout << "  --short-y-step <days>       Short slow MA sweep step (default: 1)\n";
  std::cout << "  --short-fast-lookback-min <days> Min short fast MA lookback; 0 mirrors long fast MA (default: 0)\n";
  std::cout << "  --short-fast-lookback-max <days> Max short fast MA lookback; 0 mirrors long fast MA (default: 0)\n";
  std::cout << "  --short-fast-lookback-step <days> Short fast MA sweep step (default: 1)\n";
  std::cout << "  --regime-weak-exit-mode <mode> Regime weakness exit mode: off, on, or both (default: off)\n";
  std::cout << "  --short-mode <mode>     Short selling mode: off, on, or both (default: off)\n";
  std::cout << "  --vol-lookback-min <bars> Min realized-vol lookback; 0 disables vol sizing (default: 0)\n";
  std::cout << "  --vol-lookback-max <bars> Max realized-vol lookback; 0 disables vol sizing (default: 0)\n";
  std::cout << "  --vol-lookback-step <bars> Realized-vol lookback sweep step (default: 1)\n";
  std::cout << "  --target-vol-min <value> Min annualized target vol; 0 disables vol sizing (default: 0)\n";
  std::cout << "  --target-vol-max <value> Max annualized target vol; 0 disables vol sizing (default: 0)\n";
  std::cout << "  --target-vol-step <value> Target-vol sweep step (default: 0.01)\n";
  std::cout << "  --max-position-pct-min <value> Min equity fraction used as notional cap (default: 1)\n";
  std::cout << "  --max-position-pct-max <value> Max equity fraction used as notional cap (default: 1)\n";
  std::cout << "  --max-position-pct-step <value> Position cap sweep step (default: 0.25)\n";
}

}  // namespace

void print_general_help() {
  std::cout << "Metis C++ Backtester\n\n";
  std::cout << "Usage:\n";
  std::cout << "  metis_backtester <approach> <strategy> [options]\n\n";
  std::cout << "Approaches:\n";
  std::cout << "  buy-and-hold    Fixed allocation strategies such as lump-sum buy-and-hold\n";
  std::cout << "  discrete-grid   Walk-forward grid search over discrete strategy parameters\n";
  std::cout << "  ridge           Ridge-regression strategies (planned)\n\n";
  std::cout << "Examples:\n";
  std::cout << "  metis_backtester discrete-grid gain --help\n";
  std::cout << "  metis_backtester discrete-grid regime --walk-train-months 24 --walk-test-months 6\n";
  std::cout << "  metis_backtester buy-and-hold lump-sum --help\n\n";
  std::cout << "Run 'metis_backtester <approach> --help' to list strategies for one approach.\n";
}

void print_approach_help(ApproachType approach) {
  if (approach == ApproachType::BuyAndHold) {
    std::cout << "Usage:\n";
    std::cout << "  metis_backtester buy-and-hold <strategy> [options]\n\n";
    std::cout << "Buy-and-hold strategies:\n";
    std::cout << "  lump-sum        Buy once at the first bar and hold\n";
    std::cout << "  scheduled-buy   Planned; periodic contribution strategy\n\n";
    std::cout << "Run 'metis_backtester buy-and-hold lump-sum --help' for options.\n";
    return;
  }
  if (approach == ApproachType::Ridge) {
    std::cout << "Usage:\n";
    std::cout << "  metis_backtester ridge <strategy> [options]\n\n";
    std::cout << "Ridge strategies:\n";
    std::cout << "  directional     Planned; long/flat from predicted return threshold\n";
    std::cout << "  long-short      Planned; long/short from upper/lower prediction bands\n";
    std::cout << "  vol-target      Planned; prediction-driven position with vol cap\n\n";
    std::cout << "Run 'metis_backtester ridge directional --help' for planned options.\n";
    return;
  }
  std::cout << "Usage:\n";
  std::cout << "  metis_backtester discrete-grid <strategy> [options]\n\n";
  std::cout << "Discrete-grid strategies:\n";
  std::cout << "  drop            Long after a configured drop over a lookback window\n";
  std::cout << "  gain            Long after a configured gain over a lookback window\n";
  std::cout << "  regime          Moving-average crossover regime strategy\n\n";
  std::cout << "Run 'metis_backtester discrete-grid gain --help' for options.\n";
}

void print_strategy_help(ApproachType approach, StrategyFamily strategy) {
  std::cout << "Metis C++ Backtester\n\n";
  if (approach == ApproachType::BuyAndHold) {
    std::cout << "Usage:\n";
    std::cout << "  metis_backtester buy-and-hold lump-sum [options]\n\n";
    print_shared_options();
    return;
  }
  if (approach == ApproachType::Ridge) {
    std::cout << "Usage:\n";
    std::cout << "  metis_backtester ridge <directional|long-short|vol-target> [options]\n\n";
    std::cout << "Ridge execution is planned. Shared walk-forward/data options will apply once implemented.\n\n";
    print_shared_options();
    return;
  }
  std::cout << "Usage:\n";
  std::cout << "  metis_backtester discrete-grid <drop|gain|regime> [options]\n\n";
  print_shared_options();
  std::cout << "\n";
  print_discrete_grid_options();
  if (strategy == StrategyFamily::Regime) {
    std::cout << "\n";
    print_regime_options();
  }
}

}  // namespace metis
