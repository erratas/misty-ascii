// SPDX-License-Identifier: MIT
// Line chart algorithm ported from asciichart (MIT, Igor Kroitor) via asciichartpy; legends
// and caption placement as in asciigraph (BSD-3-Clause, Rohit Gupta). See CREDITS.md.
#include "misty/ascii.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace misty::ascii {
namespace {

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

std::string rtrim(std::string s) {
    while (!s.empty() && s.back() == ' ') s.pop_back();
    return s;
}

std::string fixed(double v, int precision) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.*f", std::max(0, precision), v);
    std::string s = buf;
    if (s == "-0" || s.find_first_not_of("-0.") == std::string::npos) {
        if (s[0] == '-') s.erase(0, 1);  // never print -0.00
    }
    return s;
}

// Shortest readable number: up to 2 decimals, no trailing zeros.
std::string shortNum(double v) {
    std::string s = fixed(v, 2);
    if (s.find('.') != std::string::npos) {
        while (s.back() == '0') s.pop_back();
        if (s.back() == '.') s.pop_back();
    }
    return s;
}

std::vector<double> resample(const std::vector<double>& s, int width) {
    if (width <= 0 || s.empty() || int(s.size()) == width) return s;
    std::vector<double> out(width);
    if (s.size() == 1) { std::fill(out.begin(), out.end(), s[0]); return out; }
    for (int i = 0; i < width; ++i) {
        double pos = width == 1 ? 0 : double(i) * double(s.size() - 1) / double(width - 1);
        size_t a = size_t(pos);
        if (a + 1 >= s.size()) { out[i] = s.back(); continue; }
        double f = pos - double(a);
        // NaN at either neighbour keeps the gap.
        out[i] = (std::isnan(s[a]) || std::isnan(s[a + 1])) ? (f < 0.5 ? s[a] : s[a + 1])
                                                             : s[a] + (s[a + 1] - s[a]) * f;
    }
    return out;
}

}  // namespace

std::string plot(const std::vector<std::vector<double>>& input, const ChartOptions& opt) {
    std::vector<std::vector<double>> series;
    for (auto& s : input) series.push_back(resample(s, opt.width));

    double minimum = std::numeric_limits<double>::infinity();
    double maximum = -minimum;
    size_t longest = 0;
    for (auto& s : series) {
        for (double v : s)
            if (!std::isnan(v)) { minimum = std::min(minimum, v); maximum = std::max(maximum, v); }
        longest = std::max(longest, s.size());
    }
    if (longest == 0 || !std::isfinite(minimum) || opt.height < 1) return "";
    if (opt.min) minimum = std::min(minimum, *opt.min);
    if (opt.max) maximum = std::max(maximum, *opt.max);

    const double interval = maximum - minimum;
    const int offset = std::max(opt.offset, 1);
    const double ratio = interval > 0 ? opt.height / interval : 1;
    // Math.round as in asciichart JS. (The Python port's floor/ceil bounds put points one
    // row away from their labels, so we do not follow it.)
    auto jsRound = [](double v) { return int(std::floor(v + 0.5)); };
    const int min2 = jsRound(minimum * ratio);
    const int max2 = jsRound(maximum * ratio);
    const int rows = max2 - min2;
    const int width = int(longest) + offset;

    auto scaled = [&](double y) { return jsRound(std::clamp(y, minimum, maximum) * ratio) - min2; };

    // Labels first so every label gets the same width.
    std::vector<std::string> labels;
    size_t labelWidth = 8;
    for (int y = min2; y <= max2; ++y) {
        double magnitude = maximum - ((y - min2) * interval / (rows ? rows : 1));
        labels.push_back(fixed(magnitude, opt.precision));
        labelWidth = std::max(labelWidth, labels.back().size());
    }

    // One string per cell; a label occupies a single (wide) cell as in asciichart.
    std::vector<std::vector<std::string>> grid(rows + 1, std::vector<std::string>(width, " "));
    for (int y = min2; y <= max2; ++y) {
        std::string label = labels[y - min2];
        label = std::string(labelWidth - label.size(), ' ') + label + " ";
        grid[y - min2][std::max(offset - int(label.size()), 0)] = label;
        grid[y - min2][offset - 1] = "┤";
    }
    // Zero line marker on the row that holds 0, when 0 is in range. (Upstream tests the
    // loop index instead, which marks the wrong row whenever min2 != 0.)
    if (minimum <= 0 && maximum >= 0) grid[rows - scaled(0)][offset - 1] = "┼";

    for (size_t j = 0; j < series.size(); ++j) {
        const auto& s = series[j];
        std::string open, close;
        if (opt.depth != ColorDepth::None && !opt.colors.empty()) {
            open = sgr(opt.colors[j % opt.colors.size()], opt.depth);
            close = kReset;
        }
        auto put = [&](int row, int col, const char* sym) { grid[row][col] = open + sym + close; };
        if (!s.empty() && !std::isnan(s[0]) && j == 0) grid[rows - scaled(s[0])][offset - 1] = "┼";
        for (size_t x = 0; x + 1 < s.size(); ++x) {
            bool n0 = std::isnan(s[x]), n1 = std::isnan(s[x + 1]);
            if (n0 && n1) continue;
            int col = int(x) + offset;
            if (n0) { put(rows - scaled(s[x + 1]), col, "╶"); continue; }
            if (n1) { put(rows - scaled(s[x]), col, "╴"); continue; }
            int y0 = scaled(s[x]), y1 = scaled(s[x + 1]);
            if (y0 == y1) { put(rows - y0, col, "─"); continue; }
            put(rows - y1, col, y0 > y1 ? "╰" : "╭");
            put(rows - y0, col, y0 > y1 ? "╮" : "╯");
            for (int y = std::min(y0, y1) + 1; y < std::max(y0, y1); ++y) put(rows - y, col, "│");
        }
    }

    std::string out;
    for (auto& row : grid) {
        std::string line;
        for (auto& cell : row) line += cell;
        out += rtrim(line) + "\n";
    }
    // Caption and legends centred under the plot area (asciigraph style).
    const int plotStart = int(labelWidth) + 2;
    auto centred = [&](const std::string& text) {
        int tw = displayWidth(text);
        int lead = plotStart + std::max(0, (int(longest) - tw) / 2);
        return std::string(lead, ' ') + text + "\n";
    };
    if (!opt.caption.empty()) out += "\n" + centred(opt.caption);
    if (!opt.legends.empty()) {
        std::string legend;
        for (size_t i = 0; i < opt.legends.size(); ++i) {
            std::string box = "■";
            if (opt.depth != ColorDepth::None && !opt.colors.empty())
                box = colorize(box, opt.colors[i % opt.colors.size()], opt.depth);
            legend += (i ? "   " : "") + box + " " + opt.legends[i];
        }
        out += (opt.caption.empty() ? "\n" : "") + centred(legend);
    }
    return out;
}

std::string sparkline(const std::vector<double>& values) {
    static const char* kBlocks[] = {"▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
    double mn = std::numeric_limits<double>::infinity(), mx = -mn;
    for (double v : values)
        if (!std::isnan(v)) { mn = std::min(mn, v); mx = std::max(mx, v); }
    std::string out;
    for (double v : values) {
        if (std::isnan(v)) { out += " "; continue; }
        int i = mx > mn ? int(std::lround((v - mn) / (mx - mn) * 7)) : 3;
        out += kBlocks[std::clamp(i, 0, 7)];
    }
    return out;
}

std::string barChart(const std::vector<std::string>& labels, const std::vector<double>& values,
                     int width) {
    static const char* kEighths[] = {"", "▏", "▎", "▍", "▌", "▋", "▊", "▉"};
    if (values.empty()) return "";
    width = std::max(width, 1);
    double mx = 0;
    for (double v : values)
        if (!std::isnan(v)) mx = std::max(mx, v);
    int lw = 0;
    for (size_t i = 0; i < values.size(); ++i)
        lw = std::max(lw, displayWidth(i < labels.size() ? labels[i] : std::to_string(i + 1)));
    std::string out;
    for (size_t i = 0; i < values.size(); ++i) {
        std::string label = i < labels.size() ? labels[i] : std::to_string(i + 1);
        double v = values[i];
        std::string bar;
        int cells = 0;
        if (!std::isnan(v) && v > 0 && mx > 0) {
            int eighths = int(std::lround(v / mx * width * 8));
            for (int k = 0; k < eighths / 8; ++k) bar += "█";
            bar += kEighths[eighths % 8];
            cells = eighths / 8 + (eighths % 8 ? 1 : 0);
        }
        std::string value = std::isnan(v) ? "n/a" : shortNum(v);
        out += pad(label, lw, Align::Left) + " │" + bar + std::string(width - cells + 1, ' ') + value + "\n";
    }
    return out;
}

std::vector<std::vector<double>> parseSeries(const std::string& text) {
    auto tokens = [](const std::string& line) {
        std::vector<double> out;
        std::string cur;
        auto flush = [&]() {
            if (cur.empty()) return;
            std::string l;
            for (char c : cur) l += char(std::tolower(static_cast<unsigned char>(c)));
            if (l == "nan" || l == "-" || l == "null" || l == "na" || l == "n/a") {
                out.push_back(kNaN);
            } else {
                char* end = nullptr;
                double v = std::strtod(cur.c_str(), &end);
                if (end && *end == 0 && std::isfinite(v)) out.push_back(v);
            }
            cur.clear();
        };
        for (char c : line) {
            if (c == ' ' || c == '\t' || c == ',' || c == ';' || c == '\r') flush();
            else cur += c;
        }
        flush();
        return out;
    };
    std::vector<std::vector<double>> multi;
    for (const std::string& l : lines(text)) {
        auto t = tokens(l);
        if (t.size() >= 2) multi.push_back(t);
    }
    if (multi.size() > 1) return multi;
    std::vector<double> all;
    for (const std::string& l : lines(text))
        for (double v : tokens(l)) all.push_back(v);
    if (all.empty()) return {};
    return {all};
}

}  // namespace misty::ascii
