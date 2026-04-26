from __future__ import annotations

import csv
from dataclasses import dataclass
from pathlib import Path


@dataclass(slots=True)
class PriceBar:
    date: str
    open: float
    high: float
    low: float
    close: float
    adj_close: float
    volume: float


PRICE_COLUMNS = ["Date", "Open", "High", "Low", "Close", "Adj Close", "Volume"]


def processed_price_path(data_dir: str | Path, symbol: str, bar: str = "1d") -> Path:
    return Path(data_dir) / "processed" / symbol.upper() / f"{bar}.csv"


def write_price_csv(path: str | Path, bars: list[PriceBar]) -> Path:
    output_path = Path(path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(PRICE_COLUMNS)
        for bar in bars:
            writer.writerow(
                [
                    bar.date,
                    f"{bar.open:.6f}",
                    f"{bar.high:.6f}",
                    f"{bar.low:.6f}",
                    f"{bar.close:.6f}",
                    f"{bar.adj_close:.6f}",
                    f"{bar.volume:.0f}",
                ]
            )
    return output_path

