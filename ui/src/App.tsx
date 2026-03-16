import { useEffect, useMemo, useState } from "react";

type ResultRow = {
  dip_pct: number;
  lookback_days: number;
  hold_days: number;
  final_equity: number;
  total_return: number;
  cagr: number;
  max_drawdown: number;
  sharpe: number;
  trades: number;
  win_rate: number;
};

type BacktestPayload = {
  symbol: string;
  generated_at: string;
  bars: number;
  initial_equity: number;
  hold_days: number;
  best: ResultRow;
  results: ResultRow[];
};

const percent = (value: number) => `${(value * 100).toFixed(2)}%`;
const money = (value: number) => `$${value.toFixed(2)}`;

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
        <h1>SPY Dip Strategy Explorer</h1>
        <p>Buy when SPY drops by X% over Y days, then hold for a fixed window.</p>
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
              <p><strong>X dip:</strong> {percent(data.best.dip_pct)}</p>
              <p><strong>Y lookback:</strong> {data.best.lookback_days} days</p>
              <p><strong>Hold:</strong> {data.best.hold_days} days</p>
            </article>

            <article className="card">
              <h2>Performance</h2>
              <p><strong>Final equity:</strong> {money(data.best.final_equity)}</p>
              <p><strong>Total return:</strong> {percent(data.best.total_return)}</p>
              <p><strong>CAGR:</strong> {percent(data.best.cagr)}</p>
              <p><strong>Max drawdown:</strong> {percent(data.best.max_drawdown)}</p>
              <p><strong>Sharpe:</strong> {data.best.sharpe.toFixed(2)}</p>
              <p><strong>Trades:</strong> {data.best.trades}</p>
              <p><strong>Win rate:</strong> {percent(data.best.win_rate)}</p>
            </article>
          </section>

          <section className="card">
            <h2>Top 10 by CAGR</h2>
            <table>
              <thead>
                <tr>
                  <th>X Dip</th>
                  <th>Y Days</th>
                  <th>Final Equity</th>
                  <th>CAGR</th>
                  <th>Max DD</th>
                  <th>Trades</th>
                </tr>
              </thead>
              <tbody>
                {topTen.map((row) => (
                  <tr key={`${row.dip_pct}-${row.lookback_days}`}>
                    <td>{percent(row.dip_pct)}</td>
                    <td>{row.lookback_days}</td>
                    <td>{money(row.final_equity)}</td>
                    <td>{percent(row.cagr)}</td>
                    <td>{percent(row.max_drawdown)}</td>
                    <td>{row.trades}</td>
                  </tr>
                ))}
              </tbody>
            </table>
            <p className="meta">
              Generated {new Date(data.generated_at).toLocaleString()} from {data.bars} rows.
            </p>
          </section>
        </>
      )}
    </main>
  );
}

export default App;

