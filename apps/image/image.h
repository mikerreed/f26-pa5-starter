/**
 *  Copyright 2015 Mike Reed
 */

#ifndef G_image_DEFINED
#define G_image_DEFINED

#include "../auto_register.h"
#include "../../include/GBlendMode.h"
#include "../../include/GCanvas.h"
#include "../../include/GPaint.h"
#include "../../include/GPoint.h"
#include "../../include/GRect.h"

using GDrawProc = void(GCanvas*);

struct GDrawRec {
    GDrawProc*  fDraw;
    GISize      fSize;
    const char* fName;
    int         fPA;
};

using ImageRegistrant = GRegistrant<GDrawRec>;
#define REGISTER_GIMAGE(proc, w, h, name, pa) \
    static ImageRegistrant G_MACRO_UNIQUE_NAME(gimage_proc)({proc, {w, h}, name, pa})

/// shared helpers

static inline std::array<GPoint, 4> rect_pts(const GRect& r) {
    return { r.TL(), r.TR(), r.BR(), r.BL() };
}

static inline void outer_frame(GCanvas* canvas, const GRect& rect) {
    GPaint paint;
    paint.setLineWidth(1);
    auto hline = [&](GPoint p, float w) {
        canvas->drawLine(p, {p.x + w, p.y}, paint);
    };
    auto vline = [&](GPoint p, float h) {
        canvas->drawLine(p, {p.x, p.y + h}, paint);
    };
    auto r = rect.outset(1.5f, 1.5f);
    hline({r.left - 0.5f, r.top}, r.width() + 1);
    hline({r.left - 0.5f, r.bottom}, r.width() + 1);
    vline({r.left, r.top - 0.5f}, r.height() + 1);
    vline({r.right, r.top - 0.5f}, r.height() + 1);
}

static inline void draw_all_blendmodes(GCanvas* canvas,
                                       void (*proc)(GCanvas*, const GRect&, GBlendMode)) {
    canvas->clear({1,1,1,1});

    const float W = 100;
    const float H = 100;
    const float margin = 10;
    float x = margin;
    float y = margin;
    for (int i = 0; i < 12; ++i) {
        GBlendMode mode = static_cast<GBlendMode>(i);
        canvas->save();
        canvas->translate(x, y);
        proc(canvas, GRect::WH(W, H), mode);
        canvas->restore();
        if (i % 4 == 3) {
            y += H + margin;
            x = margin;
        } else {
            x += W + margin;
        }
    }
}

#endif
