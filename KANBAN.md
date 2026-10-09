# KANBAN: Misty ASCII programme

Mandatory for all agentic work. Read [`AGENTS.md`](AGENTS.md) first. Update this board before you
stop: move your card, add the PR link and blockers, and add a line to the Session log.

Card format: `ID` title (repo). `[owner]` = needs the human. Checklists for every card live in
[`CHECKLIST.md`](CHECKLIST.md) under the same ID.

Last updated: 2026-10-09 by Claude Code (cloud session "engine-core", after "determined-newton").

---

## Blocked (waiting on someone)

- `X1` **Turn off or bootstrap the groundwork-specflow plugin for these repos** `[owner]`.
  It blocks file edits in every repo without its docs; a bypass lasts only 60 minutes.
- `X2` **Review misty-os PR #1** `[owner]` (erratas/misty-os#1, subtab alignment). CI is red only
  because `main` is red the same way (556 ruff errors + strict stub check).
- `X3` **Decide: clean up the 556 lint errors in misty-os** `[owner]` as a separate PR, or leave.

## In progress

- `E1` + `E2` (tables part) + `E3` (core charts) **Engine core + `misty` CLI** (misty-ascii,
  erratas/misty-ascii#2, draft, stacked on #1). Library, CLI, 40 ctest cases, cosmocc build, CI.
  Left: CI green on Windows/macOS (portable job), release upload, E2 lists/trees/progress/text
  helpers/filters/colours, E3 axis extras, thresholds, statistics, streaming, braille plots.

## Next (in order)

1. **Master board catch-up** (misty-ascii): per-repo board links (misty-os#2, kate#2), card
   `R1` (rename Kate to Misty), copy ideas `I1`-`I35` from kate `KANBAN.md`.
2. `B1` dev build of the editor. The previous container (KF 6.31, 24 of 32 frameworks built)
   is gone; rebuild from scratch per kate `AGENTS.md` section 4.
3. `E2` rest: lists, trees, progress, text helpers, filters, colours, reformatter.
4. `E3` rest: axis labels/ticks, thresholds, statistics, streaming, braille plots.
5. `K1` **Kate: Diagon diagrams bundled in the plugin** (kate): all 9 translators, live preview
   pane, render-in-place blocks. Needs `B1`.
6. `K0` **Kate: switch asciifun to the shared engine** (kate): the existing chart/banner/table/
   image code moves into misty-ascii; plugin links the library.

## Backlog

- `E4` Fonts and banners (misty-ascii): our own redrawn fonts in all cfonts styles, ascii-art's 11
  fonts (incl. `3d-diagonal`, full starwar lowercase), gradients/colours/alignment/spacing,
  comment banners.
- `E5` Images and video (misty-ascii): all ascii-generator / ascii-image-converter / ascii-view
  modes, 12 language alphabets with density sorting, colour, braille, dither, edges, GIF/video.
- `E6` Fun and widgets (misty-ascii): matrix rain with every cmatrix option (own code), astroterm
  sky (own constellation data), FTXUI canvas primitives, spinners.
- `E8` Diagrams in the engine (misty-ascii): all 9 Diagon translators behind `misty-ascii diagram`, so CLIs get them too (Kate gets them first via `K1`).
- `E7` Colour output: ANSI (16/256/truecolor) + HTML export, ANSI viewer.
- `K2` Kate: tables, lists, reformatter UI. `K3` Kate: charts UI. `K4` Kate: fonts, gallery,
  banners, ANSI viewer. `K5` Kate: images/video. `K6` Kate: ASCII paint mode, art library, idle
  screen (matrix / star sky).
- `C1` Kate canvas: `canvas_write` MCP tool so Hermes, Claude Code and any sub-agent can write to
  the canvas (misty-os, hermes_mcp_bridge).
- `C2` Kate canvas: agent writes type out with the typewriter animation + sounds (misty-os).
- `H1` Hermes CLI (E15 + Venus): banners, tables, charts, typed replies via the engine binary.
  Install on E15/Venus is `[owner]` (no cloud access to those machines).
- `T1` AI-SUITE terminals use the engine. `T2` astroterm idle screen. `T3` Other CLIs
  (cli, cli-go, cli-js, cli-react).

## Done

- `E0` 2026-10-09 Repo bootstrap: AGENTS.md, KANBAN.md, CHECKLIST.md, CREDITS.md
  (erratas/misty-ascii#1, draft, awaiting owner merge).

- `D1` 2026-10-09 misty-os subtab alignment + TELEMETRY rename + bottom-bar overlap fix + offline
  embed card with Retry + card style cleanup + Playwright check (erratas/misty-os#1, draft).
- `D2` 2026-10-09 Feature inventory of all 16 source repos (summarised in CHECKLIST.md).
- `D3` 2026-10-09 Toolchain proven in the cloud: Diagon builds and renders; cosmocc builds a
  working portable binary; Qt 6.11.3 installed.

---

## Session log

One line per session, newest last: `date | who | what changed | what failed | what is next`.

- 2026-10-09 | Claude Code cloud (determined-newton) | misty-os PR #1; repo inventory; Qt 6.11 +
  KF6 build started; cosmocc proven; this board created | KF 6.24 vs Qt 6.8 mismatch, two
  self-inflicted build restarts, GitHub refused repo creation (owner created it) | finish B1,
  then E1.
- 2026-10-09 | Claude Code cloud (engine-core) | E1-E3 core: library + `misty` CLI + 40 ctest
  cases + cosmocc APE binary (40/40 goldens pass on it, ~3 ms start) + CI with Windows/macOS
  runs of the same binary (erratas/misty-ascii#2) | fixed upstream asciichart off-by-one-row
  rounding; B1 not resumed (container gone) | board catch-up, then B1, then K0.
