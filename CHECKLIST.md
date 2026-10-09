# CHECKLIST: everything, nothing left behind

Mandatory companion to [`KANBAN.md`](KANBAN.md). Each section matches a card ID. Tick `[x]` only
after you verified it (ran it, tested it, looked at it). Write a test for every ticked feature.
Source names in brackets show where the feature comes from; we rebuild it as our own code.

Already shipped in the Kate asciifun plugin (erratas/kate PR #1, fixed settings only): one line
chart, 10 banner fonts, one CSV/TSV table style, image to ASCII, image to braille, matrix rain.
Those move into the engine under `K0` and gain all their options below.

---

## E0 Repo bootstrap

- [x] AGENTS.md reorientation (start/end rituals, rules, map, build notes)
- [x] KANBAN.md board with session log
- [x] CHECKLIST.md (this file)
- [x] CREDITS.md
- [x] README links to the three files above, CLAUDE.md points to AGENTS.md

## E1 Engine core + Cosmopolitan CLI

- [x] Library layout: `include/misty/ascii.hpp` (one header, per the engine design), `src/`, no Qt
      dependency, C++17; works as a CMake subproject (`misty::ascii`)
- [x] Unicode width handling (East Asian wide, combining marks, emoji) [go-pretty text]
- [x] ANSI escape awareness: strip, measure without escapes (CSI and OSC) [go-pretty text]
- [x] Colour model: 16 / 256 / truecolor, named colours, hex, HSV, gradient interpolation
      [cfonts, FTXUI, go-pretty]; honour `NO_COLOR`, `FORCE_COLOR`
- [ ] Output targets: plain text, ANSI, HTML (inline styles and CSS classes), Markdown
      (done for tables: text, ANSI colour, HTML with CSS class, Markdown; generic ANSI to HTML
      export is E7)
- [ ] `misty` CLI (binary renamed per the engine design) with subcommands, `--help` for each,
      reads stdin or files. Done: `table chart spark bar color gradient capabilities`, `--json`,
      exit codes, `--color`, `--ascii`, `--width`. Planned ones exit 3: `banner fonts image rain
      sky diagram`; `list` and `progress` not wired yet
- [x] Cosmopolitan build (`cosmoc++`) produces one portable binary; plain CMake build too
      (`scripts/build-cosmo.sh`; 40/40 golden tests pass against the APE binary on Linux)
- [x] Test harness (golden-file tests) + CI on GitHub Actions (40 ctest cases; one APE binary
      passes them on Linux, Windows and macOS runners)
- [ ] Release: attach the portable binary to GitHub releases

## E2 Tables, lists, text

Tables [go-pretty table, Diagon Table, ASCII-Data, FTXUI table]:
- [ ] Input: CSV, TSV, any delimiter, Markdown table, JSON array, aligned text
      (done: CSV with RFC 4180 quotes, TSV, any delimiter + detection, Markdown)
- [ ] go-pretty styles: Default, Bold, Double, Light, Rounded, ColoredBright, ColoredDark,
      ColoredBlackOn{Blue,Cyan,Green,Magenta,Yellow,Red}White,
      Colored{Blue,Cyan,Green,Magenta,Red,Yellow}WhiteOnBlack
- [ ] Box styles: Default `+-|`, Bold, Double, Light, Rounded; every BoxStyle character settable
- [ ] Diagon table styles (18): unicode, unicode rounded, unicode bold, unicode double,
      unicode with bold header, unicode with double header, unicode cells, unicode cells 2, ascii,
      ascii rounded, ascii with header 1, ascii with header 2, ascii light header,
      ascii light header/separator, ascii light header/separator/border,
      ascii light separator/border, ascii light border, conceptual
- [ ] ASCII-Data formats (ASCII `+-=|`, UTF8 double with row separators), null value text
- [ ] Options: draw border, separate columns/header/footer/rows, hide separators when empty
- [ ] Header, footer, title (with align), caption (done, title always centred)
- [ ] Auto-index (row numbers + column letters), index column
- [ ] Per-column: align (left, center, right, justify, auto), vertical align, header/footer
      align, colours, hidden, width min/max, wrap (text, soft, hard, trim), transformers
- [ ] Transformers: number, JSON, time, unix time, URL; header case upper/lower/title
- [ ] Auto-merge cells vertically and horizontally (with merge alignment)
- [ ] Sorting: asc, asc alpha-numeric, asc numeric, asc numeric-alpha, and the 4 descending;
      ignore case; multiple sort keys (done: asc, dsc, asc/dsc numeric, one key)
- [ ] Filtering: =, !=, >, >=, <, <=, contains, not contains, starts with, ends with, regex,
      not regex; ignore case
- [ ] Suppress empty columns, suppress trailing spaces, allowed row length, paging
- [ ] Colours: header, rows, alternate rows, footer, border, separator, index column
- [ ] Output: text, CSV (with formula-injection guard), TSV, HTML (CSS class, escape, colours to
      spans), Markdown (padded content) (all done except colours to spans)
- [ ] Table reformatter: realign an existing messy table in place, any style

Lists [go-pretty list]:
- [ ] 11 styles: Default `*`, BulletCircle, BulletFlower, BulletSquare, BulletStar,
      BulletTriangle, ConnectedBold, ConnectedDouble, ConnectedLight, ConnectedRounded, Markdown
- [ ] Indent / un-indent levels, line prefix, HTML and Markdown output

Trees [Diagon Tree]:
- [ ] 8 styles: unicode 1, unicode 2, ASCII 1, ASCII 2, ASCII 3, unicode right top,
      unicode right center, unicode right bottom

Progress bars [go-pretty progress, FTXUI gauge]:
- [ ] Styles: Default, Blocks, Circle, Rhombus; quarter-step characters
- [ ] Indeterminate indicators: dominoes, coloured dominoes, back-and-forth, left-to-right,
      right-to-left, Pac-Man, Pac-Man chomp
- [ ] Shows: message, percent, value, units (number, bytes, $, EUR, GBP), speed, ETA, time,
      overall tracker; sort trackers; pinned messages; errors
- [ ] Gauges in 4 directions [FTXUI]

Text helpers [go-pretty text, FTXUI paragraph]:
- [ ] Align left/center/right/justify, vertical align, pad, trim, snip with indicator, wrap
      (hard/soft/text), widen, repeat-and-trim, insert every N, CRLF handling
- [ ] Attributes: bold, faint, italic, underline, double underline, blink, reverse, concealed,
      strikethrough
- [ ] OSC-8 hyperlinks
- [ ] Frame / box around text with optional line numbers [Diagon Frame]; 6 border styles
      (light, dashed, heavy, double, rounded, empty) [FTXUI]

## E3 Charts

[asciichart, asciichart1, asciigraph, asciigraph-rs, ASCII-Data, FTXUI]
- [x] Line chart, one or many series, auto range
- [ ] Height, width (resampling), offset, padding (done except padding)
- [ ] Min/max (lower/upper bound), precision, Y formatter (done except Y formatter)
- [ ] Custom symbol sets, single-character plots
- [ ] Series colours, axis colour, label colour, caption colour (done: series colours)
- [ ] Caption, legends (with padding), X axis range / tick count / formatter, axis labels
      (done: caption, legends)
- [ ] Gradient/heatmap colouring, colour above/below threshold
- [ ] Zero line, threshold lines (dashed), moving average
- [ ] Statistics annotations: min, max, mean, median, plus/minus sigma
- [x] Gaps for missing values (NaN)
- [ ] Streaming / realtime mode with buffer and FPS; animation by redraw
- [ ] Input: numbers from text, CSV columns, delimiter, series count
- [ ] Function graph [FTXUI graph]; sparklines; bar charts; braille-resolution plots
      (done: sparklines, horizontal bar charts with eighth blocks)

## E4 Fonts and banners

Our own fonts, drawn fresh (cfonts is GPL-3: rebuild its features, never copy its files):
- [ ] Styles to draw: console, block, simpleBlock, simple, 3d, simple3d, chrome, huge, shade,
      slick, grid, pallet, tiny (multi-layer fonts get one colour per layer)
- [ ] Characters for every font: A-Z, 0-9, and `| ! ? . + - _ = @ # $ % & ( ) / : ; , ' "` space
- [ ] ascii-art's 11 fonts (MIT): sevenstar, boomer, straight, starwar (full lowercase),
      carlos, banner, block, amongus, drpepper, small, 3d-diagonal
- [ ] Options: align left/center/right/top/bottom, letter spacing, line height, spaceless,
      max length per line with auto wrap, `|` as line break
- [ ] Colours per layer, background colour, candy (random), hex
- [ ] Gradient, independent gradient per line, transition gradient, HSV interpolation, preset
      flag gradients (lgbt, pride, trans, bi, pan, ace, nonbinary, and the rest)
- [ ] Font gallery with live preview
- [ ] Comment banners wrapped in the current language's comment syntax
- [ ] FIGlet `.flf` font loading (bonus: thousands of existing fonts)

## E5 Images and video

[ascii-image-converter, ascii-view, ascii-generator, existing Kate code]
- [ ] Inputs: png, jpeg, webp, tiff, bmp, gif (animated), URL, stdin
- [ ] Size: width, height, dimensions, full, character aspect ratio
- [ ] Ramps: simple, detailed/complex, custom map; 12 language alphabets (general, english,
      german, french, spanish, italian, portuguese, polish, russian, chinese, korean, japanese
      hiragana + katakana) with density sorting by font
- [ ] Braille with threshold and Floyd-Steinberg dither
- [ ] Negative, grayscale, flip X, flip Y
- [ ] Colour: truecolor, 8-bit, colour as background, retro 3-bit HSV mapping
- [ ] Sobel edge characters `_ / | \` with threshold
- [ ] Save: text, PNG, GIF (with font, font colour, background colour)
- [ ] Video to ASCII (fps, colour, overlay of the original)

## E6 Fun and widgets

- [ ] Matrix rain, our own code, every cmatrix option: async scroll, bold / all bold / no bold,
      kana, lambda mode, characters change while scrolling, old-style scroll, screensaver
      (exit on key), lock mode, centred message, speed 0-10, 8 colours, rainbow, pause;
      runtime keys for all of them
- [ ] Star sky [astroterm, MIT code, our own data files]: stars by magnitude threshold, labels,
      constellations, planets, moon phase with name, azimuth grid, braille lines, colour,
      unicode/ASCII, date/time, speed, fps, latitude/longitude/city, zodiac and metadata panel
- [ ] Spinners: 23 charsets [FTXUI]
- [ ] Canvas primitives (braille 2x4 and block 2x2): point, line, circle, filled circle,
      ellipse, filled ellipse, text, image [FTXUI]
- [ ] Layout: hbox, vbox, grid, flexbox, separators, borders, windows [FTXUI], rendered to text

## E7 Colour output and viewing

- [ ] Export any result as ANSI (16/256/truecolor) and as HTML
- [ ] ANSI viewer: show `.ans` files and coloured logs in colour
- [ ] Palettes: 16 named, 256 named, 140 CSS/X11 names [asciigraph], linear gradients

## E8 Diagrams (engine side, from Diagon)

- [ ] Math: Unicode / ASCII / LaTeX output, transform math letters on/off; fractions, powers,
      subscripts, sqrt, sum, int, lim, mult, blackboard letters, Greek, matrices, functions
- [ ] Sequence: ascii only, interpret `\n`; arrows `->` `-->` `<-` `<--`, ordering, comments
- [ ] Tree (see E2), Table (see E2), Frame (see E2 text helpers)
- [ ] Graph DAG; Graph planar (needs Boost.Graph); Flowchart; Grammar railroad diagrams
      (abnf/bnf/rbnf/wsn in; unicode/ascii/svg/html out)

## K0 Kate plugin on the shared engine

- [ ] Plugin links the engine library; existing actions keep working
- [ ] Every existing action gets its options dialog (height, columns, threshold, style...)

## K1 Kate: diagrams

- [ ] All 9 Diagon translators from the Tools menu, on the selection
- [ ] Live preview pane (source left, rendered right, updates while typing)
- [ ] Render-in-place fenced blocks (` ```diagon flowchart `) toggled source/art
- [ ] Diagon + ANTLR bundled in the plugin build, no network at build time

## K2-K6 Kate UI for the engine

- [ ] K2 tables, lists, trees, progress, reformatter
- [ ] K3 charts with all options
- [ ] K4 fonts gallery, banners, comment banners, ANSI viewer, colour export
- [ ] K5 images and video
- [ ] K6 ASCII paint mode (boxes/lines, auto-joined corners), art library, idle screen
- [ ] Minimap keeps working (owner likes it; do not regress)

## C1-C2 Kate canvas (misty-os)

- [ ] C1 `canvas_write` MCP tool (and a matching Hermes tool) wrapping `POST /api/canvas/write`
- [ ] C1 Claude Code skill or command so any session can write to the canvas
- [ ] C2 agent writes type out with the typewriter lever animation and synth sounds, in the
      author's colour; respects reduced-motion and mute

## H1 Hermes CLI (E15 Misty + Venus Misty)

- [ ] Banners at start, tables and charts in replies, typed-out replies, colour themes
- [ ] Uses the portable `misty-ascii` binary; works on Windows (E15) and Linux (Venus)
- [ ] Install steps written for the owner (no cloud access to those machines)

## T1-T3 Other terminals

- [ ] T1 AI-SUITE terminals in the Misty OS web GUI
- [ ] T2 astroterm-style star sky as an idle screen
- [ ] T3 cli, cli-go, cli-js, cli-react
