// SPDX-License-Identifier: MIT
// Colour parsing, terminal escape output and HSV gradients.
#include "misty/ascii.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace misty::ascii {
namespace {

struct Named {
    const char* name;
    unsigned rgb;
};

// The 140 CSS / X11 colour names (CSS Color Module Level 4), plus "grey" spellings.
constexpr Named kNames[] = {
    {"aliceblue", 0xF0F8FF}, {"antiquewhite", 0xFAEBD7}, {"aqua", 0x00FFFF},
    {"aquamarine", 0x7FFFD4}, {"azure", 0xF0FFFF}, {"beige", 0xF5F5DC}, {"bisque", 0xFFE4C4},
    {"black", 0x000000}, {"blanchedalmond", 0xFFEBCD}, {"blue", 0x0000FF},
    {"blueviolet", 0x8A2BE2}, {"brown", 0xA52A2A}, {"burlywood", 0xDEB887},
    {"cadetblue", 0x5F9EA0}, {"chartreuse", 0x7FFF00}, {"chocolate", 0xD2691E},
    {"coral", 0xFF7F50}, {"cornflowerblue", 0x6495ED}, {"cornsilk", 0xFFF8DC},
    {"crimson", 0xDC143C}, {"cyan", 0x00FFFF}, {"darkblue", 0x00008B}, {"darkcyan", 0x008B8B},
    {"darkgoldenrod", 0xB8860B}, {"darkgray", 0xA9A9A9}, {"darkgreen", 0x006400},
    {"darkgrey", 0xA9A9A9}, {"darkkhaki", 0xBDB76B}, {"darkmagenta", 0x8B008B},
    {"darkolivegreen", 0x556B2F}, {"darkorange", 0xFF8C00}, {"darkorchid", 0x9932CC},
    {"darkred", 0x8B0000}, {"darksalmon", 0xE9967A}, {"darkseagreen", 0x8FBC8F},
    {"darkslateblue", 0x483D8B}, {"darkslategray", 0x2F4F4F}, {"darkslategrey", 0x2F4F4F},
    {"darkturquoise", 0x00CED1}, {"darkviolet", 0x9400D3}, {"deeppink", 0xFF1493},
    {"deepskyblue", 0x00BFFF}, {"dimgray", 0x696969}, {"dimgrey", 0x696969},
    {"dodgerblue", 0x1E90FF}, {"firebrick", 0xB22222}, {"floralwhite", 0xFFFAF0},
    {"forestgreen", 0x228B22}, {"fuchsia", 0xFF00FF}, {"gainsboro", 0xDCDCDC},
    {"ghostwhite", 0xF8F8FF}, {"gold", 0xFFD700}, {"goldenrod", 0xDAA520}, {"gray", 0x808080},
    {"green", 0x008000}, {"greenyellow", 0xADFF2F}, {"grey", 0x808080}, {"honeydew", 0xF0FFF0},
    {"hotpink", 0xFF69B4}, {"indianred", 0xCD5C5C}, {"indigo", 0x4B0082}, {"ivory", 0xFFFFF0},
    {"khaki", 0xF0E68C}, {"lavender", 0xE6E6FA}, {"lavenderblush", 0xFFF0F5},
    {"lawngreen", 0x7CFC00}, {"lemonchiffon", 0xFFFACD}, {"lightblue", 0xADD8E6},
    {"lightcoral", 0xF08080}, {"lightcyan", 0xE0FFFF}, {"lightgoldenrodyellow", 0xFAFAD2},
    {"lightgray", 0xD3D3D3}, {"lightgreen", 0x90EE90}, {"lightgrey", 0xD3D3D3},
    {"lightpink", 0xFFB6C1}, {"lightsalmon", 0xFFA07A}, {"lightseagreen", 0x20B2AA},
    {"lightskyblue", 0x87CEFA}, {"lightslategray", 0x778899}, {"lightslategrey", 0x778899},
    {"lightsteelblue", 0xB0C4DE}, {"lightyellow", 0xFFFFE0}, {"lime", 0x00FF00},
    {"limegreen", 0x32CD32}, {"linen", 0xFAF0E6}, {"magenta", 0xFF00FF}, {"maroon", 0x800000},
    {"mediumaquamarine", 0x66CDAA}, {"mediumblue", 0x0000CD}, {"mediumorchid", 0xBA55D3},
    {"mediumpurple", 0x9370DB}, {"mediumseagreen", 0x3CB371}, {"mediumslateblue", 0x7B68EE},
    {"mediumspringgreen", 0x00FA9A}, {"mediumturquoise", 0x48D1CC},
    {"mediumvioletred", 0xC71585}, {"midnightblue", 0x191970}, {"mintcream", 0xF5FFFA},
    {"mistyrose", 0xFFE4E1}, {"moccasin", 0xFFE4B5}, {"navajowhite", 0xFFDEAD},
    {"navy", 0x000080}, {"oldlace", 0xFDF5E6}, {"olive", 0x808000}, {"olivedrab", 0x6B8E23},
    {"orange", 0xFFA500}, {"orangered", 0xFF4500}, {"orchid", 0xDA70D6},
    {"palegoldenrod", 0xEEE8AA}, {"palegreen", 0x98FB98}, {"paleturquoise", 0xAFEEEE},
    {"palevioletred", 0xDB7093}, {"papayawhip", 0xFFEFD5}, {"peachpuff", 0xFFDAB9},
    {"peru", 0xCD853F}, {"pink", 0xFFC0CB}, {"plum", 0xDDA0DD}, {"powderblue", 0xB0E0E6},
    {"purple", 0x800080}, {"rebeccapurple", 0x663399}, {"red", 0xFF0000},
    {"rosybrown", 0xBC8F8F}, {"royalblue", 0x4169E1}, {"saddlebrown", 0x8B4513},
    {"salmon", 0xFA8072}, {"sandybrown", 0xF4A460}, {"seagreen", 0x2E8B57},
    {"seashell", 0xFFF5EE}, {"sienna", 0xA0522D}, {"silver", 0xC0C0C0}, {"skyblue", 0x87CEEB},
    {"slateblue", 0x6A5ACD}, {"slategray", 0x708090}, {"slategrey", 0x708090},
    {"snow", 0xFFFAFA}, {"springgreen", 0x00FF7F}, {"steelblue", 0x4682B4}, {"tan", 0xD2B48C},
    {"teal", 0x008080}, {"thistle", 0xD8BFD8}, {"tomato", 0xFF6347}, {"turquoise", 0x40E0D0},
    {"violet", 0xEE82EE}, {"wheat", 0xF5DEB3}, {"white", 0xFFFFFF}, {"whitesmoke", 0xF5F5F5},
    {"yellow", 0xFFFF00}, {"yellowgreen", 0x9ACD32},
};

// xterm default values for the 16 basic colours, in SGR order (30..37 then 90..97).
constexpr unsigned kBasic16[16] = {
    0x000000, 0xCD0000, 0x00CD00, 0xCDCD00, 0x0000EE, 0xCD00CD, 0x00CDCD, 0xE5E5E5,
    0x7F7F7F, 0xFF0000, 0x00FF00, 0xFFFF00, 0x5C5CFF, 0xFF00FF, 0x00FFFF, 0xFFFFFF,
};

Rgb fromHex(unsigned v) { return {int((v >> 16) & 0xFF), int((v >> 8) & 0xFF), int(v & 0xFF)}; }

int dist2(Rgb a, Rgb b) {
    int dr = a.r - b.r, dg = a.g - b.g, db = a.b - b.b;
    return dr * dr * 3 + dg * dg * 4 + db * db * 2;  // rough perceptual weights
}

std::string lower(std::string s) {
    for (char& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

struct Hsv {
    double h, s, v;
};

Hsv toHsv(Rgb c) {
    double r = c.r / 255.0, g = c.g / 255.0, b = c.b / 255.0;
    double mx = std::max({r, g, b}), mn = std::min({r, g, b}), d = mx - mn;
    double h = 0;
    if (d > 0) {
        if (mx == r) h = 60 * std::fmod((g - b) / d, 6.0);
        else if (mx == g) h = 60 * ((b - r) / d + 2);
        else h = 60 * ((r - g) / d + 4);
    }
    if (h < 0) h += 360;
    return {h, mx == 0 ? 0 : d / mx, mx};
}

Rgb fromHsv(Hsv x) {
    double c = x.v * x.s, hp = std::fmod(x.h, 360.0) / 60.0;
    double xx = c * (1 - std::fabs(std::fmod(hp, 2.0) - 1));
    double r = 0, g = 0, b = 0;
    if (hp < 1) { r = c; g = xx; }
    else if (hp < 2) { r = xx; g = c; }
    else if (hp < 3) { g = c; b = xx; }
    else if (hp < 4) { g = xx; b = c; }
    else if (hp < 5) { r = xx; b = c; }
    else { r = c; b = xx; }
    double m = x.v - c;
    auto ch = [&](double v) { return int(std::lround((v + m) * 255)); };
    return {ch(r), ch(g), ch(b)};
}

// Parses "a,b,c" inside "fn(...)". Returns false on any error.
bool parseTriple(const std::string& inner, double out[3]) {
    std::string s = inner;
    for (char& c : s)
        if (c == ',' || c == '%') c = ' ';
    const char* p = s.c_str();
    for (int i = 0; i < 3; ++i) {
        char* end;
        out[i] = std::strtod(p, &end);
        if (end == p) return false;
        p = end;
    }
    while (*p == ' ') ++p;
    return *p == 0;
}

}  // namespace

std::optional<Rgb> parseColor(const std::string& raw) {
    std::string s = lower(raw);
    s.erase(std::remove_if(s.begin(), s.end(), [](char c) { return c == ' ' || c == '_' || c == '-'; }),
            s.end());
    if (s.empty()) return std::nullopt;
    if (s[0] == '#') {
        std::string h = s.substr(1);
        if (!std::all_of(h.begin(), h.end(), [](char c) { return std::isxdigit(static_cast<unsigned char>(c)); }))
            return std::nullopt;
        if (h.size() == 3) h = {h[0], h[0], h[1], h[1], h[2], h[2]};
        if (h.size() != 6) return std::nullopt;
        return fromHex(unsigned(std::strtoul(h.c_str(), nullptr, 16)));
    }
    auto fn = [&](const char* name) -> std::optional<std::string> {
        size_t n = std::strlen(name);
        if (s.size() > n + 1 && s.compare(0, n, name) == 0 && s[n] == '(' && s.back() == ')')
            return s.substr(n + 1, s.size() - n - 2);
        return std::nullopt;
    };
    double v[3];
    if (auto in = fn("rgb")) {
        if (!parseTriple(*in, v)) return std::nullopt;
        for (double x : v)
            if (x < 0 || x > 255) return std::nullopt;
        return Rgb{int(v[0]), int(v[1]), int(v[2])};
    }
    if (auto in = fn("hsv")) {
        if (!parseTriple(*in, v)) return std::nullopt;
        if (v[1] > 1 || v[2] > 1) { v[1] /= 100; v[2] /= 100; }  // accept 0..100 too
        if (v[1] < 0 || v[1] > 1 || v[2] < 0 || v[2] > 1) return std::nullopt;
        return fromHsv({std::fmod(v[0], 360.0) + (v[0] < 0 ? 360 : 0), v[1], v[2]});
    }
    for (const Named& n : kNames)
        if (s == n.name) return fromHex(n.rgb);
    return std::nullopt;
}

std::vector<std::string> colorNames() {
    std::vector<std::string> out;
    for (const Named& n : kNames) out.emplace_back(n.name);
    std::sort(out.begin(), out.end());
    return out;
}

std::optional<ColorDepth> parseColorDepth(const std::string& raw) {
    std::string s = lower(raw);
    if (s == "never" || s == "none" || s == "0" || s == "off") return ColorDepth::None;
    if (s == "16" || s == "basic" || s == "basic16") return ColorDepth::Basic16;
    if (s == "256" || s == "ansi256") return ColorDepth::Ansi256;
    if (s == "truecolor" || s == "24bit" || s == "16m" || s == "always") return ColorDepth::TrueColor;
    return std::nullopt;
}

std::string colorDepthName(ColorDepth d) {
    switch (d) {
        case ColorDepth::None: return "none";
        case ColorDepth::Basic16: return "16";
        case ColorDepth::Ansi256: return "256";
        case ColorDepth::TrueColor: return "truecolor";
    }
    return "none";
}

int toBasic16(Rgb c) {
    int best = 0, bd = dist2(c, fromHex(kBasic16[0]));
    for (int i = 1; i < 16; ++i) {
        int d = dist2(c, fromHex(kBasic16[i]));
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

int toAnsi256(Rgb c) {
    // Candidate in the 6x6x6 cube.
    auto level = [](int v) { return v < 48 ? 0 : v < 115 ? 1 : (v - 35) / 40; };
    static const int steps[6] = {0, 95, 135, 175, 215, 255};
    int ri = level(c.r), gi = level(c.g), bi = level(c.b);
    Rgb cube{steps[ri], steps[gi], steps[bi]};
    int cubeIdx = 16 + 36 * ri + 6 * gi + bi;
    // Candidate on the grey ramp (232..255 = 8, 18, ..., 238).
    int avg = (c.r + c.g + c.b) / 3;
    int gi2 = avg > 238 ? 23 : avg < 8 ? 0 : (avg - 8) / 10;
    int gv = 8 + gi2 * 10;
    Rgb grey{gv, gv, gv};
    return dist2(c, grey) < dist2(c, cube) ? 232 + gi2 : cubeIdx;
}

std::string sgr(Rgb c, ColorDepth d, bool bg) {
    switch (d) {
        case ColorDepth::None: return "";
        case ColorDepth::Basic16: {
            int i = toBasic16(c);
            int code = (i < 8 ? 30 + i : 90 + i - 8) + (bg ? 10 : 0);
            return "\x1b[" + std::to_string(code) + "m";
        }
        case ColorDepth::Ansi256:
            return std::string("\x1b[") + (bg ? "48" : "38") + ";5;" + std::to_string(toAnsi256(c)) + "m";
        case ColorDepth::TrueColor:
            return std::string("\x1b[") + (bg ? "48" : "38") + ";2;" + std::to_string(c.r) + ";" +
                   std::to_string(c.g) + ";" + std::to_string(c.b) + "m";
    }
    return "";
}

std::string colorize(const std::string& text, Rgb fg, ColorDepth d, std::optional<Rgb> bg) {
    if (d == ColorDepth::None || text.empty()) return text;
    std::string out = sgr(fg, d);
    if (bg) out += sgr(*bg, d, true);
    // Keep colour across lines: reset before each newline and re-open after it.
    std::string open = out;
    for (char ch : text) {
        if (ch == '\n') out += std::string(kReset) + "\n" + open;
        else out += ch;
    }
    return out + kReset;
}

Rgb gradientAt(const std::vector<Rgb>& stops, double t) {
    if (stops.empty()) return {255, 255, 255};
    if (stops.size() == 1) return stops[0];
    t = std::clamp(t, 0.0, 1.0);
    double pos = t * double(stops.size() - 1);
    size_t i = std::min(size_t(pos), stops.size() - 2);
    double f = pos - double(i);
    Hsv a = toHsv(stops[i]), b = toHsv(stops[i + 1]);
    // Greys have no hue: borrow the other end's hue so the blend does not swing.
    if (a.s == 0) a.h = b.h;
    if (b.s == 0) b.h = a.h;
    double dh = b.h - a.h;
    if (dh > 180) dh -= 360;
    if (dh < -180) dh += 360;
    double h = a.h + dh * f;
    if (h < 0) h += 360;
    return fromHsv({h, a.s + (b.s - a.s) * f, a.v + (b.v - a.v) * f});
}

std::string gradient(const std::string& text, const std::vector<Rgb>& stops, ColorDepth d) {
    if (d == ColorDepth::None || stops.empty()) return text;
    // Gradient runs across the widest line; every line uses the same column positions.
    std::vector<std::string> ls = lines(stripAnsi(text));
    int widest = 0;
    for (auto& l : ls) widest = std::max(widest, displayWidth(l));
    std::string out;
    for (size_t li = 0; li < ls.size(); ++li) {
        if (li) out += "\n";
        int col = 0;
        std::u32string cps = decodeUtf8(ls[li]);
        std::string last;
        for (char32_t cp : cps) {
            std::string ch = encodeUtf8(cp);
            if (cp == U' ') { out += ch; ++col; continue; }
            double t = widest > 1 ? double(col) / double(widest - 1) : 0;
            std::string code = sgr(gradientAt(stops, t), d);
            if (code != last) { out += code; last = code; }
            out += ch;
            col += codepointWidth(cp);
        }
        if (!last.empty()) out += kReset;
    }
    return out;
}

ColorDepth detectColorDepth(bool isTty) {
    const char* noColor = std::getenv("NO_COLOR");
    if (noColor && *noColor) return ColorDepth::None;
    if (const char* force = std::getenv("FORCE_COLOR")) {
        std::string f = force;
        if (f == "0" || f == "false") return ColorDepth::None;
        if (f == "2") return ColorDepth::Ansi256;
        if (f == "3") return ColorDepth::TrueColor;
        return ColorDepth::Basic16;  // "", "1", "true"
    }
    if (!isTty) return ColorDepth::None;
    std::string ct = lower(std::getenv("COLORTERM") ? std::getenv("COLORTERM") : "");
    if (ct == "truecolor" || ct == "24bit") return ColorDepth::TrueColor;
    std::string term = lower(std::getenv("TERM") ? std::getenv("TERM") : "");
    if (term == "dumb") return ColorDepth::None;
    if (term.find("256color") != std::string::npos) return ColorDepth::Ansi256;
    if (std::getenv("WT_SESSION")) return ColorDepth::TrueColor;  // Windows Terminal
    return ColorDepth::Basic16;
}

}  // namespace misty::ascii
