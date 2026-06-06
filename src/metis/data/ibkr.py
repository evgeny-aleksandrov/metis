from __future__ import annotations

import argparse
import json
import os
import socket
import ssl
import sys
import time
from dataclasses import dataclass
from datetime import datetime, timedelta, timezone
from pathlib import Path
from typing import Any
from urllib.error import HTTPError, URLError
from urllib.parse import urlencode
from urllib.request import Request, urlopen

from metis.data.storage import PriceBar, processed_price_path, write_price_csv

DEFAULT_BASE_URL = "https://localhost:5000/v1/api"
DEFAULT_HISTORY_THROTTLE_SECONDS = 1.0
DEFAULT_REQUEST_TIMEOUT_SECONDS = 30.0
DEFAULT_RETRIES = 3
DEFAULT_HISTORY_YEARS = 2
DEFAULT_CHUNK_DAYS = 30
DEFAULT_USER_AGENT = (
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
    "AppleWebKit/537.36 (KHTML, like Gecko) "
    "Chrome/124.0.0.0 Safari/537.36"
)


class IBKRError(RuntimeError):
    pass


@dataclass(slots=True)
class RequestThrottle:
    """Conservative one-at-a-time throttle for IBKR historical data requests."""

    min_interval_seconds: float = DEFAULT_HISTORY_THROTTLE_SECONDS
    _last_request_at: float = 0.0

    def wait(self) -> None:
        if self.min_interval_seconds <= 0:
            return
        elapsed = time.monotonic() - self._last_request_at
        remaining = self.min_interval_seconds - elapsed
        if remaining > 0:
            time.sleep(remaining)

    def mark(self) -> None:
        self._last_request_at = time.monotonic()


def _bool_env(name: str, default: bool = False) -> bool:
    raw = os.getenv(name)
    if raw is None:
        return default
    return raw.strip().lower() in {"1", "true", "yes", "y", "on"}


class IBKRWebApiClient:
    """Small IBKR Client Portal Gateway client for historical bars."""

    def __init__(
        self,
        base_url: str | None = None,
        access_token: str | None = None,
        cookie: str | None = None,
        insecure: bool | None = None,
        history_throttle_seconds: float | None = None,
        request_timeout_seconds: float | None = None,
        max_retries: int = DEFAULT_RETRIES,
    ) -> None:
        self.base_url = (base_url or os.getenv("IBKR_WEB_API_BASE_URL") or DEFAULT_BASE_URL).rstrip("/")
        self.access_token = access_token or os.getenv("IBKR_WEB_API_ACCESS_TOKEN")
        self.cookie = cookie or os.getenv("IBKR_WEB_API_COOKIE")
        self.insecure = _bool_env("IBKR_WEB_API_INSECURE", True) if insecure is None else insecure
        throttle_seconds = (
            float(os.getenv("IBKR_HISTORY_THROTTLE_SECONDS", DEFAULT_HISTORY_THROTTLE_SECONDS))
            if history_throttle_seconds is None
            else history_throttle_seconds
        )
        self.history_throttle = RequestThrottle(throttle_seconds)
        self.request_timeout_seconds = (
            float(os.getenv("IBKR_REQUEST_TIMEOUT_SECONDS", DEFAULT_REQUEST_TIMEOUT_SECONDS))
            if request_timeout_seconds is None
            else request_timeout_seconds
        )
        self.max_retries = max_retries

    def _ssl_context(self) -> ssl.SSLContext | None:
        if not self.insecure:
            return None
        context = ssl.create_default_context()
        context.check_hostname = False
        context.verify_mode = ssl.CERT_NONE
        return context

    def _headers(self) -> dict[str, str]:
        headers = {
            "Accept": "application/json",
            "User-Agent": os.getenv("IBKR_WEB_API_USER_AGENT", DEFAULT_USER_AGENT),
        }
        if self.access_token:
            headers["Authorization"] = f"Bearer {self.access_token}"
        if self.cookie:
            headers["Cookie"] = self.cookie
        return headers

    def request_json(
        self,
        method: str,
        endpoint: str,
        params: dict[str, Any] | None = None,
        throttle: RequestThrottle | None = None,
    ) -> Any:
        query = urlencode(params or {})
        url = f"{self.base_url}/{endpoint.lstrip('/')}"
        if query:
            url = f"{url}?{query}"
        request = Request(url, headers=self._headers(), method=method.upper())
        attempts = self.max_retries + 1
        for attempt in range(attempts):
            if throttle is not None:
                throttle.wait()
            try:
                with urlopen(request, context=self._ssl_context(), timeout=self.request_timeout_seconds) as response:  # noqa: S310
                    payload = json.loads(response.read().decode("utf-8"))
                if throttle is not None:
                    throttle.mark()
                break
            except HTTPError as exc:
                if throttle is not None:
                    throttle.mark()
                body = exc.read().decode("utf-8", errors="replace")
                if attempt < self.max_retries and exc.code in {429, 500, 502, 503, 504}:
                    time.sleep(_retry_delay(exc, attempt))
                    continue
                detail = f"HTTP Error {exc.code}: {exc.reason}"
                if body:
                    detail = f"{detail}; body={body[:500]}"
                raise IBKRError(f"IBKR request failed for {endpoint}: {detail}") from exc
            except URLError as exc:
                if throttle is not None:
                    throttle.mark()
                if attempt < self.max_retries:
                    time.sleep(2**attempt)
                    continue
                raise IBKRError(f"IBKR request failed for {endpoint}: {exc}") from exc
            except TimeoutError as exc:
                if throttle is not None:
                    throttle.mark()
                if attempt < self.max_retries:
                    time.sleep(2**attempt)
                    continue
                raise IBKRError(f"IBKR request failed for {endpoint}: {exc}") from exc
            except socket.timeout as exc:
                if throttle is not None:
                    throttle.mark()
                if attempt < self.max_retries:
                    time.sleep(2**attempt)
                    continue
                raise IBKRError(f"IBKR request failed for {endpoint}: {exc}") from exc
            except Exception as exc:
                if throttle is not None:
                    throttle.mark()
                raise IBKRError(f"IBKR request failed for {endpoint}: {exc}") from exc
        else:
            raise IBKRError(f"IBKR request failed for {endpoint}: retries exhausted")
        if isinstance(payload, dict) and payload.get("error"):
            raise IBKRError(str(payload["error"]))
        return payload

    def ensure_brokerage_session(self) -> Any:
        return self.request_json("GET", "iserver/accounts")

    def auth_status(self) -> Any:
        return self.request_json("GET", "iserver/auth/status")

    def resolve_stock_conid(self, symbol: str) -> int:
        payload = self.request_json("GET", "trsrv/stocks", {"symbols": symbol.upper()})
        candidates = payload.get(symbol.upper(), [])
        for candidate in candidates:
            if candidate.get("assetClass") != "STK":
                continue
            for contract in candidate.get("contracts", []):
                if contract.get("isUS"):
                    return int(contract["conid"])
        raise IBKRError(f"Could not resolve a US stock conid for {symbol}.")

    def fetch_historical_bars(
        self,
        conid: int,
        exchange: str = "SMART",
        period: str = "1y",
        bar: str = "1d",
        outside_rth: bool = False,
        source: str = "trades",
        start_time: datetime | None = None,
    ) -> dict[str, Any]:
        params = {
            "conid": str(conid),
            "exchange": exchange,
            "period": period,
            "bar": bar,
            "outsideRth": str(outside_rth).lower(),
            "source": _format_history_source(source),
        }
        if start_time is not None:
            params["startTime"] = _format_ibkr_start_time(start_time)
        return self.request_json(
            "GET",
            "iserver/marketdata/history",
            params,
            throttle=self.history_throttle,
        )


def _number(value: Any) -> float:
    if value in (None, "", "--"):
        return 0.0
    return float(str(value).replace(",", ""))


def _retry_delay(exc: HTTPError, attempt: int) -> float:
    retry_after = exc.headers.get("Retry-After")
    if retry_after:
        try:
            return max(float(retry_after), 1.0)
        except ValueError:
            pass
    return float(2**attempt)


def _timestamp_to_utc(timestamp: Any) -> datetime:
    return datetime.fromtimestamp(int(timestamp) / 1000, tz=timezone.utc)


def _format_ibkr_start_time(value: datetime) -> str:
    if value.tzinfo is None:
        value = value.replace(tzinfo=timezone.utc)
    return value.astimezone(timezone.utc).strftime("%Y%m%d-%H:%M:%S")


def _format_history_source(source: str) -> str:
    return {
        "trades": "Trades",
        "midpoint": "Midpoint",
        "bid_ask": "Bid_Ask",
        "bid": "bid",
        "ask": "ask",
    }.get(source, source)


def _bar_timestamp(timestamp: Any, bar: str) -> str:
    parsed = _timestamp_to_utc(timestamp)
    if bar.endswith("min") or bar.endswith("h"):
        return parsed.strftime("%Y-%m-%d %H:%M:%S")
    return parsed.strftime("%Y-%m-%d")


def normalize_historical_bars(payload: dict[str, Any], bar: str = "1d") -> list[PriceBar]:
    rows = payload.get("data", [])
    volume_factor = _number(payload.get("volumeFactor", 1) or 1)
    bars: list[PriceBar] = []
    for row in rows:
        timestamp = row.get("t", row.get("time"))
        if timestamp is None:
            continue
        date = _bar_timestamp(timestamp, bar)
        close = _number(row.get("c"))
        volume = _number(row.get("v")) * volume_factor
        bars.append(
            PriceBar(
                date=date,
                open=_number(row.get("o", close)),
                high=_number(row.get("h", close)),
                low=_number(row.get("l", close)),
                close=close,
                adj_close=close,  # IBKR Client Portal history does not provide adjusted close.
                volume=volume,
            )
        )
    return sorted(bars, key=lambda item: item.date)


def _normalize_windowed_historical_bars(
    payload: dict[str, Any],
    bar: str,
    window_start: datetime,
    window_end: datetime,
) -> list[PriceBar]:
    rows = payload.get("data", [])
    volume_factor = _number(payload.get("volumeFactor", 1) or 1)
    bars: list[PriceBar] = []
    for row in rows:
        timestamp = row.get("t", row.get("time"))
        if timestamp is None:
            continue
        parsed = _timestamp_to_utc(timestamp)
        if not window_start <= parsed < window_end:
            continue
        close = _number(row.get("c"))
        volume = _number(row.get("v")) * volume_factor
        bars.append(
            PriceBar(
                date=_bar_timestamp(timestamp, bar),
                open=_number(row.get("o", close)),
                high=_number(row.get("h", close)),
                low=_number(row.get("l", close)),
                close=close,
                adj_close=close,
                volume=volume,
            )
        )
    return bars


def _dedupe_bars(bars: list[PriceBar]) -> list[PriceBar]:
    by_timestamp: dict[str, PriceBar] = {}
    for bar in bars:
        by_timestamp[bar.date] = bar
    return [by_timestamp[key] for key in sorted(by_timestamp)]


def _default_history_end() -> datetime:
    return datetime.now().astimezone().replace(hour=22, minute=0, second=0, microsecond=0)


def _parse_history_start_date(value: str) -> datetime:
    return datetime.strptime(value, "%Y-%m-%d").replace(tzinfo=timezone.utc)


def _chunk_windows(
    years: int | None,
    chunk_days: int,
    end: datetime | None = None,
    start: datetime | None = None,
) -> list[tuple[datetime, datetime]]:
    if years is not None and years <= 0:
        raise ValueError("years must be positive.")
    if chunk_days <= 0:
        raise ValueError("chunk_days must be positive.")
    resolved_end = end or _default_history_end()
    if resolved_end.tzinfo is None:
        resolved_end = resolved_end.replace(tzinfo=timezone.utc)
    resolved_end = resolved_end.astimezone(timezone.utc).replace(minute=0, second=0, microsecond=0)
    if start is None:
        if years is None:
            raise ValueError("years must be set when start is not provided.")
        start = resolved_end - timedelta(days=years * 365)
    if start.tzinfo is None:
        start = start.replace(tzinfo=timezone.utc)
    start = start.astimezone(timezone.utc).replace(minute=0, second=0, microsecond=0)
    if start >= resolved_end:
        raise ValueError("start must be before end.")
    windows: list[tuple[datetime, datetime]] = []
    cursor = start
    while cursor < resolved_end:
        next_cursor = min(cursor + timedelta(days=chunk_days), resolved_end)
        windows.append((cursor, next_cursor))
        cursor = next_cursor
    return windows


def _years_from_period(period: str) -> int:
    normalized = period.strip().lower()
    if not normalized.endswith("y"):
        raise ValueError("--period is only supported for yearly chunked backfills, e.g. --period 2y. Use --years instead.")
    years = int(normalized[:-1])
    if years <= 0:
        raise ValueError("period years must be positive.")
    return years


def fetch_stock_history(symbol: str, period: str = "1y", bar: str = "1d", client: IBKRWebApiClient | None = None) -> list[PriceBar]:
    resolved_client = client or IBKRWebApiClient()
    resolved_client.ensure_brokerage_session()
    conid = resolved_client.resolve_stock_conid(symbol)
    payload = resolved_client.fetch_historical_bars(conid=conid, period=period, bar=bar)
    return normalize_historical_bars(payload, bar=bar)


def fetch_stock_history_chunked(
    symbol: str,
    years: int | None = DEFAULT_HISTORY_YEARS,
    chunk_days: int = DEFAULT_CHUNK_DAYS,
    bar: str = "1h",
    client: IBKRWebApiClient | None = None,
    outside_rth: bool = False,
    source: str = "trades",
    progress: bool = True,
    start: datetime | None = None,
) -> list[PriceBar]:
    resolved_client = client or IBKRWebApiClient()
    resolved_client.ensure_brokerage_session()
    conid = resolved_client.resolve_stock_conid(symbol)
    bars: list[PriceBar] = []
    windows = _chunk_windows(years=years, chunk_days=chunk_days, start=start)
    if progress:
        span = f"from {start:%Y-%m-%d}" if start is not None else f"{years}y"
        print(
            f"[{symbol.upper()}] fetching {span} of {bar} bars in {len(windows)} chunks "
            f"({chunk_days}d windows)",
            file=sys.stderr,
            flush=True,
        )
    for index, (window_start, window_end) in enumerate(windows, start=1):
        request_start = window_start - timedelta(days=1)
        seconds = int((window_end - request_start).total_seconds())
        period_days = max(1, (seconds + 86_399) // 86_400)
        period = f"{period_days}d"
        payload = resolved_client.fetch_historical_bars(
            conid=conid,
            period=period,
            bar=bar,
            outside_rth=outside_rth,
            source=source,
            start_time=window_end,
        )
        window_bars = _normalize_windowed_historical_bars(
            payload,
            bar=bar,
            window_start=window_start,
            window_end=window_end,
        )
        bars.extend(window_bars)
        if progress:
            print(
                f"[{symbol.upper()}] chunk {index}/{len(windows)} "
                f"{window_start:%Y-%m-%d}..{window_end:%Y-%m-%d}: "
                f"{len(window_bars)} bars ({len(_dedupe_bars(bars))} total)",
                file=sys.stderr,
                flush=True,
            )
    return _dedupe_bars(bars)


def write_stock_history(
    output_path: str | Path,
    symbol: str,
    period: str = "1y",
    bar: str = "1d",
    client: IBKRWebApiClient | None = None,
    outside_rth: bool = False,
    source: str = "trades",
) -> Path:
    resolved_client = client or IBKRWebApiClient()
    resolved_client.ensure_brokerage_session()
    conid = resolved_client.resolve_stock_conid(symbol)
    payload = resolved_client.fetch_historical_bars(
        conid=conid,
        period=period,
        bar=bar,
        outside_rth=outside_rth,
        source=source,
    )
    bars = normalize_historical_bars(payload, bar=bar)
    if not bars:
        raise IBKRError(f"IBKR returned no historical bars for {symbol}.")
    return write_price_csv(output_path, bars)


def write_stock_history_chunked(
    output_path: str | Path,
    symbol: str,
    years: int | None = DEFAULT_HISTORY_YEARS,
    chunk_days: int = DEFAULT_CHUNK_DAYS,
    bar: str = "1h",
    client: IBKRWebApiClient | None = None,
    outside_rth: bool = False,
    source: str = "trades",
    progress: bool = True,
    start: datetime | None = None,
) -> Path:
    bars = fetch_stock_history_chunked(
        symbol=symbol,
        years=years,
        chunk_days=chunk_days,
        bar=bar,
        client=client,
        outside_rth=outside_rth,
        source=source,
        progress=progress,
        start=start,
    )
    if not bars:
        raise IBKRError(f"IBKR returned no historical bars for {symbol}.")
    return write_price_csv(output_path, bars)


def write_stock_histories(
    symbols: list[str],
    data_dir: str | Path,
    years: int | None = DEFAULT_HISTORY_YEARS,
    chunk_days: int = DEFAULT_CHUNK_DAYS,
    bar: str = "1h",
    outside_rth: bool = False,
    source: str = "trades",
    throttle_seconds: float = DEFAULT_HISTORY_THROTTLE_SECONDS,
    progress: bool = True,
    start: datetime | None = None,
) -> list[Path]:
    client = IBKRWebApiClient(history_throttle_seconds=throttle_seconds)
    client.ensure_brokerage_session()
    paths: list[Path] = []
    for symbol in symbols:
        output = processed_price_path(data_dir, symbol, bar)
        path = write_stock_history_chunked(
            output,
            symbol=symbol,
            years=years,
            chunk_days=chunk_days,
            bar=bar,
            client=client,
            outside_rth=outside_rth,
            source=source,
            progress=progress,
            start=start,
        )
        paths.append(path)
    return paths


def main() -> None:
    parser = argparse.ArgumentParser(description="Fetch historical stock bars from the local IBKR Client Portal Gateway.")
    parser.add_argument("--symbol", default="SPY")
    parser.add_argument("--symbols", help="Comma-separated symbols. Overrides --symbol.")
    parser.add_argument("--years", type=int, default=DEFAULT_HISTORY_YEARS)
    parser.add_argument("--period", help="Deprecated alias for --years when expressed as Ny, e.g. 2y.")
    parser.add_argument("--start-date", help="Fetch from this UTC date, in YYYY-MM-DD format. Overrides --years/--period.")
    parser.add_argument("--chunk-days", type=int, default=DEFAULT_CHUNK_DAYS)
    parser.add_argument("--bar", default="1h")
    parser.add_argument("--data-dir", default="data")
    parser.add_argument("--output")
    parser.add_argument("--outside-rth", action="store_true", help="Include data outside regular trading hours.")
    parser.add_argument("--source", default="trades", choices=["trades", "midpoint", "bid_ask", "bid", "ask"])
    parser.add_argument("--quiet", action="store_true", help="Do not print per-chunk download progress.")
    parser.add_argument(
        "--throttle-seconds",
        type=float,
        default=DEFAULT_HISTORY_THROTTLE_SECONDS,
        help="Minimum seconds between historical data requests. Default follows IBKR's 60 requests / 10 minutes guidance.",
    )
    parser.add_argument("--auth-status", action="store_true", help="Only print the IBKR authentication status.")
    args = parser.parse_args()

    if args.auth_status:
        print(json.dumps(IBKRWebApiClient().auth_status(), indent=2))
        return

    start = _parse_history_start_date(args.start_date) if args.start_date else None
    years = None if start is not None else (_years_from_period(args.period) if args.period else args.years)
    symbols = [item.strip().upper() for item in (args.symbols or args.symbol).split(",") if item.strip()]
    if args.output and len(symbols) != 1:
        raise IBKRError("--output can only be used with one symbol.")

    if args.output:
        path = write_stock_history_chunked(
            Path(args.output),
            symbol=symbols[0],
            years=years,
            chunk_days=args.chunk_days,
            bar=args.bar,
            client=IBKRWebApiClient(history_throttle_seconds=args.throttle_seconds),
            outside_rth=args.outside_rth,
            source=args.source,
            progress=not args.quiet,
            start=start,
        )
        print(f"Wrote {path}")
        return

    paths = write_stock_histories(
        symbols,
        data_dir=args.data_dir,
        years=years,
        chunk_days=args.chunk_days,
        bar=args.bar,
        outside_rth=args.outside_rth,
        source=args.source,
        throttle_seconds=args.throttle_seconds,
        progress=not args.quiet,
        start=start,
    )
    for path in paths:
        print(f"Wrote {path}")


if __name__ == "__main__":
    main()
