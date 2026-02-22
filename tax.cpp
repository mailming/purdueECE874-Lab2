#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

/* ===================== Utilities ===================== */

long long round_dollar(double x) {
    return llround(x);
}

double compute_progressive_tax(double income,
                                const vector<pair<double,double>>& brackets) {
    double tax = 0.0;
    double prev_limit = 0.0;

    for (const auto& [limit, rate] : brackets) {
        if (income <= prev_limit) break;
        double taxable = min(income, limit) - prev_limit;
        if (taxable > 0)
            tax += taxable * rate;
        prev_limit = limit;
    }
    return tax;
}

/* ===================== Capital Gains (FIFO) ===================== */

double compute_net_capital_gain(const json& record) {
    unordered_map<string, queue<pair<double,double>>> inventory;

    if (record.contains("purchases")) {
        for (const auto& p : record["purchases"]) {
            string asset = p["asset_id"];
            double qty = p["quantity"];
            double price = p["unit_price"];
            inventory[asset].push({qty, price});
        }
    }

    double total_gain = 0.0;

    if (record.contains("sales")) {
        for (const auto& s : record["sales"]) {
            string asset = s["asset_id"];
            double qty = s["quantity"];
            double sale_price = s["unit_price"];

            while (qty > 1e-9 && !inventory[asset].empty()) {
                auto& front = inventory[asset].front();
                double available = front.first;
                double buy_price = front.second;

                double used = min(qty, available);

                total_gain += used * (sale_price - buy_price);

                front.first -= used;
                qty -= used;

                if (front.first <= 1e-9)
                    inventory[asset].pop();
            }
        }
    }

    return total_gain;
}

/* ===================== EWMA ===================== */

double compute_ewma(const vector<double>& history) {
    const double alpha = 0.6;
    double E = history[0];

    for (int k = 1; k < 5; ++k)
        E = alpha * history[k] + (1 - alpha) * E;

    return E;
}

/* ===================== Federal Tax ===================== */

double compute_federal_tax(const json& record,
                           double gross_income,
                           double net_cap_gain,
                           bool& surcharge_applies,
                           double& total_deductions_used) {

    double charitable = 0.0;
    for (double d : record["charitable_donations"])
        charitable += d;

    int children = record["num_children"];

    /* ---------- STANDARD DEDUCTION ---------- */
    double std_taxable = max(0.0, gross_income - 10000.0);

    /* ---------- ITEMIZED ---------- */
    double taxable_before_child = max(0.0, gross_income - charitable);

    double child_percent = 0.0;
    for (int i = 1; i <= min(children,10); ++i)
        child_percent += i / 100.0;

    double child_deduction = taxable_before_child * child_percent;

    double itemized_taxable =
        max(0.0, taxable_before_child - child_deduction);

    /* ---------- Brackets ---------- */
    vector<pair<double,double>> fed_brackets = {
        {100000, 0.05},
        {200000, 0.10},
        {300000, 0.15},
        {1e18,   0.20}
    };

    double std_tax = compute_progressive_tax(std_taxable, fed_brackets);
    double item_tax = compute_progressive_tax(itemized_taxable, fed_brackets);

    double bracket_tax;
    double chosen_taxable;

    if (std_tax <= item_tax) {
        bracket_tax = std_tax;
        chosen_taxable = std_taxable;
        total_deductions_used = min(gross_income, 10000.0);
    } else {
        bracket_tax = item_tax;
        chosen_taxable = itemized_taxable;
        total_deductions_used =
            min(gross_income, charitable + child_deduction);
    }

    /* ---------- EWMA Surcharge ---------- */
    vector<double> hist = record["prior_five_years_income"];
    double ewma = compute_ewma(hist);

    double surcharge = 0.0;
    surcharge_applies = false;

    if (ewma > 1000000.0) {
        surcharge = 0.02 * gross_income;
        surcharge_applies = true;
    }

    return bracket_tax + surcharge;
}

/* ===================== State Tax ===================== */

double compute_state_tax(const json& record,
                         double gross_income,
                         double net_cap_gain,
                         double taxable_income,
                         bool surcharge_applies,
                         double total_deductions_used) {

    string state = record["state"];
    double w2 = record["w2_income"];

    if (state == "California") {
        double tax = 0.04 * w2 +
                     0.06 * max(0.0, net_cap_gain);

        if (surcharge_applies)
            tax *= 1.05;

        return tax;
    }

    else {  // Texas
        vector<pair<double,double>> tx_brackets = {
            {90000, 0.03},
            {200000, 0.05},
            {1e18, 0.07}
        };

        double tax = compute_progressive_tax(taxable_income, tx_brackets);

        if (total_deductions_used > 15000.0)
            tax *= 0.99;

        return tax;
    }
}

/* ===================== Main ===================== */

int main(int argc, char* argv[]) {

    string inputFile, outputFile;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--input" && i+1 < argc)
            inputFile = argv[++i];
        else if (arg == "--output" && i+1 < argc)
            outputFile = argv[++i];
    }

    if (inputFile.empty() || outputFile.empty()) {
        cerr << "Usage: ./tax --input in.ndjson --output out.ndjson\n";
        return 1;
    }

    ifstream in(inputFile);
    ofstream out(outputFile);

    if (!in || !out) {
        cerr << "File error\n";
        return 1;
    }

    string line;

    while (getline(in, line)) {
        if (line.empty()) continue;

        json record = json::parse(line);

        double w2 = record["w2_income"];
        double net_cap_gain = compute_net_capital_gain(record);
        double gross_income = w2 + net_cap_gain;

        bool surcharge_applies = false;
        double total_deductions_used = 0.0;

        double federal_tax =
            compute_federal_tax(record,
                                gross_income,
                                net_cap_gain,
                                surcharge_applies,
                                total_deductions_used);

        double charitable = 0.0;
        for (double d : record["charitable_donations"])
            charitable += d;

        double taxable_income =
            max(0.0, gross_income - total_deductions_used);

        double state_tax =
            compute_state_tax(record,
                              gross_income,
                              net_cap_gain,
                              taxable_income,
                              surcharge_applies,
                              total_deductions_used);

        json result;
        result["taxpayer_id"] = record["taxpayer_id"];
        result["federal_tax"] = round_dollar(federal_tax);
        result["state_tax"]   = round_dollar(state_tax);

        out << result.dump() << "\n";
    }

    return 0;
}
