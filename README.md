# misty-ascii
ASCII FUN AND MADNESS!! ALL HERE!


Start here: [AGENTS.md](AGENTS.md) (reorientation, mandatory), [KANBAN.md](KANBAN.md) (board), [CHECKLIST.md](CHECKLIST.md) (every feature), [CREDITS.md](CREDITS.md).

## The engine

A C++17 library (`include/misty/ascii.hpp`, no Qt, no network) and the `misty` command-line
tool. Built with Cosmopolitan it is one binary that runs on Linux, Windows and macOS.

```sh
# Native build + tests
cmake -S . -B build && cmake --build build && ctest --test-dir build

# Portable binary (downloads cosmocc on first run): dist/misty and dist/misty.exe
scripts/build-cosmo.sh
```

```sh
misty table data.csv                      # also TSV, ; or | text, Markdown tables
misty table --style rounded --sort 3 --dsc --numeric --title People data.csv
misty table --format markdown|csv|tsv|html data.csv
echo "3 1 4 1 5 9 2 6" | misty chart --height 6
misty chart --legend cpu,mem --colors red,cyan two-lines.txt
misty spark 1 5 2 8 3 9 4                 # ▁▅▂▇▃█▄
misty bar fruit.txt                       # lines of "label value"
misty color tomato "hello"                # --list for all 148 names
misty gradient --stops red,blue "Misty"
misty capabilities --json
```

Contract for callers (Go, Node, Python, shell): art on stdout, errors on stderr, `--json` gives
`{"ok":true,"text":...,"width":N,"height":N}` or `{"ok":false,"error":...}`, exit codes
0 ok / 1 bad input / 2 bad usage / 3 not in this build. `--color auto|never|16|256|truecolor`
(auto honours `NO_COLOR`, `FORCE_COLOR`, `COLORTERM` and is off when piped), `--ascii` for
plain 7-bit output, `--width N` else `$COLUMNS`, the terminal, or 80.

macOS: start the binary through a shell (`sh misty table ...`) when calling it from Go, Node or
anything using `posix_spawn()`; interactive shells already do this.

Use as a CMake subproject: `add_subdirectory(misty-ascii)` then link `misty::ascii`
(the CLI and tests are off by default in that case).
