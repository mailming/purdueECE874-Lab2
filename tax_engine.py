#!/usr/bin/env python3
"""
Avalon 2026 Tax Engine — Lab 2 implementation.
Reads NDJSON households, outputs NDJSON with federal_tax and state_tax (whole dollars).
Accepts --inputFile/--outputFile (spec) or --input/--output (version runner).
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any, List, Optional

# --- Constants (Avalon 2026) ---
STANDARD_DEDUCTION = 10_000.0
FEDERAL_BRACKETS = [(0, 100_000, 0.05), (100_000, 200_000, 0.10), (200_000, 300_000, 0.15), (300_000, float("inf"), 0.20)]
EWMA_ALPHA = 0.6
SURCHARGE_THRESHOLD = 1_000_000
SURCHARGE_RATE = 0.02
CA_WAGE_RATE = 0.04
CA_CAPGAIN_RATE = 0.06
CA_SURCHARGE_RATE = 0.05
TX_BRACKETS = [(0, 90_000, 0.03), (90_000, 200_000, 0.05), (200_000, float("inf"), 0.07)]
TX_DEDUCTION_THRESHOLD = 15_000
TX_DEDUCTION_RATE = 0.01  # 1% off state tax if federal deductions > 15k


def round_dollars(x: float) -> int:
    """Round to nearest whole dollar (A.4)."""
    return int(round(x))


def round_3dec(x: float) -> float:
    """Round to three decimal places for intermediate transactions."""
    return round(x, 3)


# --- Capital gains (FIFO) ---


def _fifo_net_capital_gain(purchases: List[dict], sales: List[dict]) -> float:
    """
    Compute net capital gain using FIFO cost basis.
    purchases/sales: list of {asset_id, date, quantity, unit_price}.
    Returns sum of realized gains (rounded to 3 decimals per transaction, then to nearest dollar).
    """
    from collections import defaultdict
    from decimal import Decimal, ROUND_HALF_UP

    # Group by asset_id
    by_asset: dict[str, dict[str, list]] = defaultdict(lambda: {"purchases": [], "sales": []})
    for p in purchases:
        by_asset[p["asset_id"]]["purchases"].append(
            {"date": p["date"], "quantity": float(p["quantity"]), "unit_price": float(p["unit_price"])}
        )
    for s in sales:
        by_asset[s["asset_id"]]["sales"].append(
            {"date": s["date"], "quantity": float(s["quantity"]), "unit_price": float(s["unit_price"])}
        )

    total_realized = Decimal("0")
    for asset_id, data in by_asset.items():
        # FIFO: sort purchases by date, sales by date
        purs = sorted(data["purchases"], key=lambda x: x["date"])
        sals = sorted(data["sales"], key=lambda x: x["date"])
        # Queue of (remaining_qty, unit_price)
        queue: List[tuple] = []
        for p in purs:
            queue.append([round_3dec(p["quantity"]), round_3dec(p["unit_price"])])

        for s in sals:
            qty_left = round_3dec(s["quantity"])
            sale_price = round_3dec(s["unit_price"])
            while qty_left > 0 and queue:
                avail_qty, cost_basis = queue[0][0], queue[0][1]
                take = min(avail_qty, qty_left)
                gain = take * (sale_price - cost_basis)
                total_realized += Decimal(str(round_3dec(gain)))
                queue[0][0] -= take
                if queue[0][0] <= 0:
                    queue.pop(0)
                qty_left -= take

    # "Results should be rounded to the nearest Avalon dollar" for Net Capital Gain
    return float(total_realized.quantize(Decimal("1"), rounding=ROUND_HALF_UP))


# --- Federal tax ---


def _ewma(prior_five: List[float]) -> float:
    """E1 = Y_{t-5}, E_k = alpha*Y_{t-(6-k)} + (1-alpha)*E_{k-1} for k=2..5. Returns E5."""
    if len(prior_five) != 5:
        return 0.0
    e = prior_five[0]  # E1 = Y_{t-5}
    alpha = EWMA_ALPHA
    for k in range(2, 6):
        y = prior_five[k - 1]  # Y_{t-(6-k)}: k=2->Y_{t-4}, k=5->Y_{t-1}
        e = alpha * y + (1 - alpha) * e
    return e


def _bracket_tax(income: float, brackets: List[tuple]) -> float:
    """Marginal bracket tax. income >= 0. brackets = [(low, high, rate), ...]."""
    if income <= 0:
        return 0.0
    tax = 0.0
    for low, high, rate in brackets:
        if income <= low:
            break
        band = min(income, high) - low
        if band > 0:
            tax += band * rate
    return tax


def _federal_tax_one(
    gross_income: float,
    taxable_income: float,
    prior_five: List[float],
) -> tuple[float, bool]:
    """
    Returns (national_tax, surcharge_applies).
    National = bracket tax on taxable_income + surcharge on gross if EWMA > 1M.
    """
    bracket_tax = _bracket_tax(taxable_income, FEDERAL_BRACKETS)
    e5 = _ewma(prior_five)
    surcharge_applies = e5 > SURCHARGE_THRESHOLD
    surcharge = (SURCHARGE_RATE * gross_income) if surcharge_applies else 0.0
    national = bracket_tax + surcharge
    return national, surcharge_applies


def _itemized_deduction(
    gross_income: float,
    charitable_donations: List[float],
    num_children: int,
) -> float:
    """
    Charitable + Child deduction. Child % is applied to (gross - charitable).
    Child: 1% + 2% + ... + min(10, num_children)% of that base.
    """
    charitable = sum(float(d) for d in charitable_donations) if charitable_donations else 0.0
    base_for_child = max(0.0, gross_income - charitable)
    n = min(10, max(0, num_children))
    child_pct = (n * (n + 1) / 2) / 100.0  # 1%+2%+...+n%
    child_ded = base_for_child * child_pct
    return charitable + child_ded


def federal_tax(
    gross_income: float,
    charitable_donations: List[float],
    num_children: int,
    prior_five: List[float],
) -> tuple[float, float]:
    """
    Taxpayer chooses standard or itemized to minimize federal tax.
    Returns (national_tax, taxable_income_used).
    """
    # Standard
    taxable_std = max(0.0, gross_income - STANDARD_DEDUCTION)
    tax_std, _ = _federal_tax_one(gross_income, taxable_std, prior_five)

    # Itemized
    itemized = _itemized_deduction(gross_income, charitable_donations, num_children)
    taxable_item = max(0.0, gross_income - itemized)
    tax_item, _ = _federal_tax_one(gross_income, taxable_item, prior_five)

    if tax_std <= tax_item:
        national = tax_std
        taxable_used = taxable_std
    else:
        national = tax_item
        taxable_used = taxable_item

    return national, taxable_used


# --- State tax ---


def state_tax_ca(
    w2_income: float,
    net_capital_gain: float,
    gross_income: float,
    federal_surcharge_applies: bool,
) -> float:
    """CA: 4% wage + 6% max(0, net cap gain). If federal surcharge: +5% of gross."""
    tax = CA_WAGE_RATE * w2_income + CA_CAPGAIN_RATE * max(0.0, net_capital_gain)
    if federal_surcharge_applies:
        tax += CA_SURCHARGE_RATE * gross_income
    return tax


def state_tax_tx(
    taxable_income: float,
    federal_deduction_amount: float,
) -> float:
    """TX: marginal on taxable income. If federal deductions > 15000, 1% off state tax."""
    tax = _bracket_tax(taxable_income, TX_BRACKETS)
    if federal_deduction_amount > TX_DEDUCTION_THRESHOLD:
        tax *= 1.0 - TX_DEDUCTION_RATE
    return tax


# --- Per-household pipeline ---


def process_household(obj: dict) -> dict:
    """
    One household -> { taxpayer_id, federal_tax, state_tax } (whole dollars).
    """
    taxpayer_id = obj["taxpayer_id"]
    state = obj["state"]
    w2_income = float(obj["w2_income"])
    num_children = int(obj["num_children"])
    prior_five = [float(x) for x in obj.get("prior_five_years_income", [])]
    purchases = obj.get("purchases", [])
    sales = obj.get("sales", [])
    charitable_donations = obj.get("charitable_donations", [])

    net_cap_gain = _fifo_net_capital_gain(purchases, sales)
    gross_income = w2_income + net_cap_gain

    national_tax, taxable_used = federal_tax(
        gross_income, charitable_donations, num_children, prior_five
    )
    federal_deduction_amount = gross_income - taxable_used

    # Recompute whether surcharge applies (for CA)
    e5 = _ewma(prior_five)
    surcharge_applies = e5 > SURCHARGE_THRESHOLD

    if state == "California":
        state_tax = state_tax_ca(w2_income, net_cap_gain, gross_income, surcharge_applies)
    elif state == "Texas":
        state_tax = state_tax_tx(taxable_used, federal_deduction_amount)
    else:
        raise ValueError(f"Unknown state: {state}")

    return {
        "taxpayer_id": taxpayer_id,
        "federal_tax": round_dollars(national_tax),
        "state_tax": round_dollars(state_tax),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Avalon 2026 Tax Engine (NDJSON in/out)")
    parser.add_argument("--inputFile", dest="input_file", metavar="FILE", help="Input NDJSON (spec)")
    parser.add_argument("--outputFile", dest="output_file", metavar="FILE", help="Output NDJSON (spec)")
    parser.add_argument("--input", dest="input_file_alt", metavar="FILE", help="Input NDJSON (runner)")
    parser.add_argument("--output", dest="output_file_alt", metavar="FILE", help="Output NDJSON (runner)")
    args = parser.parse_args()

    input_path = args.input_file or args.input_file_alt
    output_path = args.output_file or args.output_file_alt
    if not input_path or not output_path:
        parser.error("Must provide (--inputFile and --outputFile) or (--input and --output)")

    inp = Path(input_path)
    out = Path(output_path)
    if not inp.exists():
        print(f"Error: input file not found: {inp}", file=sys.stderr)
        return 1

    results: List[dict] = []
    with open(inp, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            obj = json.loads(line)
            try:
                results.append(process_household(obj))
            except Exception as e:
                print(f"Error processing {obj.get('taxpayer_id', '?')}: {e}", file=sys.stderr)
                raise

    with open(out, "w", encoding="utf-8") as f:
        for r in results:
            f.write(json.dumps(r) + "\n")

    return 0


if __name__ == "__main__":
    sys.exit(main())
