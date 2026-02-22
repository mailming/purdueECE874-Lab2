// Avalon 2026 Tax Engine — C++ implementation (impl-3).
// Reads NDJSON households, outputs NDJSON with federal_tax and state_tax (whole dollars).
// CLI: --inputFile/--outputFile or --input/--output
// Build: g++ -std=c++17 -O2 -o tax_engine main.cpp
// Requires: nlohmann/json (single header at nlohmann/json.hpp)

#include "nlohmann/json.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace {
constexpr double STANDARD_DEDUCTION = 10000.0;
constexpr double EWMA_ALPHA = 0.6;
constexpr double SURCHARGE_THRESHOLD = 1e6;
constexpr double SURCHARGE_RATE = 0.02;
constexpr double CA_WAGE_RATE = 0.04;
constexpr double CA_CAPGAIN_RATE = 0.06;
constexpr double CA_SURCHARGE_RATE = 0.05;
constexpr double TX_DEDUCTION_THRESHOLD = 15000.0;
constexpr double TX_DEDUCTION_RATE = 0.01;

struct Bracket { double low, high, rate; };
const std::vector<Bracket> FEDERAL_BRACKETS = {
    {0, 100000, 0.05}, {100000, 200000, 0.10}, {200000, 300000, 0.15}, {300000, 1e18, 0.20}
};
const std::vector<Bracket> TX_BRACKETS = {
    {0, 90000, 0.03}, {90000, 200000, 0.05}, {200000, 1e18, 0.07}
};

double round3(double x) { return std::round(x * 1000) / 1000; }
int roundDollars(double x) { return static_cast<int>(std::round(x)); }

using DateQtyPrice = std::pair<std::string, std::pair<double, double>>;
double fifoNetCapitalGain(const json& purchases, const json& sales) {
    std::map<std::string, std::pair<std::vector<DateQtyPrice>, std::vector<DateQtyPrice>>> byAsset;
    for (const auto& p : purchases) {
        std::string id = p["asset_id"].get<std::string>();
        byAsset[id].first.push_back({ p["date"].get<std::string>(), { round3(p["quantity"].get<double>()), round3(p["unit_price"].get<double>()) }});
    }
    for (const auto& s : sales) {
        std::string id = s["asset_id"].get<std::string>();
        byAsset[id].second.push_back({ s["date"].get<std::string>(), { round3(s["quantity"].get<double>()), round3(s["unit_price"].get<double>()) }});
    }
    double total = 0;
    for (auto& [asset, data] : byAsset) {
        auto& purs = data.first;
        auto& sals = data.second;
        std::sort(purs.begin(), purs.end());
        std::sort(sals.begin(), sals.end());
        std::vector<std::pair<double, double>> queue;
        for (const auto& pr : purs) queue.push_back(pr.second);
        for (const auto& sl : sals) {
            double qtyLeft = sl.second.first, salePrice = sl.second.second;
            while (qtyLeft > 0 && !queue.empty()) {
                double take = std::min(queue[0].first, qtyLeft);
                total += round3(take * (salePrice - queue[0].second));
                queue[0].first -= take;
                if (queue[0].first <= 0) queue.erase(queue.begin());
                qtyLeft -= take;
            }
        }
    }
    return std::round(total);
}

double ewma(const std::vector<double>& prior) {
    if (prior.size() != 5) return 0;
    double e = prior[0];
    for (size_t k = 1; k < 5; ++k) e = EWMA_ALPHA * prior[k] + (1 - EWMA_ALPHA) * e;
    return e;
}

double bracketTax(double income, const std::vector<Bracket>& brackets) {
    if (income <= 0) return 0;
    double tax = 0;
    for (const auto& b : brackets) {
        if (income <= b.low) break;
        double band = std::min(income, b.high) - b.low;
        if (band > 0) tax += band * b.rate;
    }
    return tax;
}

double itemizedDeduction(double gross, const std::vector<double>& donations, int numChildren) {
    double charitable = 0;
    for (double d : donations) charitable += d;
    double base = std::max(0.0, gross - charitable);
    int n = std::min(10, std::max(0, numChildren));
    double childPct = (n * (n + 1) / 2.0) / 100.0;
    return charitable + base * childPct;
}

std::pair<double, double> federalTax(double gross, const std::vector<double>& donations, int numChildren, const std::vector<double>& prior) {
    double taxableStd = std::max(0.0, gross - STANDARD_DEDUCTION);
    double taxStd = bracketTax(taxableStd, FEDERAL_BRACKETS);
    if (ewma(prior) > SURCHARGE_THRESHOLD) taxStd += SURCHARGE_RATE * gross;

    double itemized = itemizedDeduction(gross, donations, numChildren);
    double taxableItem = std::max(0.0, gross - itemized);
    double taxItem = bracketTax(taxableItem, FEDERAL_BRACKETS);
    if (ewma(prior) > SURCHARGE_THRESHOLD) taxItem += SURCHARGE_RATE * gross;

    if (taxStd <= taxItem) return { taxStd, taxableStd };
    return { taxItem, taxableItem };
}

double stateTaxCA(double w2, double netCapGain, double gross, bool surchargeApplies) {
    double tax = CA_WAGE_RATE * w2 + CA_CAPGAIN_RATE * std::max(0.0, netCapGain);
    if (surchargeApplies) tax += CA_SURCHARGE_RATE * gross;
    return tax;
}

double stateTaxTX(double taxable, double federalDedAmt) {
    double tax = bracketTax(taxable, TX_BRACKETS);
    if (federalDedAmt > TX_DEDUCTION_THRESHOLD) tax *= 1 - TX_DEDUCTION_RATE;
    return tax;
}

json processHousehold(const json& h) {
    std::string state = h["state"].get<std::string>();
    double w2 = h["w2_income"].get<double>();
    int numChildren = h["num_children"].get<int>();
    std::vector<double> prior;
    for (const auto& v : h["prior_five_years_income"]) prior.push_back(v.get<double>());
    std::vector<double> donations;
    if (h.contains("charitable_donations"))
        for (const auto& v : h["charitable_donations"]) donations.push_back(v.get<double>());
    json purchases = h.contains("purchases") ? h["purchases"] : json::array();
    json sales = h.contains("sales") ? h["sales"] : json::array();

    double netCapGain = fifoNetCapitalGain(purchases, sales);
    double gross = w2 + netCapGain;
    auto [nationalTax, taxableUsed] = federalTax(gross, donations, numChildren, prior);
    double federalDedAmt = gross - taxableUsed;
    bool surchargeApplies = ewma(prior) > SURCHARGE_THRESHOLD;

    double stateTax = 0;
    if (state == "California") stateTax = stateTaxCA(w2, netCapGain, gross, surchargeApplies);
    else if (state == "Texas") stateTax = stateTaxTX(taxableUsed, federalDedAmt);
    else { std::cerr << "Unknown state: " << state << std::endl; std::exit(1); }

    return json::object({
        {"taxpayer_id", h["taxpayer_id"]},
        {"federal_tax", roundDollars(nationalTax)},
        {"state_tax", roundDollars(stateTax)}
    });
}
} // namespace

int main(int argc, char** argv) {
    std::string inPath, outPath;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if ((arg == "--input" || arg == "--inputFile") && i + 1 < argc) inPath = argv[++i];
        else if ((arg == "--output" || arg == "--outputFile") && i + 1 < argc) outPath = argv[++i];
    }
    if (inPath.empty() || outPath.empty()) {
        std::cerr << "Usage: need (--inputFile and --outputFile) or (--input and --output)\n";
        return 1;
    }
    std::ifstream in(inPath);
    if (!in) { std::cerr << "Cannot open input: " << inPath << "\n"; return 1; }
    std::ofstream out(outPath);
    if (!out) { std::cerr << "Cannot open output: " << outPath << "\n"; return 1; }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        json h = json::parse(line);
        json row = processHousehold(h);
        out << row.dump() << "\n";
    }
    return 0;
}
