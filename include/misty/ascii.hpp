// SPDX-License-Identifier: MIT
// misty-ascii: the shared ASCII engine. C++17, no Qt, no network.
#pragma once

#include <optional>
#include <string>
#include <vector>

namespace misty::ascii {

inline constexpr const char* kVersion = "0.1.0";

// ---------------------------------------------------------------- Text

/// Columns @p s occupies on a terminal: East Asian wide and emoji = 2, combining marks and
/// zero-width characters = 0, ANSI escape sequences (CSI and OSC) = 0.
int displayWidth(const std::string& s);
/// Width of one Unicode code point (0, 1 or 2). Control characters count 0.
int codepointWidth(char32_t cp);
/// @p s without ANSI escape sequences.
std::string stripAnsi(const std::string& s);

enum class Align { Left, Center, Right, Auto };  // Auto: numbers right, text left
/// True when @p s (ignoring ANSI and spaces) parses as a number, with optional % or units sign.
bool looksNumeric(const std::string& s);
/// @p s padded with spaces to @p width display columns. Longer text is returned unchanged.
std::string pad(const std::string& s, int width, Align a);
/// @p s cut to @p width display columns; when cut, the last columns hold @p indicator.
std::string snip(const std::string& s, int width, const std::string& indicator = "~");
/// Word wrap to @p width columns; words longer than the width are split hard.
std::vector<std::string> wrap(const std::string& s, int width);
/// Split on \n (a trailing \r on each line is dropped). Empty input gives one empty line.
std::vector<std::string> lines(const std::string& s);

/// Plain 7-bit ASCII version of @p s for old consoles and logs: box drawing becomes + - |,
/// blocks become a density ramp, other non-ASCII becomes '?' (one per display column).
/// ANSI escapes are kept.
std::string toAscii(const std::string& s);

/// Decode UTF-8 to code points (invalid bytes become U+FFFD).
std::u32string decodeUtf8(const std::string& s);
/// Encode one code point as UTF-8.
std::string encodeUtf8(char32_t cp);

// ---------------------------------------------------------------- Colour

struct Rgb {
    int r = 0, g = 0, b = 0;
    bool operator==(const Rgb& o) const { return r == o.r && g == o.g && b == o.b; }
};
enum class ColorDepth { None, Basic16, Ansi256, TrueColor };

/// CSS/X11 names (140), "#rgb", "#rrggbb", "rgb(r,g,b)", "hsv(h,s,v)". Case-insensitive.
std::optional<Rgb> parseColor(const std::string& name);
/// Every colour name parseColor() knows, sorted.
std::vector<std::string> colorNames();
/// Parse "none|never|16|256|truecolor|24bit"; nullopt for anything else ("auto" too).
std::optional<ColorDepth> parseColorDepth(const std::string& s);
std::string colorDepthName(ColorDepth d);

/// SGR escape that selects @p c as foreground (or background) at depth @p d. Empty for None.
std::string sgr(Rgb c, ColorDepth d, bool background = false);
inline constexpr const char* kReset = "\x1b[0m";
/// @p text wrapped in colour escapes (and a reset). Unchanged at ColorDepth::None.
std::string colorize(const std::string& text, Rgb fg, ColorDepth d, std::optional<Rgb> bg = {});
/// Each visible character coloured along a gradient through @p stops, interpolated in HSV.
std::string gradient(const std::string& text, const std::vector<Rgb>& stops, ColorDepth d);
/// Colour at position @p t (0..1) along @p stops, HSV interpolation (shortest hue path).
Rgb gradientAt(const std::vector<Rgb>& stops, double t);
/// Nearest xterm 256-colour index and nearest of the 16 basic colours.
int toAnsi256(Rgb c);
int toBasic16(Rgb c);
/// Depth for this process: NO_COLOR wins, then FORCE_COLOR (0..3), then a non-TTY gives None,
/// then COLORTERM=truecolor|24bit, then TERM containing 256color, then dumb, else Basic16.
ColorDepth detectColorDepth(bool isTty);

// ---------------------------------------------------------------- Tables (go-pretty set)

enum class TableStyle { Default, Light, Rounded, Bold, Double, Ascii, None };
enum class TableFormat { Text, Markdown, Csv, Tsv, Html };
enum class SortMode { None, Asc, Dsc, AscNumeric, DscNumeric };
struct ColumnConfig {
    Align align = Align::Auto;
    int widthMax = 0;  // 0 = unlimited; longer cells wrap
    bool hidden = false;
};
struct TableOptions {
    TableStyle style = TableStyle::Light;
    TableFormat format = TableFormat::Text;
    bool header = true, footer = false, separateRows = false, border = true, autoIndex = false;
    int sortColumn = -1;
    SortMode sortMode = SortMode::None;
    std::string title, caption;
    std::vector<ColumnConfig> columns;
};
using Row = std::vector<std::string>;

/// The first row is the header when opt.header, the last is the footer when opt.footer.
std::string renderTable(const std::vector<Row>& rows, const TableOptions& opt);
/// Rows from CSV (RFC 4180 quotes), TSV, ';' or '|' text, or a Markdown table. With
/// @p delimiter 0 the delimiter is detected. Markdown separator rows are dropped.
std::vector<Row> parseDelimited(const std::string& text, char delimiter = 0);
std::vector<std::string> tableStyleNames();
std::optional<TableStyle> parseTableStyle(const std::string& s);
std::optional<TableFormat> parseTableFormat(const std::string& s);
std::optional<SortMode> parseSortMode(const std::string& s);

// ---------------------------------------------------------------- Charts (asciichart)

struct ChartOptions {
    int height = 10;
    int width = 0;  // 0 = one column per value; otherwise series are resampled
    std::optional<double> min, max;
    int offset = 3, precision = 2;
    std::string caption;
    std::vector<std::string> legends;
    std::vector<Rgb> colors;
    ColorDepth depth = ColorDepth::None;
};
/// Line chart of one or more series sharing a y axis. NaN values leave a gap.
std::string plot(const std::vector<std::vector<double>>& series, const ChartOptions& opt);
/// One line of block characters ▁▂▃▄▅▆▇█ (NaN = space).
std::string sparkline(const std::vector<double>& values);
/// Horizontal bars, labels left, values right, the longest bar @p width cells (eighth blocks).
std::string barChart(const std::vector<std::string>& labels, const std::vector<double>& values,
                     int width = 40);
/// Several numbers per line on more than one line: each line is a series. Otherwise every
/// number in the text forms one series. "nan" / "-" / "null" tokens become NaN gaps.
std::vector<std::vector<double>> parseSeries(const std::string& text);

}  // namespace misty::ascii
