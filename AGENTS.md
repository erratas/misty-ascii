# AGENTS.md: read this first (every agent, every session)

This file is the **reorientation point** for all agentic work on the Misty ASCII / terminal-art
programme: Claude Code sessions, Hermes agents, sub-agents, anything that touches this work.
It is **mandatory**: read it before you start, and update the board before you stop.

## 1. Start-of-session ritual (do these in order)

1. Read this file top to bottom.
2. Open [`KANBAN.md`](KANBAN.md). Find the card you are picking up (or the one the user named).
3. Open [`CHECKLIST.md`](CHECKLIST.md) and find that card's checklist section.
4. Check the live state the board points to (open PRs, build status, blockers) before you trust it.
   The board can be stale; GitHub and the code are the truth.
5. Move your card to **In progress** in `KANBAN.md` with your session name and the date.

## 2. End-of-session ritual (never skip, even when interrupted)

1. Tick every checklist item you finished in `CHECKLIST.md`. Only tick what you verified.
2. Update your card in `KANBAN.md`: column, PR link, what is left, any blocker (who must act).
3. Add one line to the **Session log** at the bottom of `KANBAN.md`: date, who, what changed,
   what failed, what is next.
4. Commit those two files with your work. A session that changes code but not the board is not done.

## 3. What this programme is

One **shared ASCII engine** (this repo, `misty-ascii`): a C++ library plus a command-line tool,
built with **Cosmopolitan** into a single binary that runs unchanged on Linux, Windows and macOS.
Everything else uses it:

| Consumer | Repo | How it uses the engine |
|---|---|---|
| Kate editor plugin ("ASCII Fun") | `erratas/kate`, `addons/asciifun` | Links the library directly (Qt/KDE plugin) |
| Kate canvas in the Misty OS web GUI | `erratas/misty-os`, `services/gui/static/kate-canvas.js` | Calls the CLI / server endpoints; agents write via `POST /api/canvas/write` |
| Hermes CLI (E15 Misty and Venus Misty) | `erratas/hermes-agent` | Calls the CLI binary |
| Other CLIs | `erratas/cli`, `cli-go`, `cli-js`, `cli-react` | Call the CLI binary |
| AI-SUITE terminals | `erratas/misty-os` | Call the CLI binary |

## 4. Ground rules

- **Our code is ours.** Everything written in this programme belongs to the owner, with whatever
  licence they choose.
- **Rebuild features, do not copy code** from the GPL-3 repos (`cfonts`, `cmatrix`): reimplement
  every feature and option from scratch, and draw our own fonts in their styles.
- **MIT / BSD / Apache sources** (Diagon, go-pretty, FTXUI, asciichart, asciichart1, asciigraph,
  asciigraph-rs, ascii-art, ascii-view, ascii-generator, ascii-image-converter, ASCII-Data,
  astroterm code): porting is fine; keep a line for each in [`CREDITS.md`](CREDITS.md).
- Do not ship the proprietary fonts in `ascii-generator/fonts` (`arial-unicode.ttf`, `simsun.ttc`).
- astroterm data: city list needs a GeoNames (CC-BY 4.0) credit; constellation lines come from
  Stellarium (GPL-2+): redraw ours rather than bundle theirs.
- No em dashes in any text we write (owner preference).
- Branch + draft PR for every change; never push to `main`. One card ~ one PR.
- Verify before ticking: run it, test it, look at it. Say plainly what was not verified.

## 5. Where things live

| Thing | Where |
|---|---|
| Board (what is happening) | [`KANBAN.md`](KANBAN.md) |
| Full feature checklist ("nothing left behind") | [`CHECKLIST.md`](CHECKLIST.md) |
| Credits for ported code | [`CREDITS.md`](CREDITS.md) |
| Source repos to mine for features | `erratas/cfonts`, `go-pretty`, `diagon`, `ftxui`, `astroterm`, `ascii-generator`, `cosmopolitan`, plus the `ascii*`/`asciichart*`/`asciigraph*`/`cmatrix` repos |
| Kate dev build (cloud sessions) | `/home/user/.kdebuild` (Qt 6.11.3 prebuilt, KDE Frameworks 6.31 from source, Diagon, cosmocc). Not in git: rebuild if the container is new. |

## 6. Build notes (hard-won)

- Kate needs Qt 6.5+ and KDE Frameworks 6. Ubuntu 24.04 has neither: use prebuilt Qt via
  `aqtinstall` and build KF6 from release tarballs. KF 6.24 does NOT really build on Qt 6.8
  (needs a Qt 6.9 private header); use Qt 6.11 + KF 6.31. KF 6.31 needs CMake 3.30+.
- KF build flags used: `-DWITH_WAYLAND=OFF -DKWINDOWSYSTEM_WAYLAND=OFF -DWITH_TEXT_TO_SPEECH=OFF
  -DBUILD_TESTING=OFF`. knotifications must build before kjobwidgets.
- Diagon pulls ANTLR from GitHub at configure time; turn off ANTLR's tests
  (`-DANTLR_BUILD_CPP_TESTS=OFF`) or the GoogleTest download fails behind the proxy.
- Cosmopolitan: `cosmocc` from https://cosmo.zip/pub/cosmocc/cosmocc.zip; `cosmoc++` does not
  read source from stdin, pass a file.
