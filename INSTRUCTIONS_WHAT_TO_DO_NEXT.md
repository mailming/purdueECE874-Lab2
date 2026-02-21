# Lab 2: What You Need to Do Next

Review of the full assignment and your current status, with clear next steps.

---

## What the assignment requires (summary)

| Part | Required? | Main deliverables |
|------|------------|--------------------|
| **Pre-lab** | Yes (before in-class) | 2oo3 derivation; LLM setup; slide review |
| **In-class** | Yes | 3 LLM-generated implementations; agreement CSV; short report |
| **Take-home** | Optional (for “3 take-home labs”) | kooN derivation; ≥100 versions; 1–2 page write-up |

---

## What you already have

- **One working tax engine** (`tax_engine.py`) — implements full Avalon spec, runs on smoke and full test sets.
- **Runner** (`lab2-version-runner.py`) and **shim** (`run_tax_engine.bat`) for running multiple versions.
- **Test data:** smoke (200 inputs), full (~10k inputs), and outputs (`taxes.ndjson`, `taxfull.ndjson`).
- **Lab materials:** plan (`LAB2_PLAN.md`), extracted spec/slides, LaTeX write-up skeleton (`0-main.tex`, `1-pre-lab.tex`, `2-in-class.tex`, `3-take-home.tex`).
- **Repo:** Everything committed and pushed to https://github.com/mailming/purdueECE874-Lab2.

**Important:** The in-class grade requires **three distinct, LLM-generated** implementations. Your current `tax_engine.py` can count as **one** of them (e.g. “impl-1”) only if you treat it as an LLM-assisted implementation. You still need **two more** from **new LLM sessions**, with different design choices (language, structure, or numeric handling).

---

## What you need to do next (in order)

### Step 1: Finish pre-lab (before in-class)

1. **2oo3 derivation (required deliverable)**  
   - In `1-pre-lab.tex` (or a separate page), fill in the **answer** in the yellow deliverable box:
     - **Failure cases:** 2oo3 fails when 0 or 1 version is correct.
     - **Probabilities (independence):**  
       - 0 correct: \(p^3\).  
       - 1 correct: \(3 p^2 q\) (only V1 correct, or only V2, or only V3).
     - **Closed form:**  
       \(P(\text{system failure}) = p^3 + 3p^2 q = p^2(p + 3q) = p^2(3 - 2p)\).
   - Make sure you “clearly enumerate the failure cases” and “use the independence assumption explicitly.”

2. **Review slides**  
   - Redundancy math (1oo2, independence), IEC 61508 Table A.2 row 3d (design diversity), Knight & Leveson summary. Use `redundancy_slides_extracted.txt` or the PPTX.

3. **LLM setup**  
   - Pick one: Gemini Pro, Microsoft Copilot, or local (e.g. Ollama).
   - Confirm: new session → paste spec (or chunks) → get code → run it locally on a small test.  
   - Lab warns: do **not** ask “write a program for this spec” in one shot; **break the spec down** (e.g. by section) to avoid timeouts.

4. **Experimental structure**  
   - Be clear on: frozen spec, N implementations, visible vs reserved test set, and what is measured (marginal/joint failure, voting reliability).

---

### Step 2: In-class preparation

5. **Form team**  
   - 2 people (or 3 if needed). Online: use Team project partners.  
   - **Post team in Brightspace:** Lab 2 Team Signup Discussion (names + emails).

6. **Diversity strategy (2–3 sentences)**  
   - Decide how the **three** implementations will differ, e.g.:
     - Different language (e.g. Python vs Java vs C++), or  
     - Different architecture (monolithic vs modular), or  
     - Different numeric handling/libraries.  
   - Write this as “Team Strategy” and keep it for the in-class report.

7. **Have ready**  
   - Full spec (Appendix A from lab PDF).  
   - Test Set 1 (smoke) and Test Set 2 (full) — you already have local copies.  
   - Runner and how to invoke it (e.g. `run_tax_engine.bat` for one version).

---

### Step 3: In-class execution

8. **Three implementations**  
   - **Version 1:** You may use your current `tax_engine.py` as one implementation (e.g. impl-1).  
   - **Version 2 & 3:** From **new LLM sessions**, different dimension each (language/design/numeric). Break the spec into parts when prompting.  
   - Each must: read NDJSON input, write NDJSON output, support `--input`/`--output` (and/or `--inputFile`/`--outputFile`).  
   - Tip: Get two implementations working and do a 1oo2 check before building the third.

9. **Smoke test (Test Set 1)**  
   - Run all three on Test Set 1.  
   - Fix until: no crashes, valid NDJSON, and (ideally) unanimous agreement or at least 2oo3 on every case.

10. **Full run and agreement table (Test Set 2)**  
    - Use the runner to run all three on Test Set 2 (~10k inputs).  
    - Build **agreement table:** for each input, record outputs of V1, V2, V3 (e.g. federal+state summed per lab note).  
    - Save as CSV: `input_id`, `out_v1`, `out_v2`, `out_v3`.

11. **Analysis and report**  
    - Compute: fraction unanimous (3/3), fraction 2–1 split, fraction multi-way divergence; per version how often it disagrees with the other two.  
    - Discuss: isolated vs clustered disagreements; same version often the outlier? Independence plausible or correlated?  
    - Include at least one table or figure.

12. **In-class submission**  
    - **impl-1.zip**, **impl-2.zip**, **impl-3.zip** (each with one implementation + anything needed to run it).  
    - **CSV** (agreement table).  
    - **Short report** (analysis + table/figure).  
    Submit per course/Brightspace instructions.

---

### Step 4: Take-home (optional)

Only if you want this to count as one of your “3 take-home labs”:

- Derive **k-out-of-N** system failure probability (summation form, binomial); write it up.  
- Generate **≥ 100** implementations (automate with an agent or wrapper around LLM calls).  
- Measure marginal/joint failure, compare observed to binomial prediction, discuss independence and voting benefit.  
- Optional: Knight & Leveson–style statistical tests.  
- Deliver: derivation, analysis with table/figure, 1–2 page write-up (and optional stats section).

---

## Quick checklist

- [ ] **Pre-lab:** 2oo3 derivation written in 1-pre-lab.tex (or equivalent); slides reviewed; LLM chosen and tested; experimental structure understood.  
- [ ] **Team:** Formed and posted in Brightspace (Lab 2 Team Signup).  
- [ ] **Strategy:** 2–3 sentence diversity strategy written.  
- [ ] **Impl 1:** Current `tax_engine.py` packaged as impl-1 (if using as one version).  
- [ ] **Impl 2 & 3:** Two more implementations from new LLM sessions, different dimensions.  
- [ ] **Test Set 1:** All three pass smoke (no crash, valid output, unanimous or 2oo3).  
- [ ] **Test Set 2:** Runner used; agreement CSV produced.  
- [ ] **Report:** Agreement stats + discussion + table/figure.  
- [ ] **Submit:** impl-1.zip, impl-2.zip, impl-3.zip + CSV + report.

---

## Suggested immediate next actions

1. **Today:** Fill in the **2oo3 derivation** in `1-pre-lab.tex` (failure cases, independence, closed form \(p^3 + 3p^2q\)).  
2. **Before lab:** Confirm **LLM access** and do one trial: new session → spec (chunked) → code → run on a few inputs.  
3. **Before lab:** **Form team** and post on Brightspace; draft your **diversity strategy**.  
4. **During lab:** Use your existing engine as impl-1; generate and fix impl-2 and impl-3; run smoke then full test set; build CSV and short report; zip and submit.

If you tell me which step you’re on (e.g. “fill 2oo3 in LaTeX” or “package impl-1 and prepare runner for impl-2/3”), I can give exact edits or commands for that step.
