// SPDX-License-Identifier: MIT
// Tables: feature set modelled on go-pretty (MIT, jedib0t), box characters as in FTXUI and
// ASCII-Data. Our own implementation.
#include "misty/ascii.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace misty::ascii {
namespace {

struct Box {
    // Top, middle (separator), bottom rows: left, fill, junction, right. Then vertical bars.
    const char *tl, *th, *tj, *tr;
    const char *ml, *mh, *mj, *mr;
    const char *bl, *bh, *bj, *br;
    const char *v;  // outer and inner vertical
    const char *hh, *hj, *hl, *hr;  // header separator fill, junction, left, right
};

const Box& boxFor(TableStyle s) {
    static const Box kDefault{"+", "-", "+", "+", "+", "-", "+", "+", "+", "-", "+", "+", "|",
                              "-", "+", "+", "+"};
    static const Box kAscii{"+", "-", "+", "+", "+", "-", "+", "+", "+", "-", "+", "+", "|",
                            "=", "+", "+", "+"};
    static const Box kLight{"┌", "─", "┬", "┐", "├", "─", "┼", "┤", "└", "─", "┴", "┘", "│",
                            "─", "┼", "├", "┤"};
    static const Box kRounded{"╭", "─", "┬", "╮", "├", "─", "┼", "┤", "╰", "─", "┴", "╯", "│",
                              "─", "┼", "├", "┤"};
    static const Box kBold{"┏", "━", "┳", "┓", "┣", "━", "╋", "┫", "┗", "━", "┻", "┛", "┃",
                           "━", "╋", "┣", "┫"};
    static const Box kDouble{"╔", "═", "╦", "╗", "╠", "═", "╬", "╣", "╚", "═", "╩", "╝", "║",
                             "═", "╬", "╠", "╣"};
    switch (s) {
        case TableStyle::Default: return kDefault;
        case TableStyle::Ascii: return kAscii;
        case TableStyle::Rounded: return kRounded;
        case TableStyle::Bold: return kBold;
        case TableStyle::Double: return kDouble;
        default: return kLight;
    }
}

std::string lower(std::string s) {
    for (char& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r"), b = s.find_last_not_of(" \t\r");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}

std::string repeat(const char* s, int n) {
    std::string o;
    for (int i = 0; i < n; ++i) o += s;
    return o;
}

double numericValue(const std::string& s, bool& ok) {
    std::string t;
    for (char c : stripAnsi(s))
        if (c != ',' && c != '$' && c != ' ' && c != '%') t += c;
    char* end = nullptr;
    double v = std::strtod(t.c_str(), &end);
    ok = !t.empty() && end && end != t.c_str();
    return v;
}

void sortRows(std::vector<Row>& rows, int col, SortMode mode) {
    if (mode == SortMode::None || col < 0) return;
    auto cell = [col](const Row& r) { return col < int(r.size()) ? r[col] : std::string(); };
    std::stable_sort(rows.begin(), rows.end(), [&](const Row& a, const Row& b) {
        if (mode == SortMode::AscNumeric || mode == SortMode::DscNumeric) {
            bool oa, ob;
            double va = numericValue(cell(a), oa), vb = numericValue(cell(b), ob);
            if (oa != ob) return oa;  // numbers before text either way
            if (oa && va != vb) return mode == SortMode::AscNumeric ? va < vb : va > vb;
            if (oa) return false;
        }
        bool asc = mode == SortMode::Asc || mode == SortMode::AscNumeric;
        return asc ? cell(a) < cell(b) : cell(b) < cell(a);
    });
}

std::string htmlEscape(const std::string& s) {
    std::string o;
    for (char c : s) {
        switch (c) {
            case '&': o += "&amp;"; break;
            case '<': o += "&lt;"; break;
            case '>': o += "&gt;"; break;
            case '"': o += "&quot;"; break;
            default: o += c;
        }
    }
    return o;
}

std::string csvCell(const std::string& raw, char delim) {
    std::string s = stripAnsi(raw);
    // Formula-injection guard: a leading = + - @ in non-numbers is neutralised with a quote.
    if (!s.empty() && (s[0] == '=' || s[0] == '+' || s[0] == '-' || s[0] == '@') && !looksNumeric(s))
        s = "'" + s;
    bool quote = s.find(delim) != std::string::npos || s.find('"') != std::string::npos ||
                 s.find('\n') != std::string::npos;
    if (delim == '\t') {  // TSV has no quoting: replace tabs and newlines.
        for (char& c : s)
            if (c == '\t' || c == '\n') c = ' ';
        return s;
    }
    if (!quote) return s;
    std::string o = "\"";
    for (char c : s) {
        if (c == '"') o += '"';
        o += c;
    }
    return o + "\"";
}

}  // namespace

std::vector<std::string> tableStyleNames() {
    return {"default", "light", "rounded", "bold", "double", "ascii", "none"};
}

std::optional<TableStyle> parseTableStyle(const std::string& raw) {
    std::string s = lower(raw);
    if (s == "default") return TableStyle::Default;
    if (s == "light" || s == "unicode") return TableStyle::Light;
    if (s == "rounded" || s == "round") return TableStyle::Rounded;
    if (s == "bold" || s == "heavy") return TableStyle::Bold;
    if (s == "double") return TableStyle::Double;
    if (s == "ascii") return TableStyle::Ascii;
    if (s == "none" || s == "plain") return TableStyle::None;
    return std::nullopt;
}

std::optional<TableFormat> parseTableFormat(const std::string& raw) {
    std::string s = lower(raw);
    if (s == "text" || s == "txt") return TableFormat::Text;
    if (s == "markdown" || s == "md") return TableFormat::Markdown;
    if (s == "csv") return TableFormat::Csv;
    if (s == "tsv") return TableFormat::Tsv;
    if (s == "html") return TableFormat::Html;
    return std::nullopt;
}

std::optional<SortMode> parseSortMode(const std::string& raw) {
    std::string s = lower(raw);
    if (s == "none") return SortMode::None;
    if (s == "asc") return SortMode::Asc;
    if (s == "dsc" || s == "desc") return SortMode::Dsc;
    if (s == "asc-numeric" || s == "ascnumeric" || s == "num") return SortMode::AscNumeric;
    if (s == "dsc-numeric" || s == "desc-numeric" || s == "dscnumeric") return SortMode::DscNumeric;
    return std::nullopt;
}

std::vector<Row> parseDelimited(const std::string& text, char delimiter) {
    std::vector<std::string> ls = lines(text);
    while (!ls.empty() && trim(ls.back()).empty()) ls.pop_back();
    while (!ls.empty() && trim(ls.front()).empty()) ls.erase(ls.begin());
    std::vector<Row> rows;
    if (ls.empty()) return rows;

    // Markdown table: every non-empty line starts with '|'.
    bool markdown = delimiter == 0 && std::all_of(ls.begin(), ls.end(), [](const std::string& l) {
        std::string t = trim(l);
        return t.empty() || t[0] == '|';
    });
    if (markdown) {
        for (const std::string& l : ls) {
            std::string t = trim(l);
            if (t.empty()) continue;
            if (t.front() == '|') t.erase(0, 1);
            if (!t.empty() && t.back() == '|') t.pop_back();
            // Separator row like |---|:--:|
            if (!t.empty() && t.find_first_not_of("-:| ") == std::string::npos) continue;
            Row r;
            std::string cur;
            for (size_t i = 0; i < t.size(); ++i) {
                if (t[i] == '\\' && i + 1 < t.size() && t[i + 1] == '|') { cur += '|'; ++i; }
                else if (t[i] == '|') { r.push_back(trim(cur)); cur.clear(); }
                else cur += t[i];
            }
            r.push_back(trim(cur));
            rows.push_back(r);
        }
        return rows;
    }

    if (delimiter == 0) {
        // Pick the candidate that appears most consistently on the first lines.
        const char cands[] = {'\t', ',', ';', '|'};
        int best = -1;
        for (char c : cands) {
            size_t n = std::min<size_t>(ls.size(), 10);
            int minCount = 1 << 30;
            for (size_t i = 0; i < n; ++i)
                minCount = std::min(minCount, int(std::count(ls[i].begin(), ls[i].end(), c)));
            if (minCount > best && minCount > 0) { best = minCount; delimiter = c; }
        }
        if (delimiter == 0) delimiter = ',';
    }

    // RFC 4180 style parse over the whole text so quoted fields may hold newlines.
    std::string joined;
    for (size_t i = 0; i < ls.size(); ++i) joined += ls[i] + "\n";
    Row row;
    std::string cell;
    bool inQuotes = false, wasQuoted = false;
    for (size_t i = 0; i < joined.size(); ++i) {
        char c = joined[i];
        if (inQuotes) {
            if (c == '"' && i + 1 < joined.size() && joined[i + 1] == '"') { cell += '"'; ++i; }
            else if (c == '"') inQuotes = false;
            else cell += c;
        } else if (c == '"' && trim(cell).empty()) {
            inQuotes = true;
            wasQuoted = true;
            cell.clear();
        } else if (c == delimiter) {
            row.push_back(wasQuoted ? cell : trim(cell));
            cell.clear();
            wasQuoted = false;
        } else if (c == '\n') {
            row.push_back(wasQuoted ? cell : trim(cell));
            cell.clear();
            wasQuoted = false;
            if (!(row.size() == 1 && row[0].empty())) rows.push_back(row);
            row.clear();
        } else {
            cell += c;
        }
    }
    return rows;
}

std::string renderTable(const std::vector<Row>& input, const TableOptions& opt) {
    if (input.empty()) return "";
    std::vector<Row> rows = input;
    size_t ncols = 0;
    for (auto& r : rows) ncols = std::max(ncols, r.size());
    for (auto& r : rows) r.resize(ncols);

    // Split header / body / footer.
    Row header, footer;
    bool hasHeader = opt.header && !rows.empty();
    if (hasHeader) { header = rows.front(); rows.erase(rows.begin()); }
    bool hasFooter = opt.footer && !rows.empty();
    if (hasFooter) { footer = rows.back(); rows.pop_back(); }
    sortRows(rows, opt.sortColumn, opt.sortMode);

    std::vector<ColumnConfig> cfg = opt.columns;
    cfg.resize(ncols);
    if (opt.autoIndex) {
        for (size_t i = 0; i < rows.size(); ++i) rows[i].insert(rows[i].begin(), std::to_string(i + 1));
        if (hasHeader) header.insert(header.begin(), "#");
        if (hasFooter) footer.insert(footer.begin(), "");
        cfg.insert(cfg.begin(), ColumnConfig{Align::Right, 0, false});
        ++ncols;
    }
    // Drop hidden columns.
    std::vector<size_t> keep;
    for (size_t c = 0; c < ncols; ++c)
        if (!cfg[c].hidden) keep.push_back(c);
    auto project = [&](const Row& r) {
        Row o;
        for (size_t c : keep) o.push_back(r[c]);
        return o;
    };
    for (auto& r : rows) r = project(r);
    if (hasHeader) header = project(header);
    if (hasFooter) footer = project(footer);
    std::vector<ColumnConfig> cols;
    for (size_t c : keep) cols.push_back(cfg[c]);
    ncols = keep.size();
    if (ncols == 0) return "";

    // Column alignment: Auto resolves per column (numbers right when every body cell is numeric).
    std::vector<Align> align(ncols, Align::Left);
    for (size_t c = 0; c < ncols; ++c) {
        if (cols[c].align != Align::Auto) { align[c] = cols[c].align; continue; }
        bool allNum = !rows.empty();
        for (auto& r : rows)
            if (!r[c].empty() && !looksNumeric(r[c])) { allNum = false; break; }
        align[c] = allNum ? Align::Right : Align::Left;
    }

    // ------------------------------------------------ Non-text formats
    if (opt.format == TableFormat::Csv || opt.format == TableFormat::Tsv) {
        char d = opt.format == TableFormat::Csv ? ',' : '\t';
        std::string out;
        auto emit = [&](const Row& r) {
            for (size_t c = 0; c < r.size(); ++c) out += (c ? std::string(1, d) : "") + csvCell(r[c], d);
            out += "\n";
        };
        if (hasHeader) emit(header);
        for (auto& r : rows) emit(r);
        if (hasFooter) emit(footer);
        return out;
    }
    if (opt.format == TableFormat::Markdown) {
        auto esc = [](const std::string& s) {
            std::string o;
            for (char c : stripAnsi(s)) {
                if (c == '|') o += "\\|";
                else if (c == '\n') o += "<br>";
                else o += c;
            }
            return o;
        };
        std::vector<int> w(ncols, 3);
        Row hdr = hasHeader ? header : Row(ncols, "");
        auto measure = [&](const Row& r) {
            for (size_t c = 0; c < ncols; ++c) w[c] = std::max(w[c], displayWidth(esc(r[c])));
        };
        measure(hdr);
        for (auto& r : rows) measure(r);
        if (hasFooter) measure(footer);
        std::string out;
        if (!opt.title.empty()) out += "# " + opt.title + "\n\n";
        auto emit = [&](const Row& r) {
            out += "|";
            for (size_t c = 0; c < ncols; ++c) out += " " + pad(esc(r[c]), w[c], align[c] == Align::Auto ? Align::Left : align[c]) + " |";
            out += "\n";
        };
        emit(hdr);
        out += "|";
        for (size_t c = 0; c < ncols; ++c) {
            std::string dash(w[c], '-');
            if (align[c] == Align::Right) dash.back() = ':';
            else if (align[c] == Align::Center) { dash.front() = ':'; dash.back() = ':'; }
            out += " " + dash + " |";
        }
        out += "\n";
        for (auto& r : rows) emit(r);
        if (hasFooter) emit(footer);
        if (!opt.caption.empty()) out += "\n_" + opt.caption + "_\n";
        return out;
    }
    if (opt.format == TableFormat::Html) {
        auto attr = [&](size_t c) -> std::string {
            if (align[c] == Align::Right) return " align=\"right\"";
            if (align[c] == Align::Center) return " align=\"center\"";
            return "";
        };
        auto cellHtml = [](const std::string& s) {
            std::string e = htmlEscape(stripAnsi(s)), o;
            for (char c : e) o += c == '\n' ? std::string("<br/>") : std::string(1, c);
            return o;
        };
        std::string out = "<table class=\"misty-table\">\n";
        if (!opt.title.empty()) out += "  <caption>" + htmlEscape(opt.title) + "</caption>\n";
        if (hasHeader) {
            out += "  <thead>\n  <tr>\n";
            for (size_t c = 0; c < ncols; ++c) out += "    <th" + attr(c) + ">" + cellHtml(header[c]) + "</th>\n";
            out += "  </tr>\n  </thead>\n";
        }
        out += "  <tbody>\n";
        for (auto& r : rows) {
            out += "  <tr>\n";
            for (size_t c = 0; c < ncols; ++c) out += "    <td" + attr(c) + ">" + cellHtml(r[c]) + "</td>\n";
            out += "  </tr>\n";
        }
        out += "  </tbody>\n";
        if (hasFooter) {
            out += "  <tfoot>\n  <tr>\n";
            for (size_t c = 0; c < ncols; ++c) out += "    <td" + attr(c) + ">" + cellHtml(footer[c]) + "</td>\n";
            out += "  </tr>\n  </tfoot>\n";
        }
        out += "</table>\n";
        if (!opt.caption.empty()) out += "<p class=\"misty-caption\">" + htmlEscape(opt.caption) + "</p>\n";
        return out;
    }

    // ------------------------------------------------ Text
    // Each cell becomes a list of lines (embedded newlines and widthMax wrapping).
    auto cellLines = [&](const std::string& s, size_t c) {
        std::vector<std::string> out;
        for (const std::string& l : lines(s)) {
            if (cols[c].widthMax > 0 && displayWidth(l) > cols[c].widthMax) {
                for (auto& w : wrap(stripAnsi(l), cols[c].widthMax)) out.push_back(w);
            } else {
                out.push_back(l);
            }
        }
        return out;
    };
    using Cells = std::vector<std::vector<std::string>>;
    auto toCells = [&](const Row& r) {
        Cells cs;
        for (size_t c = 0; c < ncols; ++c) cs.push_back(cellLines(r[c], c));
        return cs;
    };
    std::vector<Cells> body;
    for (auto& r : rows) body.push_back(toCells(r));
    Cells hcells = hasHeader ? toCells(header) : Cells{};
    Cells fcells = hasFooter ? toCells(footer) : Cells{};

    std::vector<int> w(ncols, 0);
    auto measure = [&](const Cells& cs) {
        for (size_t c = 0; c < ncols; ++c)
            for (auto& l : cs[c]) w[c] = std::max(w[c], displayWidth(l));
    };
    if (hasHeader) measure(hcells);
    for (auto& cs : body) measure(cs);
    if (hasFooter) measure(fcells);

    const bool plain = opt.style == TableStyle::None;
    const Box& b = boxFor(opt.style);
    const bool border = opt.border && !plain;
    const std::string colSep = plain ? "  " : std::string(" ") + b.v + " ";

    int inner = 0;  // width between outer borders
    for (size_t c = 0; c < ncols; ++c) inner += w[c] + 2;
    inner += int(ncols) - 1;

    // Title wider than the columns: grow the last column so the box fits it.
    if (!opt.title.empty() && !plain) {
        int need = displayWidth(opt.title) + 2;
        if (need > inner) { w[ncols - 1] += need - inner; inner = need; }
    }

    std::vector<std::string> out;
    auto rule = [&](const char* l, const char* h, const char* j, const char* r, bool junctions) {
        if (plain) return;
        std::string s = border ? l : "";
        for (size_t c = 0; c < ncols; ++c) {
            bool edge = !border && (c == 0 || c + 1 == ncols);
            int extra = edge ? 1 : 2;  // without outer border the first/last cell loses a pad
            if (ncols == 1 && !border) extra = 0;
            s += repeat(h, w[c] + extra);
            if (c + 1 < ncols) s += junctions ? j : h;
        }
        if (border) s += r;
        out.push_back(s);
    };
    auto emitRow = [&](const Cells& cs, bool isHeader) {
        size_t height = 1;
        for (auto& c : cs) height = std::max(height, c.size());
        for (size_t li = 0; li < height; ++li) {
            std::string s = border ? std::string(b.v) + " " : "";
            for (size_t c = 0; c < ncols; ++c) {
                std::string text = li < cs[c].size() ? cs[c][li] : "";
                Align a = isHeader ? (align[c] == Align::Right ? Align::Right : Align::Left) : align[c];
                if (isHeader && align[c] == Align::Center) a = Align::Center;
                s += pad(text, w[c], a);
                if (c + 1 < ncols) s += colSep;
            }
            if (border) s += std::string(" ") + b.v;
            // Without a border, trailing spaces of the last column are noise.
            if (!border) s.erase(s.find_last_not_of(' ') + 1);
            out.push_back(s);
        }
    };

    if (!opt.title.empty()) {
        if (plain) {
            out.push_back(opt.title);
        } else {
            if (border) out.push_back(std::string(b.tl) + repeat(b.th, inner) + b.tr);
            std::string t = pad(opt.title, inner - 2, Align::Center);
            out.push_back(border ? std::string(b.v) + " " + t + " " + b.v : t);
            rule(b.ml, b.mh, b.tj, b.mr, true);
        }
    } else if (border) {
        rule(b.tl, b.th, b.tj, b.tr, true);
    }
    if (hasHeader) {
        emitRow(hcells, true);
        if (plain) {
            std::string s;
            for (size_t c = 0; c < ncols; ++c) s += (c ? "  " : "") + std::string(w[c], '-');
            out.push_back(s);
        } else {
            rule(b.hl, b.hh, b.hj, b.hr, true);
        }
    }
    for (size_t i = 0; i < body.size(); ++i) {
        emitRow(body[i], false);
        if (opt.separateRows && i + 1 < body.size()) rule(b.ml, b.mh, b.mj, b.mr, true);
    }
    if (hasFooter) {
        if (!plain) rule(b.ml, b.mh, b.mj, b.mr, true);
        emitRow(fcells, false);
    }
    if (border) rule(b.bl, b.bh, b.bj, b.br, true);
    if (!opt.caption.empty()) out.push_back(opt.caption);

    std::string s;
    for (auto& l : out) s += l + "\n";
    return s;
}

}  // namespace misty::ascii
