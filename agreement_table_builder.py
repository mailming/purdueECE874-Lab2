import json
import csv
import sys


def load_totals(filename):
    totals = {}
    with open(filename, "r") as f:
        for line in f:
            if not line.strip():
                continue
            obj = json.loads(line)
            total = obj["federal_tax"] + obj["state_tax"]
            totals[obj["taxpayer_id"]] = total
    return totals


def main():
    if len(sys.argv) < 4:
        print("Usage:")
        print("  python agreement_table_builder.py v1.ndjson v2.ndjson output.csv [v3.ndjson]")
        print("  (v3.ndjson optional; if omitted, out_v3 column is empty)")
        sys.exit(1)

    v1_file = sys.argv[1]
    v2_file = sys.argv[2]
    out_csv = sys.argv[3]
    v3_file = sys.argv[4] if len(sys.argv) > 4 else None

    v1 = load_totals(v1_file)
    v2 = load_totals(v2_file)
    v3 = load_totals(v3_file) if v3_file else {}

    # Preserve order from v1 (matches input order)
    all_ids = list(v1.keys())

    with open(out_csv, "w", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(["taxpayer_id", "out_v1", "out_v2", "out_v3"])

        for taxpayer_id in all_ids:
            writer.writerow([
                taxpayer_id,
                v1.get(taxpayer_id, ""),
                v2.get(taxpayer_id, ""),
                v3.get(taxpayer_id, "") if v3 else ""
            ])

    print("Agreement table written to", out_csv)


if __name__ == "__main__":
    main()
