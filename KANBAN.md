# KANBAN: Misty ASCII programme

Mandatory for all agentic work. Read [`AGENTS.md`](AGENTS.md) first. Update this board before you
stop: move your card, add the PR link and blockers, and add a line to the Session log.

Card format: `ID` title (repo). `[owner]` = needs the human. Checklists for every card live in
[`CHECKLIST.md`](CHECKLIST.md) under the same ID.

Last updated: 2026-10-09 by Claude Code (cloud session "determined-newton").

---

## Blocked (waiting on someone)

- `X1` **Turn off or bootstrap the groundwork-specflow plugin for these repos** `[owner]`.
  It blocks file edits in every repo without its docs; a bypass lasts only 60 minutes.
- `X2` **Review misty-os PR #1** `[owner]` (erratas/misty-os#1, subtab alignment). CI is red only
  because `main` is red the same way (556 ruff errors + strict stub check).
- `X3` **Decide: clean up the 556 lint errors in misty-os** `[owner]` as a separate PR, or leave.

## In progress

- `B1` **Kate dev build in the cloud container** (`/home/user/.kdebuild`). KF 6.31: 24 of 32
  frameworks built (2026-10-09 20:19 UTC, now on kbookmarks). Then build Kate itself.
- `E0` **Bootstrap this repo**: AGENTS.md, KANBAN.md, CHECKLIST.md, CREDITS.md (this PR).

## Next (in order)

1. `E1` **Engine core + Cosmopolitan CLI** (misty-ascii): C++ library skeleton, `misty-ascii`
   CLI, cosmocc build, test harness, CI.
2. `E2` **Tables, lists, text** (misty-ascii): everything from go-pretty + Diagon tables +
   ASCII-Data + FTXUI borders, all output formats, table reformatter.
3. `E3` **Charts** (misty-ascii): every option from asciichart, asciichart1, asciigraph,
   asciigraph-rs, plus FTXUI gauges/spinners/graph.
4. `K1` **Kate: Diagon diagrams bundled in the plugin** (kate): all 9 translators, live preview
   pane, render-in-place blocks. Needs `B1`.
5. `K0` **Kate: switch asciifun to the shared engine** (kate): the existing chart/banner/table/
   image code moves into misty-ascii; plugin links the library.

## Backlog

- `E4` Fonts and banners (misty-ascii): our own redrawn fonts in all cfonts styles, ascii-art's 11
  fonts (incl. `3d-diagonal`, full starwar lowercase), gradients/colours/alignment/spacing,
  comment banners.
- `E5` Images and video (misty-ascii): all ascii-generator / ascii-image-converter / ascii-view
  modes, 12 language alphabets with density sorting, colour, braille, dither, edges, GIF/video.
- `E6` Fun and widgets (misty-ascii): matrix rain with every cmatrix option (own code), astroterm
  sky (own constellation data), FTXUI canvas primitives, spinners.
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
