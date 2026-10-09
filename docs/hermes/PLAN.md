# Misty Hermes: animated, colourful, themed, and update-safe

Plan for the Hermes CLI on **E15 Misty** (Windows) and **Venus Misty** (Linux).
Status: draft for owner review, 2026-10-10. Card `H0` on [`../../KANBAN.md`](../../KANBAN.md).

## 1. What the owner asked for

- Both Hermes CLIs as animated and colourful as possible, with **themes**.
- **Loading animations**, and lots of other animations.
- ASCII art from the Misty engine in **all** generations (replies, reports).
- **Hermes skills** that use all of the art.
- Many Hermes CLI improvements that **we maintain**, and Hermes stays updatable without losing
  them. Proper coding, update and maintenance strategies.

Decisions taken (owner, 2026-10-10, picker):

| Question | Answer |
|---|---|
| How improvements survive updates | **Full fork** (`erratas/hermes-agent`) |
| Default intensity | **Maximum**, with a **calm** switch; reduced-motion, `NO_COLOR` and piped output turn it down automatically |
| First milestone | **Themes + loading animations** |
| Where this plan lives | **misty-ascii repo**, `docs/hermes/` |

## 2. What is true today (checked 2026-10-09/10)

- E15 runs Hermes from `C:\Users\Bill\.hermes\hermes-agent`, a clone whose `origin` is
  **NousResearch/hermes-agent** (not our fork). It carries local edits (at least our
  `/insights` patch from 2026-10-09, backup `insights.py.bak-mistyascii-20261009`).
- `erratas/hermes-agent` is a GitHub fork with one branch, `main`, last pushed 2026-07-30
  (upstream PR #74736). It holds no Misty work and is about 2.5 months behind.
- Venus: not inspected yet. `[NEEDS CHECK: install path, origin remote, local edits]`
- Hermes already has three layers that `hermes update` never overwrites, all under
  `~/.hermes/`: **skins** (`skins/*.yaml`: colours, light/dark palettes, spinner frames and
  "wings", banner logo and hero art, branding, per-tool emoji, prompt symbol), **plugins**
  (`plugins/`, with session/tool/LLM hooks such as `on_session_start`, `post_llm_call`), and
  **skills** (`skills/`). The TypeScript terminal UI (`ui-tui`) reads `tui-theme-boot.json`.
- `hermes update` understands forks (`hermes_cli/update_cmd.py`): when `origin` is not the
  official repo and has commits upstream lacks, it **keeps them** and does not fast-forward from
  NousResearch ("Skipping upstream sync to preserve your changes"). This is what makes the
  full-fork plan work with the normal `hermes update` command.
- The Misty engine (`misty` binary, `bindings/python/misty_ascii.py`) is installed on E15 at
  `~/.hermes/bin/misty.exe`; the `misty-ascii` skill is in `~/.hermes/skills/`.

## 3. Architecture: one fork, curated `main`, tested syncs

```
NousResearch/hermes-agent  main ──────────────┐   (upstream, read only)
                                              │ sync PR (merge, never rebase)
erratas/hermes-agent        upstream-mirror ──┤   exact copy of upstream main, never committed to
                            main  ◄───────────┘   = upstream + all Misty work  (what E15 and Venus run)
                            misty/<card>          one branch + PR per card, into main
E15 / Venus  ~/.hermes/hermes-agent  origin = erratas/hermes-agent (main)
                                     upstream = NousResearch/hermes-agent
```

- `main` on the fork is the only thing machines run. `hermes update` on E15/Venus pulls it.
- `upstream-mirror` is refreshed from NousResearch on every sync, so the size of each sync is
  always visible (`git log main..upstream-mirror`).
- We merge upstream into `main` (not rebase): machines pull `main`, so its history must never
  be rewritten.

### Where code goes (least invasive first)

Every change uses the first layer that can do the job. A full fork lets us edit anything,
but the fewer upstream lines we touch, the cheaper every sync is.

1. **Skin YAML** (`skins/misty-*.yaml` in the fork, installed to `~/.hermes/skins/`): colours,
   spinner frames, banner art, branding.
2. **Skill** (`skills/misty-*`): what the agent does (draw charts, use art).
3. **Plugin** (`plugins/misty/`): hook-driven behaviour (render art in replies, report charts).
4. **New module** in the fork (`hermes_cli/misty/...`): animation engine, motion settings,
   renderers. New files never conflict with upstream.
5. **Edit an upstream file**: only to call into layer 4. Rules: smallest possible change
   (target under 10 lines), marked `# MISTY: <card> <why>`, and listed in `MISTY-PATCHES.md`
   at the fork root (file, function, why, which card). If upstream later adds an equivalent
   hook, we drop ours at the next sync.

## 4. Update strategy (how we stay current without losing anything)

Cadence: a sync every week, plus on demand when upstream ships something we want.

1. `git fetch upstream` and fast-forward `upstream-mirror` to `upstream/main`.
2. Branch `misty/sync-YYYYMMDD` from `main`, `git merge upstream-mirror`.
3. Conflicts can only come from layer-5 edits. For each one: take upstream's version, re-apply
   our marked hook, update `MISTY-PATCHES.md`. If upstream changed the code around a hook so
   much that its intent is unclear, stop and ask (do not guess).
4. Run the gates (section 6). Push, open a PR into `main`, CI must be green.
5. Merge. Tag `misty-YYYYMMDD` on `main` (rollback point).
6. Deploy: `hermes update` on E15, check, then on Venus (section 5).

## 5. Deploying to E15 and Venus

One-time switch per machine (card `H1`, owner present, because it changes the running install):

1. Back up: copy `~/.hermes/hermes-agent` local edits into a branch first
   (`git stash` is not enough: commit them to `misty/local-e15-YYYYMMDD` so nothing is lost).
2. `git remote rename origin upstream`;
   `git remote add origin https://github.com/erratas/hermes-agent.git`;
   `git fetch origin`; `git checkout -B main origin/main`.
3. `hermes update` and `hermes doctor`; start a session; check the theme and `/insights`.
4. Copy `~/.hermes/skins/misty-*.yaml`, skills and plugins (an installer script in the fork
   does this: `scripts/misty-install.ps1` for Windows and `scripts/misty-install.sh` for Linux).

Rollback: `git checkout misty-<previous tag>` in `~/.hermes/hermes-agent`, then `hermes doctor`.
Hermes' own update also keeps an automatic rollback on a failed update.

E15 first, Venus a day later, so a bad build never hits both machines at once.

## 6. Gates (what must pass before anything reaches `main`)

CI on the fork, Linux and Windows runners:

- The upstream test suite (whatever upstream's CI runs), so syncs prove we broke nothing.
- `tests/misty/`, our own tests:
  - every `skins/misty-*.yaml` loads, every colour parses, every spinner frame has the same
    display width (no jitter), banner art fits 80 columns;
  - **calm/off**: with `display.motion: off`, `NO_COLOR=1`, `TERM=dumb`, or output not a TTY,
    the CLI emits **no** escape codes and no animation frames (golden output);
  - gateway platforms (Telegram, Discord, web) never receive ANSI escapes;
  - `/insights`, status and doctor reports render with the engine and without it (fallback);
  - performance budget: startup adds under 50 ms; one animation frame renders in under 2 ms;
    the spinner thread never blocks streaming output.
- `MISTY-PATCHES.md` lists exactly the upstream files that differ from `upstream-mirror` with a
  `# MISTY:` marker (a script checks this, so an unlisted edit fails CI).

## 7. The product: what "as animated and colourful as possible" means

### 7.1 Motion levels (setting `display.motion`)

| Level | What you get | When it is chosen automatically |
|---|---|---|
| `max` (default) | every animation, truecolor gradients, typewriter streaming, transitions | interactive terminal with colour |
| `calm` | colour, simple spinners, no flashing, no typewriter, no screen transitions | `MISTY_CALM=1`, or reduced-motion requested |
| `off` | plain text | `NO_COLOR`, `TERM=dumb`, not a TTY, gateways, logs |

Colour depth follows the engine's rules (`COLORTERM`, `TERM`, Windows Terminal = truecolor).

### 7.2 Themes (card `H2`, first milestone with `H3`)

A Misty theme pack in the fork's `skins/`, installed to `~/.hermes/skins/`, selectable with
Hermes' existing skin command and the `display.skin` setting. Each theme sets colours (light and
dark), spinner frames and wings, branding, tool emoji, prompt symbol, banner logo and hero art,
and the matching `tui-theme-boot.json` for the TypeScript UI. Starting set (names provisional):
`misty-neon`, `misty-aurora`, `misty-synthwave`, `misty-matrix`, `misty-ember`, and
`misty-calm` (low-contrast, motion-light). All art is our own (licensing rules in
[`../../AGENTS.md`](../../AGENTS.md)).

### 7.3 Loading and status animations (card `H3`, first milestone)

- Spinners per activity: thinking, tool running (per tool kind), downloading, waiting on a model.
- Progress bars for long tools; indeterminate bars when no progress is known.
- Animated gradient on the status bar and banner border; a pulse while the model streams.
- Typewriter reveal for streamed replies (skipped in calm/off and for long code blocks).
- Start-up splash and goodbye animation (full banner art arrives with `H6`).
- Frame sets come from the engine where possible (`misty` spinners, E6), so the CLI, the Misty OS
  GUI and the other terminals share one animation library.

### 7.4 ASCII art in every generation (card `H4`)

The model must never hand-draw art: it misaligns. Instead:

- Replies may contain fenced blocks such as ```` ```misty chart ```` / `table` / `bar` / `spark`
  / `banner` holding data. The CLI renders each block with the engine in place (same idea as
  the editor's render-in-place blocks, card `K1`). In gateways the block is rendered to plain
  Unicode text.
- A plugin hook adds the block grammar to the system prompt only when the engine is available.
- Reports (`/insights`, status, doctor, usage) draw tables, charts and sparklines with the engine.
  `/insights` already does this on E15 as a local patch; it moves into the fork under `H8`.

### 7.5 Skills (card `H5`)

`misty-ascii` exists. Add: `misty-reports` (how to present data), `misty-banner` (banners and
headings, after `H6`), `misty-theme` (switch and preview themes), `misty-animate` (when to show
progress), `misty-dashboard` (live status boards). Each skill names the exact commands and has a
real example output pasted from the engine (never hand-drawn).

## 8. Coding strategy (rules for every change in the fork)

- Match upstream style: upstream's formatter, linter and type-check config, unchanged.
- New code lives in `hermes_cli/misty/`, `plugins/misty/`, `skins/misty-*`, `skills/misty-*`,
  `tests/misty/`, `scripts/misty-*`. Upstream files only get marked hooks (section 3, layer 5).
- Every feature is behind the motion level or a setting, and safe when off.
- The engine is optional: every call goes through `misty_ascii.py`, which falls back to plain
  text. A missing or broken binary never breaks Hermes.
- No network calls, no new dependencies without an owner decision.
- Animations run on one shared timer, never block input or streaming, and stop cleanly on
  Ctrl+C, resize and exit (no stray cursor or colour state left in the terminal).
- Windows and Linux are both first-class: test on both before a PR leaves draft.
- Red before green: a failing test first for every behaviour change.
- One card, one branch, one draft PR. No em dashes in anything we write.

## 9. Maintenance strategy

- **Inventory**: `MISTY-PATCHES.md` (upstream files we touch) and `MISTY-FEATURES.md` (every
  Misty feature, its setting, its test) at the fork root, both checked by CI.
- **Health check**: `hermes misty doctor` reports engine found and version, theme loaded, motion
  level and why, patch inventory in sync, last sync date and how far `main` is behind upstream.
- **Board**: H-cards on [`../../KANBAN.md`](../../KANBAN.md); each sync gets a session-log line.
- **Changelog**: `MISTY-CHANGELOG.md` in the fork, one entry per merged PR and per sync.
- **When upstream changes direction** (renames the skin engine, replaces the CLI): a sync PR stays
  in draft, the conflict is written up, the owner decides before anything merges.
- **Local copies**: the fork is also cloned to `_BRIDGE/CODES LOL/hermes-agent` like the other
  repos, so the work is preserved on the owner's machine.

## 10. Milestones (cards)

| Card | What | Needs |
|---|---|---|
| `H0` | This plan | owner review |
| `H1` | Fork setup: `upstream-mirror`, first sync to today's upstream, CI gates, `MISTY-PATCHES.md`; move the local E15 `/insights` edit in; switch E15 then Venus to the fork | owner present for the machine switch |
| `H2` | Theme pack (skins + TUI theme), motion levels and auto-detection | H1 |
| `H3` | Loading and status animations, typewriter, splash | H2 |
| `H4` | Render-in-place ` ```misty ` blocks in replies (CLI + gateways) | H1 |
| `H5` | Skill set | H4 |
| `H6` | Banners and big splash art | engine fonts `E4` |
| `H7` | TypeScript TUI parity for H2-H4 | H2-H4 |
| `H8` | Reports: `/insights` (move the local patch in), status, doctor, usage charts | H1 |
| `H9` | `hermes misty doctor`, changelog, weekly sync routine | H1 |

## 11. Risks

- **Big first sync**: the fork is ~2.5 months behind. H1 starts from today's upstream instead of
  merging the old fork forward, so we take no stale history.
- **Escape codes leaking** into Telegram, logs or scripts: blocked by the `off` auto-detection and
  the gateway gate in CI.
- **Windows terminals** differ (Windows Terminal, conhost, PowerShell ISE): test matrix in H2.
- **Upstream redesigns the CLI/TUI**: hooks may stop applying; handled by the draft-sync rule.
- **Speed**: animation must not slow replies; budgets are CI gates.

## 12. Open questions for the owner

- Venus: where is Hermes installed and does it carry local edits? `[NEEDS CHECK]`
- Which do you use more on each machine: the classic CLI or the new TypeScript TUI? (sets the
  order of H2-H4 vs H7)
- Theme names and moods: keep the provisional set or name your own?
- May the sync routine run as a weekly scheduled task that opens the sync PR automatically
  (you still approve the merge)?
