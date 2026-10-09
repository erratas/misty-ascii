---
name: misty-ascii
description: Draw tables, line charts, sparklines, bar charts and coloured text with the Misty ASCII engine (`misty`) in replies, reports and terminals. Load whenever you show numbers, lists of records, trends, comparisons or status to the user.
version: 0.1.0
source: erratas/misty-ascii (bindings/hermes)
---

# misty-ascii: pretty tables and graphs, never hand-drawn

The `misty` binary is the Misty ASCII engine. Use it whenever a reply, report or terminal
shows data: a table for records, a line chart for a trend, a bar chart for a comparison,
a sparkline for a quick shape. Run it, then paste its output exactly as printed inside a
fenced code block (```text). Never draw boxes or charts by hand: hand-drawn art misaligns.

## Where it is

- Windows (E15): `%USERPROFILE%\.hermes\bin\misty.exe` (also `misty` when that folder is on PATH)
- Linux / Venus: `misty` on PATH, or `sh /path/to/misty` if starting it says "exec format error"
- Source and local copy: `_BRIDGE/CODES LOL/misty-ascii` (`dist/misty.exe`)
- Check: `misty --version` and `misty capabilities --json`

## Commands (data on stdin or as a file; text as arguments)

| Want | Command |
|---|---|
| Table from CSV/TSV/Markdown | `misty table data.csv` or `... \| misty table --style rounded` |
| Sorted, titled table | `misty table --sort 3 --dsc --numeric --title "Top hosts" data.csv` |
| Markdown / CSV / HTML table | `misty table --format markdown data.csv` (also `csv`, `tsv`, `html`) |
| Line chart (one series per line) | `echo "3 1 4 1 5 9 2 6" \| misty chart --height 8` |
| Several series with legend | `misty chart --legend cpu,mem --caption "last hour" series.txt` |
| Sparkline | `misty spark 1 5 2 8 3 9 4` |
| Bar chart | lines of `label value` piped to `misty bar --width 30` |
| Coloured text (terminals only) | `misty color tomato "hello"`, `misty gradient --stops red,blue "Misty"` |

Styles: `light` (default), `rounded`, `bold`, `double`, `ascii`, `default`, `none`.
Missing values in charts: write `nan` (or `-`), the line breaks there.

## Rules

1. Replies to chat (CLI, Telegram, web): add `--color never` (the default when piped) and wrap
   the output in a ```text fence. Colour only when writing straight to a terminal.
2. Old consoles or logs that mangle Unicode: add `--ascii`.
3. Machine use: add `--json` and read `{"ok":true,"text":...}`; exit code 3 means the command
   is not in this build yet (banner, fonts, image, rain, sky, diagram are planned).
4. Python code (Hermes, Misty OS, Misty Control): `from agent import misty_ascii` in
   hermes-agent, or copy `bindings/python/misty_ascii.py`. `table()`, `chart()`, `spark()`,
   `bar()` return text and fall back to plain text if the binary is missing.
5. `/insights` already uses the engine (models table, tool bars, sessions-per-day chart,
   24-hour sparkline).

## Example

```sh
printf 'host,load,mem\nE15,72,61\nVenus,41,38\n' | misty table --style rounded --title Fleet
```

```text
╭────────────────────╮
│       Fleet        │
├───────┬──────┬─────┤
│ host  │ load │ mem │
├───────┼──────┼─────┤
│ E15   │   72 │  61 │
│ Venus │   41 │  38 │
╰───────┴──────┴─────╯
```
