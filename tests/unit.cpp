// SPDX-License-Identifier: MIT
// Unit tests for the engine. Plain asserts; prints each failure and exits non-zero.
#include "misty/ascii.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

#if defined(_WIN32) && !defined(__COSMOPOLITAN__)
static int setenv(const char* k, const char* v, int) { return _putenv_s(k, v); }
static int unsetenv(const char* k) { return _putenv_s(k, ""); }
#endif

using namespace misty::ascii;

static int failures = 0;
#define CHECK(cond)                                                                   \
    do {                                                                              \
        if (!(cond)) {                                                                \
            ++failures;                                                               \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond "\n"; \
        }                                                                             \
    } while (0)
#define CHECK_EQ(a, b)                                                                      \
    do {                                                                                    \
        auto va = (a);                                                                      \
        auto vb = (b);                                                                      \
        if (!(va == vb)) {                                                                  \
            ++failures;                                                                     \
            std::cerr << __FILE__ << ":" << __LINE__ << ": " #a " == " #b "\n  got:  [" << va \
                      << "]\n  want: [" << vb << "]\n";                                     \
        }                                                                                   \
    } while (0)

static void testWidth() {
    CHECK_EQ(displayWidth("hello"), 5);
    CHECK_EQ(displayWidth(""), 0);
    CHECK_EQ(displayWidth("日本語"), 6);           // CJK wide
    CHECK_EQ(displayWidth("한글"), 4);             // Hangul wide
    CHECK_EQ(displayWidth("e\xCC\x81"), 1);       // e + combining acute
    CHECK_EQ(displayWidth("🙂"), 2);              // emoji
    CHECK_EQ(displayWidth("\x1b[31mred\x1b[0m"), 3);
    CHECK_EQ(displayWidth("\x1b]8;;https://x.y\x07link\x1b]8;;\x07"), 4);  // OSC-8 link
    CHECK_EQ(displayWidth("┌─┐"), 3);
    CHECK_EQ(stripAnsi("\x1b[1;32mok\x1b[0m!"), std::string("ok!"));
    CHECK_EQ(codepointWidth(U'‍'), 0);  // ZWJ
}

static void testPadSnipWrap() {
    CHECK_EQ(pad("ab", 5, Align::Left), std::string("ab   "));
    CHECK_EQ(pad("ab", 5, Align::Right), std::string("   ab"));
    CHECK_EQ(pad("ab", 5, Align::Center), std::string(" ab  "));
    CHECK_EQ(pad("42", 4, Align::Auto), std::string("  42"));
    CHECK_EQ(pad("x", 3, Align::Auto), std::string("x  "));
    CHECK_EQ(pad("toolong", 3, Align::Left), std::string("toolong"));
    CHECK_EQ(pad("日本", 6, Align::Right), std::string("  日本"));
    CHECK_EQ(snip("hello world", 8), std::string("hello w~"));
    CHECK_EQ(snip("hello", 8), std::string("hello"));
    CHECK_EQ(snip("日本語", 4), std::string("日 ~"));
    CHECK_EQ(snip("abc", 0), std::string(""));
    auto w = wrap("the quick brown fox jumps", 10);
    CHECK_EQ(w.size(), size_t(3));
    CHECK_EQ(w[0], std::string("the quick"));
    CHECK_EQ(w[1], std::string("brown fox"));
    CHECK_EQ(w[2], std::string("jumps"));
    auto h = wrap("abcdefghij", 4);
    CHECK_EQ(h.size(), size_t(3));
    CHECK_EQ(h[2], std::string("ij"));
    auto l = lines("a\r\nb\n");
    CHECK_EQ(l.size(), size_t(3));
    CHECK_EQ(l[0], std::string("a"));
    CHECK(looksNumeric("3.14"));
    CHECK(looksNumeric("-1,234"));
    CHECK(looksNumeric("$12"));
    CHECK(looksNumeric("45%"));
    CHECK(looksNumeric("1e5"));
    CHECK(looksNumeric("10kB"));
    CHECK(!looksNumeric("abc"));
    CHECK(!looksNumeric(""));
    CHECK(!looksNumeric("v1.2.3x9"));
}

static void testUtf8() {
    std::u32string cps = decodeUtf8("a€😀");
    CHECK_EQ(cps.size(), size_t(3));
    CHECK(cps[1] == U'€');
    CHECK(cps[2] == U'😀');
    CHECK_EQ(encodeUtf8(U'€'), std::string("€"));
    CHECK(decodeUtf8("\xFF")[0] == 0xFFFD);
    CHECK(decodeUtf8("\xE2\x82")[0] == 0xFFFD);  // truncated
    CHECK_EQ(toAscii("┌─┐│x│└─┘"), std::string("+-+|x|+-+"));
    CHECK_EQ(toAscii("▁█"), std::string("_#"));
    CHECK_EQ(toAscii("日"), std::string("??"));
    CHECK_EQ(toAscii("Chloé Ñandú Øre ÿ"), std::string("Chloe Nandu Ore y"));
}

static void testColor() {
    auto red = parseColor("red");
    CHECK(red && *red == (Rgb{255, 0, 0}));
    CHECK(parseColor("Rebecca Purple") && *parseColor("Rebecca Purple") == (Rgb{0x66, 0x33, 0x99}));
    CHECK(parseColor("#0f0") && *parseColor("#0f0") == (Rgb{0, 255, 0}));
    CHECK(parseColor("#1e90ff") && *parseColor("#1e90ff") == (Rgb{0x1e, 0x90, 0xff}));
    CHECK(parseColor("rgb(1, 2, 3)") && *parseColor("rgb(1, 2, 3)") == (Rgb{1, 2, 3}));
    CHECK(parseColor("hsv(120,1,1)") && *parseColor("hsv(120,1,1)") == (Rgb{0, 255, 0}));
    CHECK(!parseColor("notacolour"));
    CHECK(!parseColor("#12345"));
    CHECK(!parseColor("rgb(300,0,0)"));
    CHECK(colorNames().size() >= 140);
    CHECK_EQ(toAnsi256({255, 0, 0}), 196);
    CHECK_EQ(toAnsi256({0, 0, 0}), 16);
    CHECK_EQ(toAnsi256({128, 128, 128}), 244);
    CHECK_EQ(toBasic16({255, 0, 0}), 9);
    CHECK_EQ(sgr({1, 2, 3}, ColorDepth::TrueColor), std::string("\x1b[38;2;1;2;3m"));
    CHECK_EQ(sgr({255, 0, 0}, ColorDepth::Ansi256, true), std::string("\x1b[48;5;196m"));
    CHECK_EQ(sgr({255, 0, 0}, ColorDepth::Basic16), std::string("\x1b[91m"));
    CHECK_EQ(colorize("hi", {255, 0, 0}, ColorDepth::None), std::string("hi"));
    CHECK_EQ(colorize("hi", {255, 0, 0}, ColorDepth::Basic16), std::string("\x1b[91mhi\x1b[0m"));
    // Gradient keeps text and width; endpoints hit the stops exactly.
    std::string g = gradient("abc", {{255, 0, 0}, {0, 0, 255}}, ColorDepth::TrueColor);
    CHECK_EQ(stripAnsi(g), std::string("abc"));
    CHECK(g.find("38;2;255;0;0") != std::string::npos);
    CHECK(g.find("38;2;0;0;255") != std::string::npos);
    CHECK(gradientAt({{255, 0, 0}, {0, 0, 255}}, 0.5) == (Rgb{255, 0, 255}));  // short hue path
    CHECK_EQ(gradient("abc", {{255, 0, 0}}, ColorDepth::None), std::string("abc"));

    // Environment detection.
    setenv("NO_COLOR", "1", 1);
    setenv("FORCE_COLOR", "3", 1);
    CHECK(detectColorDepth(true) == ColorDepth::None);
    unsetenv("NO_COLOR");
    CHECK(detectColorDepth(false) == ColorDepth::TrueColor);
    setenv("FORCE_COLOR", "0", 1);
    CHECK(detectColorDepth(true) == ColorDepth::None);
    unsetenv("FORCE_COLOR");
    CHECK(detectColorDepth(false) == ColorDepth::None);
    setenv("COLORTERM", "truecolor", 1);
    CHECK(detectColorDepth(true) == ColorDepth::TrueColor);
    unsetenv("COLORTERM");
    setenv("TERM", "xterm-256color", 1);
    CHECK(detectColorDepth(true) == ColorDepth::Ansi256);
    setenv("TERM", "dumb", 1);
    CHECK(detectColorDepth(true) == ColorDepth::None);
}

static void testTables() {
    auto rows = parseDelimited("a,b\n1,\"x, y\"\n2,\"say \"\"hi\"\"\"\n");
    CHECK_EQ(rows.size(), size_t(3));
    CHECK_EQ(rows[1][1], std::string("x, y"));
    CHECK_EQ(rows[2][1], std::string("say \"hi\""));
    auto tsv = parseDelimited("a\tb\n1\t2\n");
    CHECK_EQ(tsv[1][1], std::string("2"));
    auto md = parseDelimited("| a | b |\n|---|--:|\n| 1 | 2 \\| 3 |\n");
    CHECK_EQ(md.size(), size_t(2));
    CHECK_EQ(md[1][1], std::string("2 | 3"));
    auto semi = parseDelimited("x;y\n1;2\n");
    CHECK_EQ(semi[0][1], std::string("y"));
    auto multi = parseDelimited("a,b\n\"line1\nline2\",z\n");
    CHECK_EQ(multi[1][0], std::string("line1\nline2"));

    TableOptions o;
    std::string t = renderTable({{"h1", "h2"}, {"a", "1"}}, o);
    CHECK_EQ(t, std::string("┌────┬────┐\n"
                            "│ h1 │ h2 │\n"
                            "├────┼────┤\n"
                            "│ a  │  1 │\n"
                            "└────┴────┘\n"));
    o.format = TableFormat::Csv;
    CHECK_EQ(renderTable({{"h"}, {"=SUM(A1)"}, {"-5"}}, o), std::string("h\n'=SUM(A1)\n-5\n"));
    o.format = TableFormat::Markdown;
    CHECK_EQ(renderTable({{"n", "v"}, {"a", "10"}}, o),
             std::string("| n   |   v |\n| --- | --: |\n| a   |  10 |\n"));
    o.format = TableFormat::Html;
    std::string html = renderTable({{"<b>"}, {"x&y"}}, o);
    CHECK(html.find("<th>&lt;b&gt;</th>") != std::string::npos);
    CHECK(html.find("<td>x&amp;y</td>") != std::string::npos);
    CHECK(parseTableStyle("ROUNDED") == TableStyle::Rounded);
    CHECK(!parseTableStyle("nope"));
    CHECK(parseSortMode("dsc-numeric") == SortMode::DscNumeric);
    CHECK_EQ(renderTable({}, TableOptions{}), std::string(""));
}

static void testCharts() {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    ChartOptions o;
    o.height = 4;
    std::string p = plot({{1, 2, 3, 4, 5}}, o);
    CHECK(p.find("5.00") != std::string::npos);
    CHECK(p.find("1.00") != std::string::npos);
    CHECK(p.find("╭") != std::string::npos);
    std::string gap = plot({{1, nan, 3}}, o);
    CHECK(gap.find("╴") != std::string::npos);
    CHECK(gap.find("╶") != std::string::npos);
    CHECK_EQ(plot({}, o), std::string(""));
    CHECK_EQ(plot({{nan, nan}}, o), std::string(""));
    o.width = 10;
    std::string rs = plot({{0, 10}}, o);
    CHECK_EQ(lines(rs)[0].size() > 0, true);
    CHECK_EQ(sparkline({1, 2, 3, 4, 5, 6, 7, 8}), std::string("▁▂▃▄▅▆▇█"));
    CHECK_EQ(sparkline({5, 5}), std::string("▄▄"));
    CHECK_EQ(sparkline({1, nan, 8}), std::string("▁ █"));
    CHECK_EQ(barChart({"a", "bb"}, {2, 4}, 4), std::string("a  │██   2\nbb │████ 4\n"));
    CHECK_EQ(barChart({"x"}, {1}, 3) , std::string("x │███ 1\n"));
    auto s = parseSeries("1 2 3\n4 5 6\n");
    CHECK_EQ(s.size(), size_t(2));
    auto one = parseSeries("1\n2\n3\n");
    CHECK_EQ(one.size(), size_t(1));
    CHECK_EQ(one[0].size(), size_t(3));
    auto withNan = parseSeries("1, nan, 3");
    CHECK(std::isnan(withNan[0][1]));
    CHECK(parseSeries("hello").empty());
}

int main() {
    testWidth();
    testPadSnipWrap();
    testUtf8();
    testColor();
    testTables();
    testCharts();
    if (failures) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "all unit checks passed\n";
    return 0;
}
