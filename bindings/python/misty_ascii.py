# SPDX-License-Identifier: MIT
"""misty_ascii: call the `misty` ASCII engine from Python (Hermes, Misty OS, Misty Control).

Standard library only. Every function returns text and never raises because the engine is
missing or fails: it falls back to a plain pure-Python rendering, so a report or a GUI
endpoint keeps working on a machine without the binary.

Binary lookup (first hit wins):
  1. $MISTY_BIN (set it to "off" to force the fallback)
  2. `misty` / `misty.exe` on PATH
  3. ~/.hermes/bin, ~/.local/bin
  4. a `misty-ascii/dist` folder next to, or above, this file (the CODES LOL checkout)

Copy this one file into any project; there is nothing else to install.
"""
from __future__ import annotations

import csv
import io
import json
import math
import os
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Optional, Sequence

__all__ = [
    "Result", "find_binary", "available", "run", "table", "chart", "spark", "bar",
    "colorize", "gradient", "capabilities",
]

_OFF = {"off", "0", "none", "false", "no"}
_cache: dict = {}


@dataclass
class Result:
    ok: bool
    text: str = ""
    error: str = ""
    code: int = 0
    width: int = 0
    height: int = 0


def _exe_names() -> list[str]:
    return ["misty.exe", "misty"] if os.name == "nt" else ["misty", "misty.exe"]


def find_binary(refresh: bool = False) -> Optional[str]:
    """Path of the misty binary, or None. Cached; pass refresh=True after installing."""
    if not refresh and "bin" in _cache:
        return _cache["bin"]
    found = None
    env = os.environ.get("MISTY_BIN", "").strip()
    if env.lower() in _OFF:
        _cache["bin"] = None
        return None
    if env and Path(env).is_file():
        found = env
    if not found:
        for name in _exe_names():
            hit = shutil.which(name)
            if hit:
                found = hit
                break
    if not found:
        home = Path.home()
        dirs = [home / ".hermes" / "bin", home / ".local" / "bin"]
        here = Path(__file__).resolve()
        for up in here.parents:
            dirs.append(up / "dist")
            dirs.append(up / "misty-ascii" / "dist")
        for d in dirs:
            for name in _exe_names():
                p = d / name
                if p.is_file():
                    found = str(p)
                    break
            if found:
                break
    _cache["bin"] = found
    return found


def available() -> bool:
    return find_binary() is not None


def _command(binary: str) -> list[str]:
    # An APE file has no #! line. Shells and execvp() fall back to running it as a script,
    # but posix_spawn/execve (Python subprocess, Go, Node) fail with ENOEXEC on Linux and
    # macOS unless the APE loader is installed. So: try it directly once, and if the kernel
    # refuses it, run it through sh from then on. Windows runs misty.exe natively.
    if _cache.get("via_sh") == binary:
        return ["sh", binary]
    return [binary]


def _spawn(binary: str, argv: list[str], data: bytes, timeout: float):
    try:
        return subprocess.run(_command(binary) + argv, input=data, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE, timeout=timeout)
    except OSError as exc:
        if exc.errno == 8 and os.name != "nt" and _cache.get("via_sh") != binary:  # ENOEXEC
            _cache["via_sh"] = binary
            return subprocess.run(["sh", binary] + argv, input=data, stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE, timeout=timeout)
        raise


def run(command: str, args: Sequence[str] = (), stdin: Optional[str] = None,
        timeout: float = 10.0) -> Result:
    """Run `misty <command> <args> --json`. Never raises."""
    binary = find_binary()
    if not binary:
        return Result(False, error="misty binary not found", code=3)
    try:
        proc = _spawn(binary, [command, *args, "--json"],
                      (stdin if stdin is not None else "").encode("utf-8"), timeout)
    except (OSError, subprocess.SubprocessError) as exc:
        return Result(False, error=f"{type(exc).__name__}: {exc}", code=3)
    out = proc.stdout.decode("utf-8", "replace").strip()
    try:
        data = json.loads(out.splitlines()[-1]) if out else {}
    except (ValueError, IndexError):
        data = {}
    if data.get("ok"):
        return Result(True, data.get("text", ""), "", proc.returncode,
                      int(data.get("width", 0)), int(data.get("height", 0)))
    err = data.get("error") or proc.stderr.decode("utf-8", "replace").strip() or "misty failed"
    return Result(False, error=err, code=proc.returncode or 1)


def capabilities() -> dict:
    binary = find_binary()
    if not binary:
        return {"ok": False, "subcommands": []}
    try:
        proc = _spawn(binary, ["capabilities", "--json"], b"", 10)
        return json.loads(proc.stdout.decode("utf-8", "replace"))
    except (OSError, subprocess.SubprocessError, ValueError):
        return {"ok": False, "subcommands": []}


# ------------------------------------------------------------------ helpers

def _color_args(color: Optional[str]) -> list[str]:
    return ["--color", color] if color else ["--color", "never"]


def _num(v) -> str:
    if v is None:
        return "nan"
    try:
        f = float(v)
    except (TypeError, ValueError):
        return "nan"
    return "nan" if math.isnan(f) else repr(f) if not f.is_integer() else str(int(f))


def _rows_to_csv(rows: Iterable[Sequence]) -> str:
    buf = io.StringIO()
    w = csv.writer(buf, lineterminator="\n")
    for r in rows:
        w.writerow(["" if c is None else str(c) for c in r])
    return buf.getvalue()


# ------------------------------------------------------------------ fallbacks (pure Python)

_BLOCKS = "▁▂▃▄▅▆▇█"


def _fallback_spark(values: Sequence) -> str:
    nums = [float(v) for v in values if v is not None and not _isnan(v)]
    if not nums:
        return ""
    lo, hi = min(nums), max(nums)
    out = []
    for v in values:
        if v is None or _isnan(v):
            out.append(" ")
        elif hi == lo:
            out.append(_BLOCKS[3])
        else:
            out.append(_BLOCKS[min(7, max(0, math.floor((float(v) - lo) / (hi - lo) * 7 + 0.5)))])  # C lround, as the engine
    return "".join(out)


def _isnan(v) -> bool:
    try:
        return math.isnan(float(v))
    except (TypeError, ValueError):
        return True


def _fallback_table(rows: list[list[str]], header: bool) -> str:
    if not rows:
        return ""
    n = max(len(r) for r in rows)
    rows = [list(r) + [""] * (n - len(r)) for r in rows]
    w = [max(len(r[i]) for r in rows) for i in range(n)]
    lines = []
    for k, r in enumerate(rows):
        lines.append("  ".join(c.ljust(w[i]) for i, c in enumerate(r)).rstrip())
        if k == 0 and header:
            lines.append("  ".join("-" * x for x in w))
    return "\n".join(lines) + "\n"


def _fallback_bar(labels: Sequence[str], values: Sequence[float], width: int) -> str:
    peak = max([v for v in values if not _isnan(v)] + [0])
    lw = max((len(str(lab)) for lab in labels), default=0)
    out = []
    for lab, v in zip(labels, values):
        n = 0 if peak <= 0 or _isnan(v) or v <= 0 else max(1, round(v / peak * width))
        out.append(f"{str(lab).ljust(lw)} │{'█' * n}{' ' * (width - n + 1)}{v:g}")
    return "\n".join(out) + "\n"


# ------------------------------------------------------------------ public renderers

def table(rows: Sequence[Sequence] | str, *, style: str = "rounded", fmt: str = "text",
          header: bool = True, footer: bool = False, title: str = "", caption: str = "",
          sort: Optional[int] = None, descending: bool = False, numeric: bool = False,
          index: bool = False, ascii_only: bool = False) -> str:
    """Rows (first row = header) or CSV/TSV/Markdown text, rendered as a table."""
    text = rows if isinstance(rows, str) else _rows_to_csv(rows)
    args = ["--style", style, "--format", fmt]
    if not header:
        args.append("--no-header")
    if footer:
        args.append("--footer")
    if title:
        args += ["--title", title]
    if caption:
        args += ["--caption", caption]
    if sort is not None:
        args += ["--sort", str(sort + 1)] + (["--dsc"] if descending else []) + (["--numeric"] if numeric else [])
    if index:
        args.append("--index")
    if ascii_only:
        args.append("--ascii")
    if not isinstance(rows, str):
        args += ["--delimiter", ","]
    r = run("table", args, text)
    if r.ok:
        return r.text
    parsed = list(csv.reader(io.StringIO(text))) if not isinstance(rows, str) else [
        [c.strip() for c in line.split(",")] for line in text.splitlines() if line.strip()]
    return _fallback_table(parsed, header)


def chart(series: Sequence[Sequence] | Sequence[float], *, height: int = 10, width: int = 0,
          legends: Sequence[str] = (), colors: Sequence[str] = (), caption: str = "",
          precision: int = 2, color: Optional[str] = None, ascii_only: bool = False) -> str:
    """Line chart of one series or a list of series. None/NaN leave a gap."""
    multi = bool(series) and isinstance(series[0], (list, tuple))
    data = list(series) if multi else [list(series)]
    data = [s for s in data if len(s) > 0]
    if not data:
        return ""
    if len(data) == 1:  # one series: one value per line so a single row parses as one series
        text = "\n".join(_num(v) for v in data[0]) + "\n"
    else:
        text = "\n".join(" ".join(_num(v) for v in s) for s in data) + "\n"
    args = ["--height", str(height), "--precision", str(precision)] + _color_args(color)
    if width:
        args += ["--width", str(width)]
    if legends:
        args += ["--legend", ",".join(legends)]
    if colors:
        args += ["--colors", ",".join(colors)]
    if caption:
        args += ["--caption", caption]
    if ascii_only:
        args.append("--ascii")
    r = run("chart", args, text)
    if r.ok:
        return r.text
    out = "\n".join(_fallback_spark(s) for s in data) + "\n"
    return out + (caption + "\n" if caption else "")


def spark(values: Sequence) -> str:
    """One-line sparkline (no trailing newline)."""
    if not values:
        return ""
    r = run("spark", [], " ".join(_num(v) for v in values) + "\n")
    return r.text.rstrip("\n") if r.ok else _fallback_spark(values)


def bar(labels: Sequence[str], values: Sequence[float], *, width: int = 30,
        colors: Sequence[str] = (), color: Optional[str] = None, ascii_only: bool = False) -> str:
    """Horizontal bar chart, one 'label value' row per item."""
    if not values:
        return ""
    labels = [str(lab).replace("\n", " ") or str(i + 1) for i, lab in enumerate(labels)]
    text = "".join(f"{lab} {_num(v)}\n" for lab, v in zip(labels, values))
    args = ["--width", str(width)] + _color_args(color)
    if colors:
        args += ["--colors", ",".join(colors)]
    if ascii_only:
        args.append("--ascii")
    r = run("bar", args, text)
    return r.text if r.ok else _fallback_bar(labels, values, width)


def colorize(text: str, fg: str, *, bg: str = "", color: str = "truecolor") -> str:
    args = [fg, "--color", color] + (["--bg", bg] if bg else [])
    r = run("color", args, text)
    return r.text.rstrip("\n") if r.ok else text


def gradient(text: str, stops: Sequence[str] = ("magenta", "cyan"), *, color: str = "truecolor") -> str:
    r = run("gradient", ["--stops", ",".join(stops), "--color", color], text)
    return r.text.rstrip("\n") if r.ok else text


if __name__ == "__main__":  # quick self check: python misty_ascii.py
    print("binary:", find_binary() or "(none, using fallbacks)")
    print(table([["Host", "Load"], ["E15", 72], ["Venus", 41]], title="Misty"))
    print(chart([3, 1, 4, 1, 5, 9, 2, 6], height=5))
    print(spark([1, 5, 2, 8, 3, 9, 4]))
    print(bar(["tokens", "tools", "skills"], [1200, 340, 55]))
