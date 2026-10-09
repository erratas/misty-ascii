// SPDX-License-Identifier: MIT
// `misty`: command-line front end for the misty-ascii engine.
// Contract (kate docs/misty/TECH-NOTES.md section 2): art on stdout, errors on stderr,
// --json for machines, exit codes 0 ok / 1 bad input / 2 bad usage / 3 unavailable.
#include "misty/ascii.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32) && !defined(__COSMOPOLITAN__)
#include <io.h>
#define MISTY_ISATTY(fd) _isatty(fd)
#else
#include <sys/ioctl.h>
#include <unistd.h>
#define MISTY_ISATTY(fd) isatty(fd)
#endif

namespace ma = misty::ascii;

namespace {

enum Exit { kOk = 0, kBadInput = 1, kBadUsage = 2, kUnavailable = 3 };

struct Failure {
    int code;
    std::string message;
};

[[noreturn]] void fail(int code, const std::string& msg) { throw Failure{code, msg}; }

const std::set<std::string> kValueOptions = {
    "width", "color", "style", "format", "sort", "title", "caption", "height", "min", "max",
    "offset", "precision", "colors", "legend", "stops", "delimiter", "bg", "labels",
    "max-width", "hide", "align",
};
const std::set<std::string> kFlags = {
    "json", "ascii", "help", "no-header", "footer", "separate-rows", "no-border", "index",
    "asc", "dsc", "numeric", "list", "version",
};

struct Args {
    std::string command;
    std::vector<std::string> positional;
    std::map<std::string, std::string> opts;
    std::set<std::string> flags;

    bool has(const std::string& f) const { return flags.count(f) > 0; }
    std::string get(const std::string& k, const std::string& def = "") const {
        auto it = opts.find(k);
        return it == opts.end() ? def : it->second;
    }
    bool hasOpt(const std::string& k) const { return opts.count(k) > 0; }
};

Args parseArgs(int argc, char** argv) {
    Args a;
    bool onlyPositional = false;
    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        if (!onlyPositional && s == "--") { onlyPositional = true; continue; }
        if (!onlyPositional && s.size() > 2 && s.compare(0, 2, "--") == 0) {
            std::string key = s.substr(2), value;
            bool inlineValue = false;
            size_t eq = key.find('=');
            if (eq != std::string::npos) { value = key.substr(eq + 1); key = key.substr(0, eq); inlineValue = true; }
            if (kFlags.count(key)) {
                if (inlineValue) fail(kBadUsage, "--" + key + " takes no value");
                a.flags.insert(key);
            } else if (kValueOptions.count(key)) {
                if (!inlineValue) {
                    if (i + 1 >= argc) fail(kBadUsage, "--" + key + " needs a value");
                    value = argv[++i];
                }
                a.opts[key] = value;
            } else {
                fail(kBadUsage, "unknown option --" + key + " (see misty --help)");
            }
            continue;
        }
        if (!onlyPositional && s == "-h") { a.flags.insert("help"); continue; }
        if (!onlyPositional && s == "-V") { a.flags.insert("version"); continue; }
        if (a.command.empty() && !onlyPositional) a.command = s;
        else a.positional.push_back(s);
    }
    return a;
}

int toInt(const Args& a, const std::string& key, int def, int lo, int hi) {
    if (!a.hasOpt(key)) return def;
    std::string v = a.get(key);
    char* end = nullptr;
    long x = std::strtol(v.c_str(), &end, 10);
    if (v.empty() || *end != 0 || x < lo || x > hi)
        fail(kBadUsage, "--" + key + " must be a whole number from " + std::to_string(lo) + " to " +
                            std::to_string(hi));
    return int(x);
}

double toDouble(const std::string& key, const std::string& v) {
    char* end = nullptr;
    double x = std::strtod(v.c_str(), &end);
    if (v.empty() || *end != 0 || !std::isfinite(x)) fail(kBadUsage, "--" + key + " must be a number");
    return x;
}

std::vector<std::string> splitList(const std::string& s, char sep = ',') {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == sep) { out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    out.push_back(cur);
    return out;
}

std::string readAll(std::istream& in) {
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// Data from the positional file arguments (or stdin when there are none / "-").
std::string readInput(const std::vector<std::string>& files) {
    if (files.empty()) return readAll(std::cin);
    std::string all;
    for (const std::string& f : files) {
        if (f == "-") { all += readAll(std::cin); continue; }
        std::ifstream in(f, std::ios::binary);
        if (!in) fail(kBadInput, "cannot open " + f);
        all += readAll(in);
    }
    return all;
}

// Text from positional args joined by spaces, or stdin when there are none.
std::string readText(const std::vector<std::string>& words) {
    if (words.empty() || (words.size() == 1 && words[0] == "-")) {
        std::string s = readAll(std::cin);
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
        return s;
    }
    std::string s;
    for (size_t i = 0; i < words.size(); ++i) s += (i ? " " : "") + words[i];
    return s;
}

int terminalWidth() {
#if !(defined(_WIN32) && !defined(__COSMOPOLITAN__))
    struct winsize ws {};
    if (ioctl(1, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) return ws.ws_col;
#endif
    return 0;
}

// --width, then $COLUMNS, then the terminal, else 80.
int resolveWidth(const Args& a) {
    if (a.hasOpt("width")) return toInt(a, "width", 80, 1, 10000);
    if (const char* c = std::getenv("COLUMNS")) {
        int v = std::atoi(c);
        if (v > 0) return v;
    }
    int t = terminalWidth();
    return t > 0 ? t : 80;
}

ma::ColorDepth resolveColor(const Args& a) {
    std::string mode = a.get("color", "auto");
    if (mode == "auto") return ma::detectColorDepth(MISTY_ISATTY(1));
    auto d = ma::parseColorDepth(mode);
    if (!d) fail(kBadUsage, "--color must be auto, never, 16, 256 or truecolor");
    return *d;
}

ma::Rgb colorOrFail(const std::string& name) {
    auto c = ma::parseColor(name);
    if (!c) fail(kBadUsage, "unknown colour '" + name + "' (try misty color --list)");
    return *c;
}

std::string jsonEscape(const std::string& s) {
    std::string o;
    for (unsigned char c : s) {
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof buf, "\\u%04x", c);
                    o += buf;
                } else {
                    o += char(c);
                }
        }
    }
    return o;
}

std::string jsonList(const std::vector<std::string>& v) {
    std::string o = "[";
    for (size_t i = 0; i < v.size(); ++i) o += (i ? "," : "") + std::string("\"") + jsonEscape(v[i]) + "\"";
    return o + "]";
}

// ------------------------------------------------------------------ help

const char* kHelp =
    "misty: ASCII art engine (tables, charts, colour)\n"
    "\n"
    "Usage: misty <command> [options] [file|text...]\n"
    "\n"
    "Commands:\n"
    "  table         CSV, TSV, Markdown or delimited text to a table\n"
    "  chart         numbers to a line chart (one series per line)\n"
    "  spark         numbers to a one-line sparkline\n"
    "  bar           'label value' lines to a horizontal bar chart\n"
    "  color         colour text: misty color red Hello  (--list for names)\n"
    "  gradient      colour text along a gradient: misty gradient --stops red,blue Hello\n"
    "  capabilities  what this build can do (use --json)\n"
    "  banner image diagram rain sky fonts   planned, exit 3 for now\n"
    "\n"
    "Common options:\n"
    "  --json                 {\"ok\":true,\"text\":...,\"width\":N,\"height\":N}\n"
    "  --width N              output width (default $COLUMNS, terminal, else 80)\n"
    "  --color MODE           auto|never|16|256|truecolor (auto honours NO_COLOR, FORCE_COLOR)\n"
    "  --ascii                plain 7-bit ASCII output\n"
    "  --help                 help for a command: misty table --help\n"
    "  --version              print the version\n"
    "\n"
    "Exit codes: 0 ok, 1 bad input, 2 bad usage, 3 not available in this build.\n";

const std::map<std::string, std::string> kCommandHelp = {
    {"table",
     "misty table [options] [file...]   (stdin when no file)\n"
     "  --style S        default|light|rounded|bold|double|ascii|none (default light)\n"
     "  --format F       text|markdown|csv|tsv|html (default text)\n"
     "  --delimiter C    input delimiter (default: detect , tab ; |  or Markdown)\n"
     "  --no-header      first row is data, not a header\n"
     "  --footer         last row is a footer\n"
     "  --separate-rows  rule between every row\n"
     "  --no-border      no outer frame\n"
     "  --index          add a row number column\n"
     "  --sort N         sort by column N (1-based); --asc (default) or --dsc; --numeric\n"
     "  --align LIST     per column: l,c,r,a (auto) e.g. --align l,r,r\n"
     "  --max-width LIST per column max width, longer cells wrap, e.g. --max-width 0,20\n"
     "  --hide LIST      columns to hide (1-based) e.g. --hide 2,4\n"
     "  --title T        title above the table\n"
     "  --caption T      caption below the table\n"},
    {"chart",
     "misty chart [options] [file...]   numbers, one series per line\n"
     "  --height N       rows of plot (default 10)\n"
     "  --width N        resample every series to N columns\n"
     "  --min X --max X  extend the y range\n"
     "  --offset N       label offset (default 3)\n"
     "  --precision N    label decimals (default 2)\n"
     "  --caption T      caption under the plot\n"
     "  --legend LIST    series names, comma separated\n"
     "  --colors LIST    series colours, e.g. red,#00ff00\n"
     "  NaN, nan, -, null in the data leave a gap\n"},
    {"spark", "misty spark [file...] | misty spark 1 5 2 8   one-line sparkline\n"},
    {"bar",
     "misty bar [options] [file...]   lines of 'label value' (or 'label,value')\n"
     "  --width N        longest bar (default fits the terminal, at most 40)\n"
     "  --colors LIST    bar colours\n"},
    {"color",
     "misty color NAME [text...]   colour text (stdin when no text)\n"
     "  --bg NAME        background colour\n"
     "  --list           every colour name with a swatch\n"
     "  NAME: CSS name, #rgb, #rrggbb, rgb(r,g,b), hsv(h,s,v)\n"},
    {"gradient",
     "misty gradient [--stops LIST] [text...]   HSV gradient across each line\n"
     "  --stops LIST     colours, default magenta,cyan\n"},
    {"capabilities", "misty capabilities [--json]   subcommands, styles and formats of this build\n"},
};

const std::vector<std::string> kAvailable = {"table", "chart", "spark", "bar", "color", "gradient",
                                             "capabilities"};
const std::map<std::string, std::string> kPlanned = {
    {"banner", "E4"}, {"fonts", "E4"}, {"image", "E5"}, {"rain", "E6"}, {"sky", "E6"},
    {"diagram", "E8"},
};

// ------------------------------------------------------------------ commands

std::string cmdTable(const Args& a) {
    ma::TableOptions opt;
    if (a.hasOpt("style")) {
        auto s = ma::parseTableStyle(a.get("style"));
        if (!s) fail(kBadUsage, "unknown --style " + a.get("style"));
        opt.style = *s;
    }
    if (a.hasOpt("format")) {
        auto f = ma::parseTableFormat(a.get("format"));
        if (!f) fail(kBadUsage, "unknown --format " + a.get("format"));
        opt.format = *f;
    }
    if (a.has("ascii") && opt.style != ma::TableStyle::None && opt.style != ma::TableStyle::Default)
        opt.style = ma::TableStyle::Ascii;
    opt.header = !a.has("no-header");
    opt.footer = a.has("footer");
    opt.separateRows = a.has("separate-rows");
    opt.border = !a.has("no-border");
    opt.autoIndex = a.has("index");
    opt.title = a.get("title");
    opt.caption = a.get("caption");
    if (a.has("asc") && a.has("dsc")) fail(kBadUsage, "--asc and --dsc together");
    if (a.hasOpt("sort")) {
        opt.sortColumn = toInt(a, "sort", 1, 1, 100000) - 1;
        bool num = a.has("numeric");
        opt.sortMode = a.has("dsc") ? (num ? ma::SortMode::DscNumeric : ma::SortMode::Dsc)
                                    : (num ? ma::SortMode::AscNumeric : ma::SortMode::Asc);
    } else if (a.has("asc") || a.has("dsc") || a.has("numeric")) {
        fail(kBadUsage, "--asc, --dsc and --numeric need --sort N");
    }
    char delim = 0;
    if (a.hasOpt("delimiter")) {
        std::string d = a.get("delimiter");
        if (d == "tab" || d == "\\t") delim = '\t';
        else if (d.size() == 1) delim = d[0];
        else fail(kBadUsage, "--delimiter must be one character (or tab)");
    }
    std::vector<ma::Row> rows = ma::parseDelimited(readInput(a.positional), delim);
    if (rows.empty()) fail(kBadInput, "no table rows in the input");
    size_t ncols = 0;
    for (auto& r : rows) ncols = std::max(ncols, r.size());
    opt.columns.resize(ncols);
    if (a.hasOpt("align")) {
        auto list = splitList(a.get("align"));
        for (size_t i = 0; i < list.size() && i < ncols; ++i) {
            const std::string& v = list[i];
            if (v == "l" || v == "left") opt.columns[i].align = ma::Align::Left;
            else if (v == "c" || v == "center" || v == "centre") opt.columns[i].align = ma::Align::Center;
            else if (v == "r" || v == "right") opt.columns[i].align = ma::Align::Right;
            else if (v == "a" || v == "auto" || v.empty()) opt.columns[i].align = ma::Align::Auto;
            else fail(kBadUsage, "--align values are l, c, r or a");
        }
    }
    if (a.hasOpt("max-width")) {
        auto list = splitList(a.get("max-width"));
        for (size_t i = 0; i < list.size() && i < ncols; ++i) {
            if (list[i].empty()) continue;
            opt.columns[i].widthMax = int(toDouble("max-width", list[i]));
            if (opt.columns[i].widthMax < 0) fail(kBadUsage, "--max-width must not be negative");
        }
    }
    if (a.hasOpt("hide")) {
        for (auto& v : splitList(a.get("hide"))) {
            int c = int(toDouble("hide", v));
            if (c < 1) fail(kBadUsage, "--hide takes 1-based column numbers");
            if (size_t(c) <= ncols) opt.columns[c - 1].hidden = true;
        }
    }
    return ma::renderTable(rows, opt);
}

std::vector<ma::Rgb> colorList(const Args& a, const std::string& key) {
    std::vector<ma::Rgb> out;
    if (!a.hasOpt(key)) return out;
    for (auto& n : splitList(a.get(key))) out.push_back(colorOrFail(n));
    return out;
}

std::string cmdChart(const Args& a, ma::ColorDepth depth) {
    ma::ChartOptions opt;
    opt.height = toInt(a, "height", 10, 1, 1000);
    if (a.hasOpt("width")) opt.width = toInt(a, "width", 0, 2, 10000);
    if (a.hasOpt("min")) opt.min = toDouble("min", a.get("min"));
    if (a.hasOpt("max")) opt.max = toDouble("max", a.get("max"));
    opt.offset = toInt(a, "offset", 3, 1, 100);
    opt.precision = toInt(a, "precision", 2, 0, 12);
    opt.caption = a.get("caption");
    if (a.hasOpt("legend")) opt.legends = splitList(a.get("legend"));
    opt.colors = colorList(a, "colors");
    opt.depth = depth;
    auto series = ma::parseSeries(readInput(a.positional));
    if (series.empty()) fail(kBadInput, "no numbers in the input");
    std::string out = ma::plot(series, opt);
    if (out.empty()) fail(kBadInput, "nothing to plot (all values missing)");
    return out;
}

std::vector<double> numbersFromArgsOrInput(const Args& a) {
    // Positional numbers (misty spark 1 2 3) or files / stdin.
    bool allNumbers = !a.positional.empty();
    for (auto& p : a.positional) {
        char* end = nullptr;
        std::strtod(p.c_str(), &end);
        if (p.empty() || *end != 0) { allNumbers = false; break; }
    }
    std::string text;
    if (allNumbers) {
        for (auto& p : a.positional) text += p + " ";
    } else {
        text = readInput(a.positional);
    }
    std::vector<double> all;
    for (auto& s : ma::parseSeries(text)) all.insert(all.end(), s.begin(), s.end());
    return all;
}

std::string cmdSpark(const Args& a) {
    auto v = numbersFromArgsOrInput(a);
    if (v.empty()) fail(kBadInput, "no numbers in the input");
    return ma::sparkline(v) + "\n";
}

std::string cmdBar(const Args& a, ma::ColorDepth depth) {
    std::vector<std::string> labels;
    std::vector<double> values;
    std::string text = readInput(a.positional);
    for (const std::string& raw : ma::lines(text)) {
        std::string line = raw;
        size_t e = line.find_last_not_of(" \t");
        if (e == std::string::npos) continue;
        line = line.substr(0, e + 1);
        size_t cut = line.find_last_of(" \t,;:");
        std::string label = cut == std::string::npos ? "" : line.substr(0, cut);
        std::string num = cut == std::string::npos ? line : line.substr(cut + 1);
        while (!label.empty() && (label.back() == ' ' || label.back() == '\t' || label.back() == ',' ||
                                  label.back() == ';' || label.back() == ':'))
            label.pop_back();
        char* end = nullptr;
        double v = std::strtod(num.c_str(), &end);
        if (num.empty() || *end != 0) fail(kBadInput, "no number at the end of line: " + raw);
        labels.push_back(label.empty() ? std::to_string(values.size() + 1) : label);
        values.push_back(v);
    }
    if (values.empty()) fail(kBadInput, "no 'label value' lines in the input");
    int width = 40;
    if (a.hasOpt("width")) {
        width = toInt(a, "width", 40, 1, 10000);
    } else {
        int lw = 0;
        for (auto& l : labels) lw = std::max(lw, ma::displayWidth(l));
        width = std::clamp(resolveWidth(a) - lw - 12, 1, 40);
    }
    std::string out = ma::barChart(labels, values, width);
    auto colors = colorList(a, "colors");
    if (colors.empty() || depth == ma::ColorDepth::None) return out;
    // Colour each bar (the part after the axis) line by line.
    std::string coloured;
    auto ls = ma::lines(out);
    if (!ls.empty() && ls.back().empty()) ls.pop_back();
    for (size_t i = 0; i < ls.size(); ++i) {
        size_t axis = ls[i].find("│");
        if (axis == std::string::npos) { coloured += ls[i] + "\n"; continue; }
        size_t start = axis + std::string("│").size();
        size_t endBar = ls[i].find(' ', start);
        if (endBar == std::string::npos) endBar = ls[i].size();
        coloured += ls[i].substr(0, start) +
                    ma::colorize(ls[i].substr(start, endBar - start), colors[i % colors.size()], depth) +
                    ls[i].substr(endBar) + "\n";
    }
    return coloured;
}

std::string cmdColor(const Args& a, ma::ColorDepth depth) {
    if (a.has("list")) {
        std::string out;
        for (auto& n : ma::colorNames()) {
            ma::Rgb c = *ma::parseColor(n);
            char hex[8];
            std::snprintf(hex, sizeof hex, "#%02x%02x%02x", c.r, c.g, c.b);
            std::string swatch = depth == ma::ColorDepth::None ? "" : ma::colorize("████", c, depth) + " ";
            out += swatch + ma::pad(n, 22, ma::Align::Left) + hex + "\n";
        }
        return out;
    }
    if (a.positional.empty()) fail(kBadUsage, "usage: misty color NAME [text...]");
    ma::Rgb fg = colorOrFail(a.positional[0]);
    std::optional<ma::Rgb> bg;
    if (a.hasOpt("bg")) bg = colorOrFail(a.get("bg"));
    std::vector<std::string> words(a.positional.begin() + 1, a.positional.end());
    return ma::colorize(readText(words), fg, depth, bg) + "\n";
}

std::string cmdGradient(const Args& a, ma::ColorDepth depth) {
    std::vector<ma::Rgb> stops;
    for (auto& n : splitList(a.get("stops", "magenta,cyan"))) stops.push_back(colorOrFail(n));
    if (stops.empty()) fail(kBadUsage, "--stops needs at least one colour");
    return ma::gradient(readText(a.positional), stops, depth) + "\n";
}

std::string cmdCapabilities(const Args& a) {
    std::vector<std::string> planned;
    for (auto& p : kPlanned) planned.push_back(p.first);
    if (a.has("json")) {
        // Printed as-is: this object already is the machine answer.
        return std::string("{\"ok\":true,\"version\":\"") + ma::kVersion + "\",\"subcommands\":" +
               jsonList(kAvailable) + ",\"planned\":" + jsonList(planned) +
               ",\"table_styles\":" + jsonList(ma::tableStyleNames()) +
               ",\"table_formats\":[\"text\",\"markdown\",\"csv\",\"tsv\",\"html\"]" +
               ",\"color_modes\":[\"auto\",\"never\",\"16\",\"256\",\"truecolor\"]" +
               ",\"colors\":" + std::to_string(ma::colorNames().size()) + ",\"fonts\":[]}\n";
    }
    std::string out = std::string("misty ") + ma::kVersion + "\n";
    out += "available: ";
    for (size_t i = 0; i < kAvailable.size(); ++i) out += (i ? ", " : "") + kAvailable[i];
    out += "\nplanned:   ";
    for (size_t i = 0; i < planned.size(); ++i) out += (i ? ", " : "") + planned[i];
    out += "\ntable styles: ";
    auto st = ma::tableStyleNames();
    for (size_t i = 0; i < st.size(); ++i) out += (i ? ", " : "") + st[i];
    out += "\ncolour names: " + std::to_string(ma::colorNames().size()) + "\n";
    return out;
}

int emit(const Args& a, const std::string& text) {
    if (a.has("json")) {
        int w = 0, h = 0;
        auto ls = ma::lines(text);
        if (!ls.empty() && ls.back().empty()) ls.pop_back();
        for (auto& l : ls) w = std::max(w, ma::displayWidth(l));
        h = int(ls.size());
        std::cout << "{\"ok\":true,\"text\":\"" << jsonEscape(text) << "\",\"width\":" << w
                  << ",\"height\":" << h << "}\n";
    } else {
        std::cout << text;
    }
    std::cout.flush();
    return kOk;
}

}  // namespace

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    bool json = false;
    for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == "--json") json = true;
    try {
        Args a = parseArgs(argc, argv);
        if (a.has("version") || a.command == "version") {
            std::cout << "misty " << ma::kVersion << "\n";
            return kOk;
        }
        if (a.command.empty() || a.command == "help") {
            if (a.command == "help" && !a.positional.empty() && kCommandHelp.count(a.positional[0])) {
                std::cout << kCommandHelp.at(a.positional[0]);
                return kOk;
            }
            if (a.command.empty() && !a.has("help")) {
                std::cerr << kHelp;
                return kBadUsage;
            }
            std::cout << kHelp;
            return kOk;
        }
        if (a.has("help")) {
            auto it = kCommandHelp.find(a.command);
            std::cout << (it != kCommandHelp.end() ? it->second : std::string(kHelp));
            return kOk;
        }
        auto planned = kPlanned.find(a.command);
        if (planned != kPlanned.end())
            fail(kUnavailable, a.command + " is not in this build yet (planned, card " + planned->second + ")");

        ma::ColorDepth depth = resolveColor(a);
        std::string out;
        if (a.command == "table") out = cmdTable(a);
        else if (a.command == "chart") out = cmdChart(a, depth);
        else if (a.command == "spark") out = cmdSpark(a);
        else if (a.command == "bar") out = cmdBar(a, depth);
        else if (a.command == "color" || a.command == "colour") out = cmdColor(a, depth);
        else if (a.command == "gradient") out = cmdGradient(a, depth);
        else if (a.command == "capabilities") {
            std::cout << cmdCapabilities(a);
            return kOk;
        } else fail(kBadUsage, "unknown command '" + a.command + "' (see misty --help)");

        if (a.has("ascii")) out = ma::toAscii(out);
        return emit(a, out);
    } catch (const Failure& f) {
        std::cerr << "misty: " << f.message << "\n";
        if (json) std::cout << "{\"ok\":false,\"error\":\"" << jsonEscape(f.message) << "\"}\n";
        return f.code;
    } catch (const std::exception& e) {
        std::cerr << "misty: internal error: " << e.what() << "\n";
        if (json) std::cout << "{\"ok\":false,\"error\":\"internal error\"}\n";
        return kBadInput;
    }
}
