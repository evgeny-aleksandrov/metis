import { useEffect, useMemo, useState } from "react";

type ResultRow = {
  strategy: "drop" | "gain";
  diff_pct: number;
  lookback_days: number;
  hold_days: number;
  final_equity: number;
  total_return: number;
  cagr: number;
  max_drawdown: number;
  sharpe: number;
  sortino: number;
  trades: number;
  win_rate: number;
};

type EquityPoint = {
  date: string;
  equity: number;
};

type Trade = {
  entry_date: string;
  exit_date: string;
  entry_price: number;
  exit_price: number;
  shares: number;
  pnl: number;
  holding_days: number;
};

type BacktestPayload = {
  symbol: string;
  strategy: "drop" | "gain";
  generated_at: string;
  bars: number;
  initial_equity: number;
  threshold_pct: {
    min: number;
    max: number;
    step: number;
  };
  lookback_days: {
    min: number;
    max: number;
    step: number;
  };
  hold_days: {
    min: number;
    max: number;
    step: number;
  };
  best: ResultRow;
  equity_curve: EquityPoint[];
  trades: Trade[];
  results: ResultRow[];
};

const percent = (value: number) => `${(value * 100).toFixed(2)}%`;
const money = (value: number) => `$${value.toFixed(2)}`;
const strategyLabel = (value: string) => value === "gain" ? "Buy After Gain" : "Buy After Drop";

function EquityChart({ points }: { points: EquityPoint[] }) {
  if (points.length < 2) {
    return <p className="status">Not enough equity points to draw a chart.</p>;
  }
  const width = 900;
  const height = 260;
  const padding = 28;
  const values = points.map((point) => point.equity);
  const min = Math.min(...values);
  const max = Math.max(...values);
  const range = Math.max(max - min, 1);
  const path = points
    .map((point, index) => {
      const x = padding + (index / (points.length - 1)) * (width - padding * 2);
      const y = height - padding - ((point.equity - min) / range) * (height - padding * 2);
      return `${index === 0 ? "M" : "L"} ${x.toFixed(2)} ${y.toFixed(2)}`;
    })
    .join(" ");

  return (
    <svg className="chart" viewBox={`0 0 ${width} ${height}`} role="img" aria-label="Best run equity curve">
      <line x1={padding} y1={height - padding} x2={width - padding} y2={height - padding} />
      <line x1={padding} y1={padding} x2={padding} y2={height - padding} />
      <path d={path} />
      <text x={padding} y={18}>{money(max)}</text>
      <text x={padding} y={height - 6}>{money(min)}</text>
    </svg>
  );
}

function App() {
  const [data, setData] = useState<BacktestPayload | null>(null);
  const [error, setError] = useState<string>("");
  const [loading, setLoading] = useState<boolean>(true);

  useEffect(() => {
    let mounted = true;
    const load = async () => {
      try {
        setLoading(true);
        const response = await fetch("/latest.json", { cache: "no-store" });
        if (!response.ok) {
          throw new Error("No latest.json found yet. Run the C++ backtester first.");
        }
        const payload = (await response.json()) as BacktestPayload;
        if (mounted) {
          setData(payload);
          setError("");
        }
      } catch (err) {
        if (mounted) {
          setData(null);
          setError(err instanceof Error ? err.message : "Unknown error loading data.");
        }
      } finally {
        if (mounted) {
          setLoading(false);
        }
      }
    };

    load();
    return () => {
      mounted = false;
    };
  }, []);

  const topTen = useMemo(() => {
    if (!data) {
      return [];
    }
    return [...data.results].sort((a, b) => b.cagr - a.cagr).slice(0, 10);
  }, [data]);

  return (
    <main className="page">
      <header className="hero">
        <h1>Metis Strategy Explorer</h1>
        <p>Research stock strategies with Python orchestration and a C++ backtest engine.</p>
      </header>

      {loading && <p className="status">Loading backtest output...</p>}
      {!loading && error && (
        <section className="card">
          <h2>Data not ready</h2>
          <p>{error}</p>
          <p>Expected file: <code>ui/public/latest.json</code></p>
        </section>
      )}

      {!loading && data && (
        <>
          <section className="grid">
            <article className="card">
              <h2>Best Parameters</h2>
              <p><strong>Symbol:</strong> {data.symbol}</p>
              <p><strong>Strategy:</strong> {strategyLabel(data.strategy)}</p>
              <p><strong>X threshold:</strong> {percent(data.best.diff_pct)}</p>
              <p><strong>Y lookback:</strong> {data.best.lookback_days} days</p>
              <p><strong>Hold:</strong> {data.best.hold_days} days</p>
              <p><strong>Threshold sweep:</strong> {percent(data.threshold_pct.min)} to {percent(data.threshold_pct.max)}</p>
              <p><strong>Lookback sweep:</strong> {data.lookback_days.min} to {data.lookback_days.max} days</p>
            </article>

            <article className="card">
              <h2>Performance</h2>
              <p><strong>Final equity:</strong> {money(data.best.final_equity)}</p>
              <p><strong>Total return:</strong> {percent(data.best.total_return)}</p>
              <p><strong>CAGR:</strong> {percent(data.best.cagr)}</p>
              <p><strong>Max drawdown:</strong> {percent(data.best.max_drawdown)}</p>
              <p><strong>Sharpe:</strong> {data.best.sharpe.toFixed(2)}</p>
              <p><strong>Sortino:</strong> {data.best.sortino.toFixed(2)}</p>
              <p><strong>Trades:</strong> {data.best.trades}</p>
              <p><strong>Win rate:</strong> {percent(data.best.win_rate)}</p>
            </article>
          </section>

          <section className="card">
            <h2>Best Equity Curve</h2>
            <EquityChart points={data.equity_curve} />
          </section>

          <section className="card">
            <h2>Top 10 by CAGR</h2>
            <table>
              <thead>
                <tr>
                  <th>Strategy</th>
                  <th>X Threshold</th>
                  <th>Y Days</th>
                  <th>Hold Days</th>
                  <th>Final Equity</th>
                  <th>CAGR</th>
                  <th>Max DD</th>
                  <th>Sortino</th>
                  <th>Trades</th>
                </tr>
              </thead>
              <tbody>
                {topTen.map((row) => (
                  <tr key={`${row.diff_pct}-${row.lookback_days}-${row.hold_days}`}>
                    <td>{strategyLabel(row.strategy)}</td>
                    <td>{percent(row.diff_pct)}</td>
                    <td>{row.lookback_days}</td>
                    <td>{row.hold_days}</td>
                    <td>{money(row.final_equity)}</td>
                    <td>{percent(row.cagr)}</td>
                    <td>{percent(row.max_drawdown)}</td>
                    <td>{row.sortino.toFixed(2)}</td>
                    <td>{row.trades}</td>
                  </tr>
                ))}
              </tbody>
            </table>
            <p className="meta">
              Generated {new Date(data.generated_at).toLocaleString()} from {data.bars} rows.
            </p>
          </section>

          <section className="card">
            <h2>Best Run Trades</h2>
            <table>
              <thead>
                <tr>
                  <th>Entry</th>
                  <th>Exit</th>
                  <th>Entry Price</th>
                  <th>Exit Price</th>
                  <th>PnL</th>
                  <th>Days</th>
                </tr>
              </thead>
              <tbody>
                {data.trades.slice(0, 20).map((trade, index) => (
                  <tr key={`${trade.entry_date}-${trade.exit_date}-${index}`}>
                    <td>{trade.entry_date}</td>
                    <td>{trade.exit_date}</td>
                    <td>{money(trade.entry_price)}</td>
                    <td>{money(trade.exit_price)}</td>
                    <td className={trade.pnl >= 0 ? "positive" : "negative"}>{money(trade.pnl)}</td>
                    <td>{trade.holding_days}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </section>
        </>
      )}
    </main>
  );
}

export default App;
