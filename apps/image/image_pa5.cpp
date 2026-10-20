/**
 *  Copyright 2015 Mike Reed
 */

#include "image.h"
#include "../../include/GBitmap.h"
#include "../../include/GPathBuilder.h"
#include "../../include/GRandom.h"
#include "../../include/GShader.h"

template <typename DRAW> void spin(GCanvas* canvas, int N, DRAW draw) {
    for (int i = 0; i < N; ++i) {
        canvas->save();
        canvas->rotate(2 * gFloatPI * i / N);
        draw(canvas);
        canvas->restore();
    }
}

static void add_circle(GPathBuilder* bu, GPoint center, float radius, GPathDirection dir) {
    bu->addOval({center.x - radius, center.y - radius, center.x + radius, center.y + radius}, dir);
}

static void draw_rings(GCanvas* canvas) {
    GPathBuilder bu;

    auto draw_color_path = [&](const GPath& path, float x, float y, GColor c) {
        GPaint paint(c);
        canvas->save();
        canvas->translate(x, y);
        canvas->drawPath(path, paint);
        canvas->restore();
    };

    add_circle(&bu, {256, 256}, 200, GPathDirection::kCW);
    add_circle(&bu, {256, 256}, 160, GPathDirection::kCCW);
    draw_color_path(*bu.detach(), 0, 0, {0, 1, 0, 1});

    auto r = GRect{0, 0, 200, 200};

    bu.moveTo(r.left,  r.top);
    bu.lineTo(r.right, r.top);
    bu.lineTo(r.right, r.bottom);
    bu.lineTo(r.left,  r.bottom);
    add_circle(&bu, {100, 100}, 90, GPathDirection::kCCW);
    draw_color_path(*bu.detach(), 0, 0, {1, 0, 0, 1});

    bu.moveTo(r.left,  r.top);
    bu.lineTo(r.left,  r.bottom);
    bu.lineTo(r.right, r.bottom);
    bu.lineTo(r.right, r.top);
    add_circle(&bu, {100, 100}, 90, GPathDirection::kCW);
    draw_color_path(*bu.detach(), 512-200, 512-200, {0, 0, 1, 1});
}

static void draw_bm_tiling(GCanvas* canvas) {
    const GMatrix m = GMatrix::Rotate(gFloatPI/6) * GMatrix::Scale(0.4f, 0.4f);
    auto bm = GBitmap::ReadFromFile("apps/spock.png");
    assert(bm.has_value());
    auto sh = GShader::Bitmap(*bm, m, GTileMode::kRepeat);
    canvas->drawRect(GRect::XYWH(0, 0, 512, 250), GPaint(sh));
    sh = GShader::Bitmap(*bm, m, GTileMode::kMirror);
    canvas->drawRect(GRect::XYWH(0, 262, 512, 250), GPaint(sh));
}

static void draw_cartman(GCanvas* canvas) {
    GPaint paint;
#include "cartman.475"
}

static void draw_mirror_ramp(GCanvas* canvas) {
    const GColor colors[] = {
        {1, 1, 1, 1},
        {1, 0, 0, 1},
        {0, 1, 1, 1},
        {0, 1, 0, 1},
        {1, 0, 1, 1},
        {0, 0, 1, 1},
        {1, 1, 0, 1},
    };
    constexpr size_t N = sizeof(colors) / sizeof(colors[0]);

    constexpr int ROWS = 6;
    float h = 512.f / ROWS;
    float y = 0;
    GRandom rand;
    GPaint paint;
    GColor c[ROWS+2];
    for (int i = 0; i < ROWS; ++i) {
        const int colorCount = i + 2;
        for (int j = 0; j < colorCount; ++j) {
            c[j] = colors[rand.nextU() % N];
        }
        GRect r = {0, y, 512, y+h};
        auto sh = GShader::LinearGradient({0, 0}, {256, 0}, c, colorCount, GTileMode::kMirror);
        paint.setShader(sh);
        canvas->drawRect(r, paint);
        y += h;
    }
}

static std::shared_ptr<GShader> fill_rect_bm(const GBitmap& bm, const GRect& dst, GTileMode tm = GTileMode::kClamp) {
    auto mx = GMatrix::Translate(dst.left, dst.top)
            * GMatrix::Scale(dst.width() / bm.width(), dst.height() / bm.height());
    return GShader::Bitmap(bm, mx, tm);
}

static void draw_alpha_mod(GCanvas* canvas) {
    const GRect bounds = {0, 0, 512, 512};

    auto moon = GBitmap::ReadFromFile("apps/moonrock.png");

    canvas->fillRect(bounds, GColor_black);

    const GColor colors[] = {
        GColor_red, GColor_yellow, GColor_green, GColor_cyan, GColor_blue, GColor_magenta, GColor_red,
    };
    auto sh = GShader::LinearGradient(bounds.TR(), bounds.BL(), colors, std::size(colors));

    GPaint paint;
    paint.setShader(GShader::ModAlpha(sh, 0.5));
    canvas->drawRect(bounds, paint);

    auto r = GRect::WH(bounds.cx(), bounds.cy()).inset(10, 10);
    paint.setShader(GShader::ModAlpha(fill_rect_bm(*moon, r, GTileMode::kMirror), 0.5));
    canvas->drawRect(bounds, paint);
}

static std::shared_ptr<GPath> make_flower() {
    GPathBuilder bu;

    const float c = 0.5;
    bu.moveTo({c, c});
    bu.cubicTo({2, 2}, {-2, 2}, {-c, c});
    bu.cubicTo({-2, 2}, {-2, -2}, {-c, -c});
    bu.cubicTo({-2, -2}, {2, -2}, {c, -c});
    bu.cubicTo({2, -2}, {2, 2}, {c, c});

    const float r = 1.2f;
    bu.moveTo(r, 0);
    bu.quadTo(0, 0, 0, -r);
    bu.quadTo(0, 0, -r, 0);
    bu.quadTo(0, 0, 0, r);
    bu.quadTo(0, 0, r, 0);

    return bu.detach();
}

static void image_curves(GCanvas* canvas) {
    auto path = make_flower();
    const float ctms[] = {
           128,    128, 40, 40, 0,
            50,     50, 20, 20, 1,
            50, 256-50, 20, 20, 2,
        256-50,     50, 20, 20, 3,
        256-50, 256-50, 20, 20, 4,
    };
    const GColor colors[] = {
        {1, 0, 1, 1}, GColor_blue, {0, 0.75, 0, 1}, {0, 0.75, 1, 1}, {},
    };
    const float* c = ctms;

    GPaint paint;
    paint.setShader(GShader::LinearGradient({-1, 1}, {1, -1}, colors, 4));
    for (size_t i = 0; i < std::size(ctms)/5; ++i) {
        canvas->save();
        canvas->translate(c[0], c[1]);
        canvas->scale(c[2], c[3]);
        canvas->rotate(c[4]);
        canvas->drawPath(path, paint);
        canvas->restore();
        c += 5;
        paint.setShader(nullptr);
        paint.setColor(colors[i]);
    }
}

static void draw_line_ramp(GCanvas* canvas, GCapType ct, const GColor& c) {
    GPoint a = {85, 84}, b = {512-85, 84};
    GPaint paint(c);
    paint.setCapType(ct);
    paint.setLineWidth(30);

    auto draw_dot = [canvas](GPoint p) {
        canvas->fillRect({p.x - 2, p.y - 2, p.x + 2, p.y + 2}, GColor_black);
    };
    for (int i = 0; i < 5; ++i) {
        canvas->drawLine(a, b, paint);
        draw_dot(a);
        draw_dot(b);
        a.x += 40; b.x -= 40;
        a.y += 35; b.y += 35;
    }
}

static void image_linecaps(GCanvas* canvas) {
    const GCapType caps[] = { GCapType::kSquare, GCapType::kRound, GCapType::kButt };
    const GColor colors[] = { GColor_red, GColor_green.darken(0.75), GColor_blue };
    for (int i = 0; i < 3; ++i) {
        canvas->save();
        canvas->translate(256, 256);
        canvas->rotate(i * gFloatPI * 2 / 3);
        canvas->translate(-256, -256);
        draw_line_ramp(canvas, caps[i], colors[i]);
        canvas->restore();
    }
}

REGISTER_GIMAGE(draw_rings,       512, 512,   "rings",          5);
REGISTER_GIMAGE(draw_bm_tiling,   512, 512,   "bitmap_tiling",  5);
REGISTER_GIMAGE(draw_cartman,     512, 512,   "cartman",        5);
REGISTER_GIMAGE(draw_mirror_ramp, 512, 512,   "mirror_ramp",    5);
REGISTER_GIMAGE(draw_alpha_mod,   512, 512,   "alpha_mod",      5);
REGISTER_GIMAGE(image_curves,     256, 256,   "curves",         5);
REGISTER_GIMAGE(image_linecaps,   512, 512,   "linecaps",       5);
