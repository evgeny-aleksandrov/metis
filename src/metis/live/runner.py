from __future__ import annotations

import argparse
import json
from datetime import datetime, timezone
from pathlib import Path

from metis.data.ibkr import IBKRWebApiClient


def main() -> None:
    parser = argparse.ArgumentParser(description="Paper/live trading scaffold. Starts with account/session validation only.")
    parser.add_argument("--symbol", default="SPY")
    parser.add_argument("--paper", action="store_true", default=True)
    parser.add_argument("--state", default="outputs/live/live_state.json")
    args = parser.parse_args()

    client = IBKRWebApiClient()
    accounts = client.ensure_brokerage_session()
    state_path = Path(args.state)
    state_path.parent.mkdir(parents=True, exist_ok=True)
    state_path.write_text(
        json.dumps(
            {
                "checked_at": datetime.now(timezone.utc).isoformat(),
                "symbol": args.symbol.upper(),
                "paper": args.paper,
                "accounts_response": accounts,
                "status": "ibkr_session_ok_no_orders_sent",
            },
            indent=2,
        ),
        encoding="utf-8",
    )
    print(f"IBKR session OK. No orders sent. Wrote {state_path}")


if __name__ == "__main__":
    main()

