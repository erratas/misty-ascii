# SPDX-License-Identifier: MIT
"""Tests for misty_ascii.py. With MISTY_BIN pointing at a built binary they exercise the
engine; the fallback tests force MISTY_BIN=off. Run: python -m pytest bindings/python"""
import importlib
import os
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent))
import misty_ascii  # noqa: E402


def fresh(monkeypatch, value):
    monkeypatch.setenv("MISTY_BIN", value)
    importlib.reload(misty_ascii)
    misty_ascii._cache.clear()
    return misty_ascii


_bin = os.environ.get("MISTY_BIN", "")
needs_engine = pytest.mark.skipif(not (_bin and Path(_bin).is_file()),
                                  reason="set MISTY_BIN to a built misty binary")


@needs_engine
def test_table_engine():
    out = misty_ascii.table([["Host", "Load"], ["E15", 72], ["Venus", 41]])
    assert out.splitlines()[0].startswith("╭")
    assert "│ Venus │   41 │" in out


@needs_engine
def test_table_options_and_text_input():
    out = misty_ascii.table("a,b\n2,x\n10,y\n", style="ascii", sort=0, descending=True, numeric=True,
                            title="T", caption="cap")
    lines = out.splitlines()
    assert lines[-1] == "cap"
    assert lines.index("| 10 | y |") < lines.index("|  2 | x |")


@needs_engine
def test_table_cells_with_commas_and_quotes():
    out = misty_ascii.table([["k", "v"], ["a, b", 'say "hi"']])
    assert "a, b" in out and 'say "hi"' in out


@needs_engine
def test_chart_single_and_multi_and_gaps():
    one = misty_ascii.chart([1, 2, 3, 4], height=3)
    assert "4.00" in one and "1.00" in one
    two = misty_ascii.chart([[1, 2, 3], [3, 2, 1]], height=3, legends=["up", "down"])
    assert "■ up" in two and "■ down" in two
    gap = misty_ascii.chart([1, None, 3, float("nan"), 5], height=4)
    assert "╴" in gap or "╶" in gap


@needs_engine
def test_spark_bar_colour():
    assert misty_ascii.spark([1, 2, 3, 4, 5, 6, 7, 8]) == "▁▂▃▄▅▆▇█"
    b = misty_ascii.bar(["big apples", "kiwi"], [10, 5], width=10)
    assert b.splitlines()[0].startswith("big apples │██████████")
    assert misty_ascii.colorize("hi", "red", color="16") == "\x1b[91mhi\x1b[0m"
    assert "\x1b[38;2;" in misty_ascii.gradient("misty")
    caps = misty_ascii.capabilities()
    assert caps["ok"] and "table" in caps["subcommands"]


@needs_engine
def test_errors_are_results_not_exceptions():
    r = misty_ascii.run("banner", ["hi"])
    assert not r.ok and r.code == 3 and "planned" in r.error
    r = misty_ascii.run("table", ["--style", "wavy"], "a\n")
    assert not r.ok and r.code == 2


def test_fallbacks_without_engine(monkeypatch):
    m = fresh(monkeypatch, "off")
    assert not m.available()
    assert m.spark([1, 2, 3, 4, 5, 6, 7, 8]) == "▁▂▃▄▅▆▇█"
    assert m.spark([1, None, 8]) == "▁ █"
    t = m.table([["Host", "Load"], ["E15", 72]])
    assert t.splitlines() == ["Host  Load", "----  ----", "E15   72"]
    b = m.bar(["a", "bb"], [2, 4], width=4)
    assert b.splitlines()[1] == "bb │████ 4"
    assert m.chart([1, 2, 3]).strip() == "▁▅█"  # same rounding as the engine
    assert m.spark([0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70]) == "▁▂▂▃▃▄▄▅▅▆▆▇▇██"  # engine output for the same input
    assert m.colorize("x", "red") == "x"
    assert m.run("table").code == 3


def test_missing_path_falls_through(monkeypatch, tmp_path):
    m = fresh(monkeypatch, str(tmp_path / "nope"))
    # A wrong MISTY_BIN never raises; discovery continues or the fallback is used.
    assert isinstance(m.spark([1, 2]), str)
