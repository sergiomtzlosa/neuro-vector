#include "VecFont.h"
#include "LineRenderer.h"
#include <map>
#include <cctype>

namespace nv {

// ---- glyph encoding -------------------------------------------------------
// flat float array: { nStrokes, nPts, x,y, ..., nPts, x,y, ... }
// grid: x in [0,4], y in [0,6], baseline y=0
static const float G_A[] = {2, 3, 0,0, 2,6, 4,0,  2, 0.7f,2.3f, 3.3f,2.3f};
static const float G_B[] = {2, 7, 0,0, 0,6, 2.8f,6, 3.7f,5.3f, 3.7f,3.7f, 2.8f,3, 0,3,
                               5, 2.8f,3, 3.9f,2.3f, 3.9f,0.7f, 2.9f,0, 0,0};
static const float G_C[] = {1, 8, 3.8f,5.4f, 2.9f,6, 1,6, 0,4.8f, 0,1.2f, 1,0, 2.9f,0, 3.8f,0.6f};
static const float G_D[] = {1, 7, 0,0, 0,6, 2.6f,6, 3.8f,4.8f, 3.8f,1.2f, 2.6f,0, 0,0};
static const float G_E[] = {2, 4, 3.8f,6, 0,6, 0,0, 3.8f,0,  2, 0,3, 2.9f,3};
static const float G_F[] = {2, 3, 3.8f,6, 0,6, 0,0,  2, 0,3, 2.9f,3};
static const float G_G[] = {1, 10, 3.8f,5.4f, 2.9f,6, 1,6, 0,4.8f, 0,1.2f, 1,0, 2.9f,0, 3.8f,0.8f, 3.8f,2.4f, 2.1f,2.4f};
static const float G_H[] = {3, 2, 0,0, 0,6,  2, 3.9f,0, 3.9f,6,  2, 0,3, 3.9f,3};
static const float G_I[] = {3, 2, 1.9f,0, 1.9f,6,  2, 0.8f,6, 3,6,  2, 0.8f,0, 3,0};
static const float G_J[] = {1, 5, 3.3f,6, 3.3f,1.4f, 2.4f,0, 1,0, 0.2f,1};
static const float G_K[] = {3, 2, 0,0, 0,6,  2, 3.9f,6, 0.2f,2.6f,  2, 1.5f,3.5f, 3.9f,0};
static const float G_L[] = {1, 3, 0,6, 0,0, 3.6f,0};
static const float G_M[] = {1, 5, 0,0, 0,6, 1.95f,2.2f, 3.9f,6, 3.9f,0};
static const float G_N[] = {1, 4, 0,0, 0,6, 3.9f,0, 3.9f,6};
static const float G_O[] = {1, 11, 1.95f,0, 3.2f,0.7f, 3.9f,2.2f, 3.9f,3.8f, 3.2f,5.3f, 1.95f,6, 0.7f,5.3f, 0,3.8f, 0,2.2f, 0.7f,0.7f, 1.95f,0};
static const float G_P[] = {1, 7, 0,0, 0,6, 2.8f,6, 3.8f,5.2f, 3.8f,3.8f, 2.8f,3, 0,3};
static const float G_Q[] = {2, 11, 1.95f,0, 3.2f,0.7f, 3.9f,2.2f, 3.9f,3.8f, 3.2f,5.3f, 1.95f,6, 0.7f,5.3f, 0,3.8f, 0,2.2f, 0.7f,0.7f, 1.95f,0,
                               2, 2.5f,1.4f, 4,-0.6f};
static const float G_R[] = {2, 7, 0,0, 0,6, 2.8f,6, 3.8f,5.2f, 3.8f,3.8f, 2.8f,3, 0,3,
                               2, 2.2f,3, 3.9f,0};
static const float G_S[] = {1, 11, 3.7f,5.3f, 2.8f,6, 1.1f,6, 0.2f,5.1f, 0.5f,4, 2.6f,3.3f, 3.6f,2.4f, 3.6f,1.1f, 2.7f,0, 1,0, 0.2f,0.7f};
static const float G_T[] = {2, 2, 0,6, 3.9f,6,  2, 1.95f,6, 1.95f,0};
static const float G_U[] = {1, 6, 0,6, 0,1.2f, 1,0, 2.9f,0, 3.9f,1.2f, 3.9f,6};
static const float G_V[] = {1, 3, 0,6, 1.95f,0, 3.9f,6};
static const float G_W[] = {1, 5, 0,6, 0.9f,0, 1.95f,3.2f, 3,0, 3.9f,6};
static const float G_X[] = {2, 2, 0,0, 3.9f,6,  2, 0,6, 3.9f,0};
static const float G_Y[] = {2, 2, 0,6, 1.95f,2.6f,  3, 3.9f,6, 1.95f,2.6f, 1.95f,0};
static const float G_Z[] = {1, 4, 0,6, 3.9f,6, 0,0, 3.9f,0};
static const float G_0[] = {2, 11, 1.95f,0, 3.2f,0.7f, 3.9f,2.2f, 3.9f,3.8f, 3.2f,5.3f, 1.95f,6, 0.7f,5.3f, 0,3.8f, 0,2.2f, 0.7f,0.7f, 1.95f,0,
                               2, 0.8f,1.5f, 3.1f,4.5f};
static const float G_1[] = {2, 3, 0.7f,4.6f, 1.95f,6, 1.95f,0,  2, 0.7f,0, 3.2f,0};
static const float G_2[] = {1, 7, 0.2f,5, 1,6, 2.8f,6, 3.7f,5, 3.5f,3.6f, 0.2f,0, 3.7f,0};
static const float G_3[] = {1, 9, 0.3f,6, 3.6f,6, 1.9f,3.7f, 3.1f,3.7f, 3.9f,2.7f, 3.9f,1, 2.8f,0, 1,0, 0.2f,0.8f};
static const float G_4[] = {1, 4, 2.8f,0, 2.8f,6, 0,2.1f, 3.9f,2.1f};
static const float G_5[] = {1, 10, 3.6f,6, 0.4f,6, 0.2f,3.4f, 1.8f,3.9f, 3.2f,3.2f, 3.8f,1.9f, 3.5f,0.6f, 2.4f,0, 0.8f,0, 0.2f,0.8f};
static const float G_6[] = {1, 12, 3.5f,5.5f, 2.4f,6, 1,5.1f, 0.2f,3.4f, 0.2f,1.4f, 1,0.2f, 2.6f,0, 3.6f,0.8f, 3.7f,2.1f, 2.8f,3.3f, 1.2f,3.5f, 0.2f,2.7f};
static const float G_7[] = {1, 3, 0,6, 3.9f,6, 1.3f,0};
static const float G_8[] = {1, 18, 1.9f,3.2f, 3.2f,3.9f, 3.5f,4.9f, 2.8f,5.8f, 1.9f,6, 1,5.8f, 0.3f,4.9f, 0.6f,3.9f, 1.9f,3.2f,
                            3.2f,2.5f, 3.9f,1.4f, 3.5f,0.4f, 2.4f,0, 1.4f,0, 0.3f,0.4f, 0,1.4f, 0.7f,2.5f, 1.9f,3.2f};
static const float G_9[] = {1, 12, 0.4f,0.5f, 1.5f,0, 2.9f,0.9f, 3.7f,2.6f, 3.7f,4.6f, 2.9f,5.8f, 1.3f,6, 0.3f,5.2f, 0.2f,3.9f, 1.1f,2.7f, 2.7f,2.5f, 3.7f,3.3f};
static const float G_SP[] = {0};
static const float G_DASH[] = {1, 2, 0.7f,3, 3.2f,3};
static const float G_DOT[]  = {1, 2, 1.7f,0.25f, 2.2f,0.25f};
static const float G_COMMA[] = {1, 2, 2.1f,0.7f, 1.6f,-0.8f};
static const float G_COLON[] = {2, 2, 1.7f,0.3f, 2.2f,0.3f,  2, 1.7f,2.9f, 2.2f,2.9f};
static const float G_BANG[] = {2, 2, 1.75f,1.3f, 1.75f,6,  2, 1.7f,0.2f, 2.2f,0.2f};
static const float G_QUEST[] = {2, 7, 0.3f,5.1f, 1.1f,6, 2.8f,6, 3.6f,5, 3.4f,3.7f, 2,2.9f, 1.95f,1.9f,  2, 1.7f,0.2f, 2.2f,0.2f};
static const float G_LP[] = {1, 4, 3.2f,6, 0.9f,4.2f, 0.9f,1.8f, 3.2f,0};
static const float G_RP[] = {1, 4, 0.7f,6, 3,4.2f, 3,1.8f, 0.7f,0};
static const float G_SLASH[] = {1, 2, 0.2f,0, 3.7f,6};
static const float G_PLUS[] = {2, 2, 0.5f,3, 3.4f,3,  2, 1.95f,1.55f, 1.95f,4.45f};
static const float G_EQ[] = {2, 2, 0.7f,3.6f, 3.2f,3.6f,  2, 0.7f,2.4f, 3.2f,2.4f};
static const float G_OE[] = {3, 11, 1.95f,0, 3.2f,0.7f, 3.9f,2.2f, 3.9f,3.8f, 3.2f,5.3f, 1.95f,6, 0.7f,5.3f, 0,3.8f, 0,2.2f, 0.7f,0.7f, 1.95f,0,
                                2, 0.9f,6.6f, 1.4f,6.6f,  2, 2.5f,6.6f, 3,6.6f};
static const float G_UE[] = {3, 6, 0,6, 0,1.2f, 1,0, 2.9f,0, 3.9f,1.2f, 3.9f,6,
                                2, 0.9f,6.6f, 1.4f,6.6f,  2, 2.5f,6.6f, 3,6.6f};
static const float G_APO[] = {1, 2, 1.9f,4.8f, 2.4f,6};
static const float G_UND[] = {1, 2, 0.2f,0, 3.7f,0};

struct GlyphDef { char ch; float adv; const float* d; };
static const GlyphDef TABLE[] = {
    {'A',5,G_A},{'B',5,G_B},{'C',5,G_C},{'D',5,G_D},{'E',5,G_E},{'F',5,G_F},{'G',5,G_G},
    {'H',5,G_H},{'I',3.8f,G_I},{'J',5,G_J},{'K',5,G_K},{'L',5,G_L},{'M',5.5f,G_M},{'N',5.5f,G_N},
    {'O',5,G_O},{'P',5,G_P},{'Q',5.6f,G_Q},{'R',5,G_R},{'S',5,G_S},{'T',5,G_T},{'U',5,G_U},
    {'V',5,G_V},{'W',5.5f,G_W},{'X',5,G_X},{'Y',5,G_Y},{'Z',5,G_Z},
    {'0',5,G_0},{'1',5,G_1},{'2',5,G_2},{'3',5,G_3},{'4',5,G_4},{'5',5,G_5},{'6',5,G_6},
    {'7',5,G_7},{'8',5,G_8},{'9',5,G_9},
    {' ',2.8f,G_SP},{'-',5,G_DASH},{'.',3,G_DOT},{',',3,G_COMMA},{':',3,G_COLON},
    {'!',3.5f,G_BANG},{'?',5,G_QUEST},{'(',4,G_LP},{')',4,G_RP},{'/',5,G_SLASH},
    {'+',5,G_PLUS},{'=',5,G_EQ},{'\x96',5,G_OE},{'\x9C',5,G_UE},{'\'',2.5f,G_APO},{'_',5,G_UND},
    {0,5,G_SP}
};

// ---- runtime --------------------------------------------------------------
namespace {
struct RGlyph { float adv; std::vector<std::vector<Vec2>> strokes; };
std::map<int, RGlyph> g_glyphs; // key: (utf8 two-byte) or single char
bool g_init = false;

int keyOf(const char*& s) {
    unsigned char c = (unsigned char)*s;
    if (c == 0xC3 && s[1]) { // umlauts (UTF-8 two-byte)
        unsigned char c2 = (unsigned char)s[1];
        s += 2;
        if (c2 == 0x96) return 'O' * 256 + 1; // Ö
        if (c2 == 0x9C) return 'U' * 256 + 1; // Ü
        return 'X' * 256 + 2; // unknown -> skip
    }
    s += 1;
    return std::toupper(c);
}
} // namespace

void VecFont::init() {
    if (g_init) return;
    g_init = true;
    for (const GlyphDef& gd : TABLE) {
        int key = (unsigned char)gd.ch;
        if (key == 0x96) key = 'O' * 256 + 1;      // Ö (UTF-8 C3 96)
        else if (key == 0x9C) key = 'U' * 256 + 1; // Ü (UTF-8 C3 9C)
        RGlyph g;
        g.adv = gd.adv;
        const float* p = gd.d;
        int ns = (int)*p++;
        for (int i = 0; i < ns; i++) {
            int npts = (int)*p++;
            std::vector<Vec2> pts;
            for (int k = 0; k < npts; k++) {
                Vec2 v{*p++, *p++};
                pts.push_back(v);
            }
            g.strokes.push_back(std::move(pts));
        }
        g_glyphs[key] = std::move(g);
    }
}

static const RGlyph* glyphFor(int key) {
    auto it = g_glyphs.find(key);
    if (it != g_glyphs.end()) return &it->second;
    it = g_glyphs.find(std::toupper(key));
    return it != g_glyphs.end() ? &it->second : nullptr;
}

float VecFont::measure(const char* text, float size) {
    float s = size / 6.f;
    float w = 0;
    const char* p = text;
    while (*p) {
        int key = keyOf(p);
        const RGlyph* g = glyphFor(key);
        w += (g ? g->adv : 5.f) * s;
    }
    return w;
}

float VecFont::width(const char* text, float size) { return measure(text, size); }

void VecFont::draw(LineRenderer& lr, const char* text, float x, float y, float size,
                   Color c, int align, int glow) {
    if (!g_init) init();
    float s = size / 6.f;
    float w = measure(text, size);
    if (align == CENTER) x -= w * 0.5f;
    else if (align == RIGHT) x -= w;
    float pen = 0;
    const char* p = text;
    while (*p) {
        int key = keyOf(p);
        const RGlyph* g = glyphFor(key);
        if (g) {
            for (const auto& st : g->strokes) {
                for (size_t i = 0; i + 1 < st.size(); i++) {
                    Vec2 a = st[i], b = st[i + 1];
                    lr.line(x + pen + a.x * s, y - a.y * s, x + pen + b.x * s, y - b.y * s, c, glow);
                }
            }
            pen += g->adv * s;
        } else {
            pen += 5.f * s;
        }
    }
}

} // namespace nv
