from __future__ import annotations

import argparse
import json
import os
import ssl
from datetime import datetime
from pathlib import Path
from typing import Any
from urllib.error import HTTPError
from urllib.parse import urlencode
from urllib.request import Request, urlopen

from metis.data.storage import PriceBar, processed_price_path, write_price_csv

DEFAULT_BASE_URL = "https://localhost:5000/v1/api"


class IBKRError(RuntimeError):
    pass


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
    ) -> None:
        self.base_url = (base_url or os.getenv("IBKR_WEB_API_BASE_URL") or DEFAULT_BASE_URL).rstrip("/")
        self.access_token = access_token or os.getenv("IBKR_WEB_API_ACCESS_TOKEN")
        self.cookie = cookie or os.getenv("IBKR_WEB_API_COOKIE")
        self.insecure = _bool_env("IBKR_WEB_API_INSECURE", True) if insecure is None else insecure

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
            "User-Agent": "kairos/0.1 (+https://local.dev)",
        }
        if self.access_token:
            headers["Authorization"] = f"Bearer {self.access_token}"
        if self.cookie:
            headers["Cookie"] = self.cookie
        return headers

    def request_json(self, method: str, endpoint: str, params: dict[str, Any] | None = None) -> Any:
        query = urlencode(params or {})
        url = f"{self.base_url}/{endpoint.lstrip('/')}"
        if query:
            url = f"{url}?{query}"
        request = Request(url, headers=self._headers(), method=method.upper())
        try:
            with urlopen(request, context=self._ssl_context()) as response:  # noqa: S310
                payload = json.loads(response.read().decode("utf-8"))
        except HTTPError as exc:
            body = exc.read().decode("utf-8", errors="replace")
            detail = f"HTTP Error {exc.code}: {exc.reason}"
            if body:
                detail = f"{detail}; body={body[:500]}"
            raise IBKRError(f"IBKR request failed for {endpoint}: {detail}") from exc
        except Exception as exc:
            raise IBKRError(f"IBKR request failed for {endpoint}: {exc}") from exc
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
    ) -> dict[str, Any]:
        return self.request_json(
            "GET",
            "iserver/marketdata/history",
            {
                "conid": str(conid),
                "exchange": exchange,
                "period": period,
                "bar": bar,
                "outsideRth": str(outside_rth).lower(),
                "source": source.title(),
            },
        )


def _number(value: Any) -> float:
    if value in (None, "", "--"):
        return 0.0
    return float(str(value).replace(",", ""))


def normalize_historical_bars(payload: dict[str, Any]) -> list[PriceBar]:
    rows = payload.get("data", [])
    volume_factor = _number(payload.get("volumeFactor", 1) or 1)
    bars: list[PriceBar] = []
    for row in rows:
        timestamp = row.get("t", row.get("time"))
        if timestamp is None:
            continue
        date = datetime.utcfromtimestamp(int(timestamp) / 1000).strftime("%Y-%m-%d")
        close = _number(row.get("c"))
        volume = _number(row.get("v")) * volume_factor
        bars.append(
            PriceBar(
                date=date,
                open=_number(row.get("o", close)),
                high=_number(row.get("h", close)),
                low=_number(row.get("l", close)),
                close=close,
                adj_close=close,
                volume=volume,
            )
        )
    return sorted(bars, key=lambda item: item.date)


def fetch_stock_history(symbol: str, period: str = "1y", bar: str = "1d", client: IBKRWebApiClient | None = None) -> list[PriceBar]:
    resolved_client = client or IBKRWebApiClient()
    resolved_client.ensure_brokerage_session()
    conid = resolved_client.resolve_stock_conid(symbol)
    payload = resolved_client.fetch_historical_bars(conid=conid, period=period, bar=bar)
    return normalize_historical_bars(payload)


def write_stock_history(output_path: str | Path, symbol: str, period: str = "1y", bar: str = "1d") -> Path:
    bars = fetch_stock_history(symbol=symbol, period=period, bar=bar)
    if not bars:
        raise IBKRError(f"IBKR returned no historical bars for {symbol}.")
    return write_price_csv(output_path, bars)


def main() -> None:
    parser = argparse.ArgumentParser(description="Fetch historical stock bars from the local IBKR Client Portal Gateway.")
    parser.add_argument("--symbol", default="SPY")
    parser.add_argument("--period", default="1y")
    parser.add_argument("--bar", default="1d")
    parser.add_argument("--data-dir", default="data")
    parser.add_argument("--output")
    parser.add_argument("--auth-status", action="store_true", help="Only print the IBKR authentication status.")
    args = parser.parse_args()

    if args.auth_status:
        print(json.dumps(IBKRWebApiClient().auth_status(), indent=2))
        return

    output = Path(args.output) if args.output else processed_price_path(args.data_dir, args.symbol, args.bar) #TODO: Adjust where to write these files to
    path = write_stock_history(output, symbol=args.symbol, period=args.period, bar=args.bar)
    print(f"Wrote {path}")


if __name__ == "__main__":
    main()
