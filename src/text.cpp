// SPDX-License-Identifier: MIT
// Text measuring and shaping. Width tables follow Unicode East Asian Width (W/F) and the
// general categories Mn/Me/Cf, as in Markus Kuhn's public domain wcwidth.
#include "misty/ascii.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace misty::ascii {
namespace {

struct Range {
    char32_t lo, hi;
};

// Zero width: combining marks, format characters, variation selectors, ZWJ.
constexpr Range kZero[] = {
    {0x0300, 0x036F}, {0x0483, 0x0489}, {0x0591, 0x05BD}, {0x05BF, 0x05BF}, {0x05C1, 0x05C2},
    {0x05C4, 0x05C5}, {0x05C7, 0x05C7}, {0x0610, 0x061A}, {0x061C, 0x061C}, {0x064B, 0x065F},
    {0x0670, 0x0670}, {0x06D6, 0x06DC}, {0x06DF, 0x06E4}, {0x06E7, 0x06E8}, {0x06EA, 0x06ED},
    {0x0711, 0x0711}, {0x0730, 0x074A}, {0x07A6, 0x07B0}, {0x07EB, 0x07F3}, {0x0816, 0x082D},
    {0x0859, 0x085B}, {0x08D3, 0x08FF}, {0x0900, 0x0902}, {0x093A, 0x093A}, {0x093C, 0x093C},
    {0x0941, 0x0948}, {0x094D, 0x094D}, {0x0951, 0x0957}, {0x0962, 0x0963}, {0x0981, 0x0981},
    {0x09BC, 0x09BC}, {0x09C1, 0x09C4}, {0x09CD, 0x09CD}, {0x0A01, 0x0A02}, {0x0A3C, 0x0A3C},
    {0x0A41, 0x0A51}, {0x0A70, 0x0A71}, {0x0A75, 0x0A75}, {0x0A81, 0x0A82}, {0x0ABC, 0x0ABC},
    {0x0AC1, 0x0AC8}, {0x0ACD, 0x0ACD}, {0x0B01, 0x0B01}, {0x0B3C, 0x0B3C}, {0x0B3F, 0x0B3F},
    {0x0B41, 0x0B44}, {0x0B4D, 0x0B4D}, {0x0BC0, 0x0BC0}, {0x0BCD, 0x0BCD}, {0x0C3E, 0x0C40},
    {0x0C46, 0x0C56}, {0x0CBC, 0x0CBC}, {0x0CCC, 0x0CCD}, {0x0D41, 0x0D44}, {0x0D4D, 0x0D4D},
    {0x0DCA, 0x0DCA}, {0x0DD2, 0x0DD6}, {0x0E31, 0x0E31}, {0x0E34, 0x0E3A}, {0x0E47, 0x0E4E},
    {0x0EB1, 0x0EB1}, {0x0EB4, 0x0EBC}, {0x0EC8, 0x0ECD}, {0x0F18, 0x0F19}, {0x0F35, 0x0F35},
    {0x0F37, 0x0F37}, {0x0F39, 0x0F39}, {0x0F71, 0x0F7E}, {0x0F80, 0x0F84}, {0x0F86, 0x0F87},
    {0x0F8D, 0x0FBC}, {0x0FC6, 0x0FC6}, {0x102D, 0x1030}, {0x1032, 0x1037}, {0x1039, 0x103A},
    {0x1160, 0x11FF}, {0x135D, 0x135F}, {0x1712, 0x1714}, {0x1732, 0x1734}, {0x17B4, 0x17B5},
    {0x17B7, 0x17BD}, {0x17C6, 0x17C6}, {0x17C9, 0x17D3}, {0x17DD, 0x17DD}, {0x180B, 0x180F},
    {0x1AB0, 0x1AFF}, {0x1DC0, 0x1DFF}, {0x200B, 0x200F}, {0x202A, 0x202E}, {0x2060, 0x2064},
    {0x20D0, 0x20F0}, {0x2CEF, 0x2CF1}, {0x2DE0, 0x2DFF}, {0x302A, 0x302D}, {0x3099, 0x309A},
    {0xA66F, 0xA672}, {0xA674, 0xA67D}, {0xA69E, 0xA69F}, {0xA6F0, 0xA6F1}, {0xA8E0, 0xA8F1},
    {0xFE00, 0xFE0F}, {0xFE20, 0xFE2F}, {0xFEFF, 0xFEFF}, {0x1D167, 0x1D169},
    {0x1D173, 0x1D182}, {0x1F3FB, 0x1F3FF}, {0xE0001, 0xE007F}, {0xE0100, 0xE01EF},
};

// Wide: East Asian Wide/Fullwidth plus emoji presentation blocks.
constexpr Range kWide[] = {
    {0x1100, 0x115F},   {0x231A, 0x231B},   {0x2329, 0x232A},   {0x23E9, 0x23EC},
    {0x23F0, 0x23F0},   {0x23F3, 0x23F3},   {0x25FD, 0x25FE},   {0x2614, 0x2615},
    {0x2648, 0x2653},   {0x267F, 0x267F},   {0x2693, 0x2693},   {0x26A1, 0x26A1},
    {0x26AA, 0x26AB},   {0x26BD, 0x26BE},   {0x26C4, 0x26C5},   {0x26CE, 0x26CE},
    {0x26D4, 0x26D4},   {0x26EA, 0x26EA},   {0x26F2, 0x26F3},   {0x26F5, 0x26F5},
    {0x26FA, 0x26FA},   {0x26FD, 0x26FD},   {0x2705, 0x2705},   {0x270A, 0x270B},
    {0x2728, 0x2728},   {0x274C, 0x274C},   {0x274E, 0x274E},   {0x2753, 0x2755},
    {0x2757, 0x2757},   {0x2795, 0x2797},   {0x27B0, 0x27B0},   {0x27BF, 0x27BF},
    {0x2B1B, 0x2B1C},   {0x2B50, 0x2B50},   {0x2B55, 0x2B55},   {0x2E80, 0x303E},
    {0x3041, 0x33FF},   {0x3400, 0x4DBF},   {0x4E00, 0x9FFF},   {0xA000, 0xA4CF},
    {0xA960, 0xA97F},   {0xAC00, 0xD7A3},   {0xF900, 0xFAFF},   {0xFE10, 0xFE19},
    {0xFE30, 0xFE6F},   {0xFF00, 0xFF60},   {0xFFE0, 0xFFE6},   {0x16FE0, 0x16FE4},
    {0x17000, 0x18CFF}, {0x1B000, 0x1B2FF}, {0x1F004, 0x1F004}, {0x1F0CF, 0x1F0CF},
    {0x1F18E, 0x1F18E}, {0x1F191, 0x1F19A}, {0x1F200, 0x1F251}, {0x1F260, 0x1F265},
    {0x1F300, 0x1F320}, {0x1F32D, 0x1F335}, {0x1F337, 0x1F37C}, {0x1F37E, 0x1F393},
    {0x1F3A0, 0x1F3CA}, {0x1F3CF, 0x1F3D3}, {0x1F3E0, 0x1F3F0}, {0x1F3F4, 0x1F3F4},
    {0x1F3F8, 0x1F43E}, {0x1F440, 0x1F440}, {0x1F442, 0x1F4FC}, {0x1F4FF, 0x1F53D},
    {0x1F54B, 0x1F54E}, {0x1F550, 0x1F567}, {0x1F57A, 0x1F57A}, {0x1F595, 0x1F596},
    {0x1F5A4, 0x1F5A4}, {0x1F5FB, 0x1F64F}, {0x1F680, 0x1F6C5}, {0x1F6CC, 0x1F6CC},
    {0x1F6D0, 0x1F6D2}, {0x1F6D5, 0x1F6D7}, {0x1F6EB, 0x1F6EC}, {0x1F6F4, 0x1F6FC},
    {0x1F7E0, 0x1F7EB}, {0x1F90C, 0x1F93A}, {0x1F93C, 0x1F945}, {0x1F947, 0x1F9FF},
    {0x1FA70, 0x1FAFF}, {0x20000, 0x2FFFD}, {0x30000, 0x3FFFD},
};

template <size_t N>
bool inTable(char32_t cp, const Range (&t)[N]) {
    size_t lo = 0, hi = N;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (cp > t[mid].hi) lo = mid + 1;
        else if (cp < t[mid].lo) hi = mid;
        else return true;
    }
    return false;
}

// Length in bytes of the escape sequence starting at s[i] (s[i] == ESC), at least 1.
size_t escapeLength(const std::string& s, size_t i) {
    size_t n = s.size();
    if (i + 1 >= n) return 1;
    char k = s[i + 1];
    if (k == '[') {  // CSI: parameters then one final byte 0x40..0x7E
        size_t j = i + 2;
        while (j < n && !(static_cast<unsigned char>(s[j]) >= 0x40 &&
                          static_cast<unsigned char>(s[j]) <= 0x7E))
            ++j;
        return std::min(n, j + 1) - i;
    }
    if (k == ']' || k == 'P' || k == '_' || k == '^') {  // OSC/DCS: until BEL or ESC \ .
        size_t j = i + 2;
        while (j < n) {
            if (s[j] == '\a') return j + 1 - i;
            if (s[j] == '\x1b' && j + 1 < n && s[j + 1] == '\\') return j + 2 - i;
            ++j;
        }
        return n - i;
    }
    return 2;
}

// Calls f(cp, bytes) for each visible code point, skipping escapes.
template <typename F>
void forEachVisible(const std::string& s, F f) {
    size_t i = 0, n = s.size();
    while (i < n) {
        unsigned char c = s[i];
        if (c == 0x1b) {
            i += escapeLength(s, i);
            continue;
        }
        size_t len = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 1;
        if (i + len > n) len = 1;
        std::string bytes = s.substr(i, len);
        std::u32string cps = decodeUtf8(bytes);
        f(cps.empty() ? U'�' : cps[0], bytes);
        i += len;
    }
}

}  // namespace

std::u32string decodeUtf8(const std::string& s) {
    std::u32string out;
    out.reserve(s.size());
    size_t i = 0, n = s.size();
    while (i < n) {
        unsigned char c = s[i];
        char32_t cp;
        int extra;
        if (c < 0x80) { cp = c; extra = 0; }
        else if ((c >> 5) == 6) { cp = c & 0x1F; extra = 1; }
        else if ((c >> 4) == 14) { cp = c & 0x0F; extra = 2; }
        else if ((c >> 3) == 30) { cp = c & 0x07; extra = 3; }
        else { out.push_back(0xFFFD); ++i; continue; }
        bool ok = i + extra < n;  // all continuation bytes present
        for (int k = 1; ok && k <= extra; ++k) {
            unsigned char cc = s[i + k];
            if ((cc >> 6) != 2) ok = false;
            else cp = (cp << 6) | (cc & 0x3F);
        }
        if (!ok) { out.push_back(0xFFFD); ++i; continue; }
        out.push_back(cp);
        i += extra + 1;
    }
    return out;
}

std::string encodeUtf8(char32_t cp) {
    std::string o;
    if (cp < 0x80) o += char(cp);
    else if (cp < 0x800) { o += char(0xC0 | (cp >> 6)); o += char(0x80 | (cp & 0x3F)); }
    else if (cp < 0x10000) {
        o += char(0xE0 | (cp >> 12)); o += char(0x80 | ((cp >> 6) & 0x3F));
        o += char(0x80 | (cp & 0x3F));
    } else {
        o += char(0xF0 | (cp >> 18)); o += char(0x80 | ((cp >> 12) & 0x3F));
        o += char(0x80 | ((cp >> 6) & 0x3F)); o += char(0x80 | (cp & 0x3F));
    }
    return o;
}

int codepointWidth(char32_t cp) {
    if (cp == 0) return 0;
    if (cp < 32 || (cp >= 0x7F && cp < 0xA0)) return 0;
    if (cp < 0x300) return 1;
    if (inTable(cp, kZero)) return 0;
    if (inTable(cp, kWide)) return 2;
    return 1;
}

int displayWidth(const std::string& s) {
    int w = 0;
    forEachVisible(s, [&](char32_t cp, const std::string&) { w += codepointWidth(cp); });
    return w;
}

std::string stripAnsi(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    forEachVisible(s, [&](char32_t, const std::string& b) { out += b; });
    return out;
}

bool looksNumeric(const std::string& raw) {
    std::string s = stripAnsi(raw);
    size_t a = s.find_first_not_of(" \t"), b = s.find_last_not_of(" \t");
    if (a == std::string::npos) return false;
    s = s.substr(a, b - a + 1);
    // Allow a leading currency sign and a trailing % or unit suffix of letters.
    size_t i = 0;
    if (s[i] == '$' || s[i] == '+' || s[i] == '-') ++i;
    bool digit = false, dot = false;
    for (; i < s.size(); ++i) {
        char c = s[i];
        if (std::isdigit(static_cast<unsigned char>(c))) digit = true;
        else if (c == '.' && !dot) dot = true;
        else if (c == ',' && digit) continue;
        else break;
    }
    if (!digit) return false;
    if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
        size_t j = i + 1;
        if (j < s.size() && (s[j] == '+' || s[j] == '-')) ++j;
        if (j < s.size() && std::isdigit(static_cast<unsigned char>(s[j]))) {
            i = j;
            while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) ++i;
        }
    }
    std::string rest = s.substr(i);
    if (rest.empty() || rest == "%") return true;
    if (rest.size() <= 3)
        return std::all_of(rest.begin(), rest.end(),
                           [](char c) { return std::isalpha(static_cast<unsigned char>(c)); });
    return false;
}

std::string pad(const std::string& s, int width, Align a) {
    int w = displayWidth(s);
    if (w >= width) return s;
    int gap = width - w;
    if (a == Align::Auto) a = looksNumeric(s) ? Align::Right : Align::Left;
    switch (a) {
        case Align::Right: return std::string(gap, ' ') + s;
        case Align::Center: return std::string(gap / 2, ' ') + s + std::string(gap - gap / 2, ' ');
        default: return s + std::string(gap, ' ');
    }
}

std::string snip(const std::string& s, int width, const std::string& indicator) {
    if (width <= 0) return "";
    if (displayWidth(s) <= width) return s;
    int iw = displayWidth(indicator);
    if (iw >= width) return snip(indicator, width, "");
    int budget = width - iw, used = 0;
    std::string out;
    bool hadEscape = false;
    size_t i = 0, n = s.size();
    while (i < n) {
        if (s[i] == '\x1b') {
            size_t len = escapeLength(s, i);
            out += s.substr(i, len);
            hadEscape = true;
            i += len;
            continue;
        }
        unsigned char c = s[i];
        size_t len = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 1;
        if (i + len > n) len = 1;
        std::u32string cp = decodeUtf8(s.substr(i, len));
        int cw = codepointWidth(cp.empty() ? 0xFFFD : cp[0]);
        if (used + cw > budget) break;
        used += cw;
        out += s.substr(i, len);
        i += len;
    }
    out += std::string(budget - used, ' ');  // a wide char that did not fit leaves a space
    out += indicator;
    if (hadEscape) out += kReset;
    return out;
}

std::string toAscii(const std::string& s) {
    std::string out;
    size_t i = 0, n = s.size();
    while (i < n) {
        if (s[i] == '\x1b') {
            size_t len = escapeLength(s, i);
            out += s.substr(i, len);
            i += len;
            continue;
        }
        unsigned char c = s[i];
        if (c < 0x80) { out += char(c); ++i; continue; }
        size_t len = (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 1;
        if (i + len > n) len = 1;
        std::u32string cps = decodeUtf8(s.substr(i, len));
        char32_t cp = cps.empty() ? 0xFFFD : cps[0];
        i += len;
        char r = 0;
        if (cp == U'─' || cp == U'━' || cp == U'═' || cp == U'╶' || cp == U'╴' || cp == U'╌' ||
            cp == U'┄')
            r = '-';
        else if (cp == U'│' || cp == U'┃' || cp == U'║' || cp == U'┆' || cp == U'╎')
            r = '|';
        else if (cp >= 0x2500 && cp <= 0x257F)
            r = '+';  // every other box drawing piece is a corner or junction
        else if (cp >= U'▁' && cp <= U'█')
            r = "_.-~=+*#"[cp - U'▁'];
        else if (cp >= U'▉' && cp <= U'▏')
            r = '#';
        else if (cp == U'■' || cp == U'●' || cp == U'◆')
            r = '#';
        else if (cp == U'•' || cp == U'·')
            r = '*';
        else if (cp == U'…')
            r = '~';
        else if (cp >= 0xC0 && cp <= 0xFF && cp != 0xD7 && cp != 0xF7) {
            // Latin-1 letters fold to their base letter (é -> e). Ligatures/thorn use 1 char.
            static const char kFold[] = "AAAAAAACEEEEIIIIDNOOOOO?OUUUUYTsaaaaaaaceeeeiiiidnooooo?ouuuuyty";
            r = kFold[cp - 0xC0];
        }
        if (r) { out += r; continue; }
        int w = codepointWidth(cp);
        out += std::string(w, '?');
    }
    return out;
}

std::vector<std::string> lines(const std::string& s) {
    std::vector<std::string> out;
    size_t start = 0;
    while (true) {
        size_t nl = s.find('\n', start);
        std::string line = s.substr(start, nl == std::string::npos ? std::string::npos : nl - start);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        out.push_back(line);
        if (nl == std::string::npos) break;
        start = nl + 1;
    }
    return out;
}

std::vector<std::string> wrap(const std::string& s, int width) {
    std::vector<std::string> out;
    if (width < 1) width = 1;
    for (const std::string& para : lines(s)) {
        std::string cur;
        int curW = 0;
        size_t i = 0;
        bool any = false;
        while (i <= para.size()) {
            size_t sp = para.find(' ', i);
            std::string word = para.substr(i, sp == std::string::npos ? std::string::npos : sp - i);
            i = sp == std::string::npos ? para.size() + 1 : sp + 1;
            if (word.empty()) continue;
            any = true;
            int ww = displayWidth(word);
            // Hard split words wider than the line.
            while (ww > width) {
                if (curW > 0) { out.push_back(cur); cur.clear(); curW = 0; }
                std::string head;
                int hw = 0;
                size_t k = 0;
                std::u32string cps = decodeUtf8(word);
                for (; k < cps.size(); ++k) {
                    int cw = codepointWidth(cps[k]);
                    if (hw + cw > width) break;
                    head += encodeUtf8(cps[k]);
                    hw += cw;
                }
                if (head.empty()) { head = encodeUtf8(cps[0]); k = 1; }
                out.push_back(head);
                std::string rest;
                for (; k < cps.size(); ++k) rest += encodeUtf8(cps[k]);
                word = rest;
                ww = displayWidth(word);
            }
            if (word.empty()) continue;
            if (curW == 0) { cur = word; curW = ww; }
            else if (curW + 1 + ww <= width) { cur += " " + word; curW += 1 + ww; }
            else { out.push_back(cur); cur = word; curW = ww; }
        }
        if (curW > 0 || !any) out.push_back(cur);
    }
    return out;
}

}  // namespace misty::ascii
