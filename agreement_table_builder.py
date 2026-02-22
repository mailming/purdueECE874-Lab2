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
    if len(sys.argv) != 5:
        print("Usage:")
        print("python build_agreement_table.py v1.ndjson v2.ndjson v3.ndjson output.csv")
        sys.exit(1)

    v1_file = sys.argv[1]
    v2_file = sys.argv[2]
    v3_file = sys.argv[3]
    out_csv = sys.argv[4]

    v1 = load_totals(v1_file)
    v2 = load_totals(v2_file)
    v3 = load_totals(v3_file)

    with open(out_csv, "w", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(["taxpayer_id", "out_v1", "out_v2", "out_v3"])

        for taxpayer_id in v1:
            writer.writerow([
                taxpayer_id,
                v1.get(taxpayer_id, ""),
                v2.get(taxpayer_id, ""),
                v3.get(taxpayer_id, "")
            ])

    print("Agreement table written to", out_csv)


if __name__ == "__main__":
    main()
