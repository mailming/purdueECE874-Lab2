# Lab 2: What to Do Next — Plan

Based on the lab handout and the redundancy lecture slides, follow this order.

---

## Phase 1: Pre-Lab (do before the in-class session)

### 1.1 Review lecture slides ✓ (you have the deck)
- [ ] **Probabilistic argument:** Independence → P(F_i ∩ F_j) = P(F_i)P(F_j); 1oo2 → P(system failure) = p²
- [ ] **1oo2 derivation** (Slide 7): redundant compute, PFH, SIL 1/2/3
- [ ] **IEC 61508 Part 3, Table A.2, row 3d:** Design diversity = same function, different design
- [ ] **Knight & Leveson (Slides 12–13):** Eckhardt et al. 1990; independence assumption questioned in software

### 1.2 Derive 2oo3 reliability formula (required deliverable)
- [ ] **Failure cases for 2oo3:** System fails iff fewer than 2 versions are correct.
  - So: exactly 0 correct, or exactly 1 correct.
- [ ] **Probabilities under independence:**
  - 0 correct: p³  
  - 1 correct: 3 · p²q (three ways: V1 only, V2 only, V3 only)
- [ ] **Closed form:** P(system failure) = p³ + 3p²q = p²(p + 3q) = p²(3 − 2p)  
  (or equivalent simplified form)
- [ ] Write up: enumerate cases, state independence, give final formula

### 1.3 Set up LLM environment
- [ ] Choose one: **Gemini Pro** | **Microsoft Copilot** | **open-weight (e.g. Ollama)**
- [ ] Confirm: start **fresh session** per “version”
- [ ] Confirm: give a **spec** → get **source code**
- [ ] Confirm: **run generated code locally** on a small test

### 1.4 Review experimental structure
- [ ] Frozen spec (no changes)
- [ ] N implementations (3 in-class; ≥100 take-home)
- [ ] **Visible** test suite during dev; **reserved** test set for final eval
- [ ] Metrics: marginal failure p, joint failure, voting reliability; where independence enters the math

---

## Phase 2: In-class preparation (before / at start of lab)

### 2.1 Team
- [ ] Form team (2 students; 3 if necessary). **Online:** use Team project partners.
- [ ] Post team in **Brightspace:** Lab 2 Team Signup Discussion (names + emails)

### 2.2 Diversity strategy (2–3 sentences)
- [ ] Decide how to **vary** the three implementations, e.g.:
  - Different **language** (e.g. Python vs Java vs C++)
  - Different **architecture** (monolithic vs modular vs pipeline)
  - Different **numeric handling** or **libraries**
- [ ] Write short “Team Strategy” describing how you induce diversity and what might affect failure behavior (floating-point, thresholds, etc.)

### 2.3 Materials to have
- [ ] **Spec:** Appendix A (Avalon tax code) from lab PDF — have it ready to paste / attach in full
- [ ] **Test Set 1** (smoke) and **Test Set 2** (~10k) from Brightspace
- [ ] **Runner** program from Brightspace (to run multiple implementations and collect results)
- [ ] Lab note: Don’t ask the LLM to “write a program for this spec” in one shot; **break the spec down** (e.g. by section or component) to avoid timeouts/failures

---

## Phase 3: In-class execution

### 3.1 Generate three implementations
- [ ] **Impl 1:** New session → full spec (or chunked) → full implementation → runs locally
- [ ] **Impl 2:** New session, different dimension (language/design/numeric) → runs locally
- [ ] **Impl 3:** New session, again different → runs locally  
  **Tip:** Do impl 1 and 2 first, run 1oo2 check, then do impl 3.

### 3.2 Smoke test (Test Set 1)
- [ ] Run all three on Test Set 1
- [ ] Fix until: no crashes, well-formed NDJSON output, (ideally) unanimous agreement or at least 2oo3 on every case

### 3.3 Independence analysis (Test Set 2)
- [ ] Run all three on Test Set 2 using the **runner**
- [ ] Build **agreement table:** one row per input → (out_v1, out_v2, out_v3)
- [ ] Compute and report:
  - Fraction unanimous (3/3)
  - Fraction 2–1 split
  - Fraction multi-way divergence
  - Per version: how often it disagrees with the other two
- [ ] Brief discussion: isolated vs clustered disagreements; same version often outlier? Independence plausible or correlated?

### 3.4 In-class deliverables
- [ ] **impl-1.zip**, **impl-2.zip**, **impl-3.zip**
- [ ] **CSV:** columns `input_id`, `out_v1`, `out_v2`, `out_v3` (can use federal+state summed per lab note)
- [ ] **Short report:** agreement/disagreement patterns + at least one table or figure

---

## Phase 4: Take-home (optional — for “complete 3 take-home labs”)

### 4.1 Theory: k-out-of-N model
- [ ] Define X = number of successful versions out of N → X ~ Binomial(N, q)
- [ ] kooN fails iff X < k
- [ ] Derive P(system failure) = Σ_{i=0}^{k-1} P(X = i) in summation form with binomial coefficients; state where independence is used

### 4.2 Scale up
- [ ] Generate **≥ 100** implementations (same frozen spec, fresh session each, runnable)
- [ ] **Automate:** agent or wrapper around LLM calls (lab suggests this for N = 100)

### 4.3 Measure & compare
- [ ] For each input: which versions wrong → estimate **p**, pairwise joint failure, empirical failure distribution
- [ ] Predicted P(system failure) under independence (your formula) vs **observed** kooN failure rate
- [ ] Discuss: consistent with independence? Does voting help? How much deviation? Why?

### 4.4 Optional: Knight & Leveson–style statistics
- [ ] Test if joint failure rate significantly exceeds p² (or kooN prediction)
- [ ] Hypothesis tests of independence; correlation between version failure indicators; confidence intervals
- [ ] State H0, test, assumptions, interpretation

### 4.5 Take-home deliverables
- [ ] kooN derivation
- [ ] Analysis with ≥1 table or figure
- [ ] 1–2 page write-up (and optional statistical section)

---

## Quick reference

| Item | Source |
|------|--------|
| 1oo2 formula | Slides (redundancy math); lab: P(system failure) = p² |
| 2oo3 derivation | Pre-lab deliverable; failure cases = 0 or 1 correct |
| Design diversity | IEC 61508 Part 3 Table A.2 row 3d; Slides 6, 8, 9 |
| Knight & Leveson | Slides 12–13; Eckhardt et al. 1990, NASA 102613 |
| Tax spec | Lab Appendix A (federal + CA/TX, NDJSON, CLI) |
| Test sets & runner | Brightspace |
| Sample prompts | Lab Appendix B |

---

**Suggested next step:** Complete **Phase 1** (pre-lab), especially the **2oo3 derivation** and **LLM setup**, then form your team and draft your **diversity strategy** (Phase 2) so you’re ready when the in-class lab starts.
