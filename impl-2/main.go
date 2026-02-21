// Avalon 2026 Tax Engine — Go implementation (impl-2).
// Reads NDJSON households, outputs NDJSON with federal_tax and state_tax (whole dollars).
// CLI: --inputFile/--outputFile or --input/--output
package main

import (
	"bufio"
	"encoding/json"
	"flag"
	"fmt"
	"math"
	"os"
	"sort"
)

const (
	standardDeduction    = 10000.0
	ewmaAlpha            = 0.6
	surchargeThreshold   = 1_000_000
	surchargeRate        = 0.02
	caWageRate           = 0.04
	caCapGainRate        = 0.06
	caSurchargeRate      = 0.05
	txDeductionThreshold = 15_000
	txDeductionRate      = 0.01
)

var federalBrackets = []struct{ low, high, rate float64 }{
	{0, 100_000, 0.05}, {100_000, 200_000, 0.10}, {200_000, 300_000, 0.15}, {300_000, 1e18, 0.20},
}
var txBrackets = []struct{ low, high, rate float64 }{
	{0, 90_000, 0.03}, {90_000, 200_000, 0.05}, {200_000, 1e18, 0.07},
}

type purchaseSale struct {
	AssetID  string  `json:"asset_id"`
	Date     string  `json:"date"`
	Quantity float64 `json:"quantity"`
	UnitPrice float64 `json:"unit_price"`
}

type household struct {
	TaxpayerID           string          `json:"taxpayer_id"`
	State                string          `json:"state"`
	W2Income             float64        `json:"w2_income"`
	NumChildren          int             `json:"num_children"`
	PriorFiveYearsIncome []float64       `json:"prior_five_years_income"`
	Purchases            []purchaseSale  `json:"purchases"`
	Sales                []purchaseSale  `json:"sales"`
	CharitableDonations  []float64       `json:"charitable_donations"`
}

type outputRow struct {
	TaxpayerID string `json:"taxpayer_id"`
	FederalTax int    `json:"federal_tax"`
	StateTax   int    `json:"state_tax"`
}

func round3(x float64) float64 { return math.Round(x*1000) / 1000 }
func roundDollars(x float64) int { return int(math.Round(x)) }

func fifoNetCapitalGain(purchases, sales []purchaseSale) float64 {
	byAsset := make(map[string]struct {
		purchases []purchaseSale
		sales     []purchaseSale
	})
	for _, p := range purchases {
		byAsset[p.AssetID] = struct {
			purchases []purchaseSale
			sales     []purchaseSale
		}{append(byAsset[p.AssetID].purchases, p), byAsset[p.AssetID].sales}
	}
	for _, s := range sales {
		t := byAsset[s.AssetID]
		t.sales = append(t.sales, s)
		byAsset[s.AssetID] = t
	}
	var total float64
	for _, data := range byAsset {
		pu := make([]purchaseSale, len(data.purchases))
		copy(pu, data.purchases)
		sort.Slice(pu, func(i, j int) bool { return pu[i].Date < pu[j].Date })
		sa := make([]purchaseSale, len(data.sales))
		copy(sa, data.sales)
		sort.Slice(sa, func(i, j int) bool { return sa[i].Date < sa[j].Date })
		type lot struct{ qty, price float64 }
		var queue []lot
		for _, p := range pu {
			queue = append(queue, lot{round3(p.Quantity), round3(p.UnitPrice)})
		}
		for _, s := range sa {
			qtyLeft := round3(s.Quantity)
			salePrice := round3(s.UnitPrice)
			for qtyLeft > 0 && len(queue) > 0 {
				take := qtyLeft
				if queue[0].qty < take {
					take = queue[0].qty
				}
				gain := take * (salePrice - queue[0].price)
				total += round3(gain)
				queue[0].qty -= take
				if queue[0].qty <= 0 {
					queue = queue[1:]
				}
				qtyLeft -= take
			}
		}
	}
	return math.Round(total)
}

func ewma(prior []float64) float64 {
	if len(prior) != 5 {
		return 0
	}
	e := prior[0]
	for k := 1; k < 5; k++ {
		e = ewmaAlpha*prior[k] + (1-ewmaAlpha)*e
	}
	return e
}

func bracketTax(income float64, brackets []struct{ low, high, rate float64 }) float64 {
	if income <= 0 {
		return 0
	}
	var tax float64
	for _, b := range brackets {
		if income <= b.low {
			break
		}
		band := income - b.low
		if band > b.high-b.low {
			band = b.high - b.low
		}
		if band > 0 {
			tax += band * b.rate
		}
	}
	return tax
}

func itemizedDeduction(gross float64, donations []float64, numChildren int) float64 {
	var charitable float64
	for _, d := range donations {
		charitable += d
	}
	base := gross - charitable
	if base < 0 {
		base = 0
	}
	n := numChildren
	if n > 10 {
		n = 10
	}
	if n < 0 {
		n = 0
	}
	childPct := float64(n*(n+1)/2) / 100.0
	return charitable + base*childPct
}

func federalTax(gross float64, donations []float64, numChildren int, prior []float64) (nationalTax, taxableUsed float64) {
	taxableStd := gross - standardDeduction
	if taxableStd < 0 {
		taxableStd = 0
	}
	taxStd := bracketTax(taxableStd, federalBrackets)
	e5 := ewma(prior)
	if e5 > surchargeThreshold {
		taxStd += surchargeRate * gross
	}

	itemized := itemizedDeduction(gross, donations, numChildren)
	taxableItem := gross - itemized
	if taxableItem < 0 {
		taxableItem = 0
	}
	taxItem := bracketTax(taxableItem, federalBrackets)
	if e5 > surchargeThreshold {
		taxItem += surchargeRate * gross
	}

	if taxStd <= taxItem {
		return taxStd, taxableStd
	}
	return taxItem, taxableItem
}

func stateTaxCA(w2, netCapGain, gross float64, surchargeApplies bool) float64 {
	tax := caWageRate*w2 + caCapGainRate*math.Max(0, netCapGain)
	if surchargeApplies {
		tax += caSurchargeRate * gross
	}
	return tax
}

func stateTaxTX(taxable, federalDeductionAmt float64) float64 {
	tax := bracketTax(taxable, txBrackets)
	if federalDeductionAmt > txDeductionThreshold {
		tax *= 1 - txDeductionRate
	}
	return tax
}

func processHousehold(h household) (outputRow, error) {
	netCapGain := fifoNetCapitalGain(h.Purchases, h.Sales)
	gross := h.W2Income + netCapGain
	nationalTax, taxableUsed := federalTax(gross, h.CharitableDonations, h.NumChildren, h.PriorFiveYearsIncome)
	federalDedAmt := gross - taxableUsed
	surchargeApplies := ewma(h.PriorFiveYearsIncome) > surchargeThreshold

	var stateTax float64
	switch h.State {
	case "California":
		stateTax = stateTaxCA(h.W2Income, netCapGain, gross, surchargeApplies)
	case "Texas":
		stateTax = stateTaxTX(taxableUsed, federalDedAmt)
	default:
		return outputRow{}, fmt.Errorf("unknown state: %s", h.State)
	}
	return outputRow{
		TaxpayerID: h.TaxpayerID,
		FederalTax: roundDollars(nationalTax),
		StateTax:   roundDollars(stateTax),
	}, nil
}

func main() {
	var inputFile, outputFile, inputAlt, outputAlt string
	flag.StringVar(&inputFile, "inputFile", "", "input NDJSON (spec)")
	flag.StringVar(&outputFile, "outputFile", "", "output NDJSON (spec)")
	flag.StringVar(&inputAlt, "input", "", "input NDJSON (runner)")
	flag.StringVar(&outputAlt, "output", "", "output NDJSON (runner)")
	flag.Parse()
	inPath := inputFile
	if inPath == "" {
		inPath = inputAlt
	}
	outPath := outputFile
	if outPath == "" {
		outPath = outputAlt
	}
	if inPath == "" || outPath == "" {
		fmt.Fprintln(os.Stderr, "usage: need (--inputFile and --outputFile) or (--input and --output)")
		os.Exit(1)
	}
	in, err := os.Open(inPath)
	if err != nil {
		fmt.Fprintf(os.Stderr, "open input: %v\n", err)
		os.Exit(1)
	}
	defer in.Close()
	out, err := os.Create(outPath)
	if err != nil {
		fmt.Fprintf(os.Stderr, "create output: %v\n", err)
		os.Exit(1)
	}
	defer out.Close()
	enc := json.NewEncoder(out)
	enc.SetEscapeHTML(false)
	sc := bufio.NewScanner(in)
	for sc.Scan() {
		line := sc.Bytes()
		if len(line) == 0 {
			continue
		}
		var h household
		if err := json.Unmarshal(line, &h); err != nil {
			fmt.Fprintf(os.Stderr, "decode: %v\n", err)
			os.Exit(1)
		}
		row, err := processHousehold(h)
		if err != nil {
			fmt.Fprintf(os.Stderr, "process %s: %v\n", h.TaxpayerID, err)
			os.Exit(1)
		}
		if err := enc.Encode(row); err != nil {
			fmt.Fprintf(os.Stderr, "encode: %v\n", err)
			os.Exit(1)
		}
	}
	if err := sc.Err(); err != nil {
		fmt.Fprintf(os.Stderr, "read: %v\n", err)
		os.Exit(1)
	}
}
