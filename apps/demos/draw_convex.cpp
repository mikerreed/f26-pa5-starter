/**
 *  Copyright 2026 Mike Reed
 */

#include "draw.h"
#include "draw_tools.h"

static void make_regular_poly(GPoint pts[], int count, float cx, float cy, float rx, float ry) {
    float angle = 0;
    const float deltaAngle = gFloatPI * 2 / count;

    for (int i = 0; i < count; ++i) {
        pts[i] = {cx + cos(angle) * rx, cy + sin(angle) * ry};
        angle += deltaAngle;
    }
}

class ConvexShape : public Shape {
    using INHERITED = Shape;
public:
    ConvexShape(GPoint p) : fN(7) {
        fRect = make_from_pts(p, p);
    }

    GRect bounds() const override { return fRect; }

    void onDraw(GCanvas* canvas) override {
        auto r = fRect;
        float sx = r.width() * 0.5f;
        float sy = r.height() * 0.5f;
        float cx = (r.left + r.right) * 0.5f;
        float cy = (r.top + r.bottom) * 0.5f;

        std::vector<GPoint> pts(fN);
        make_regular_poly(pts.data(), fN, cx, cy, sx, sy);

        canvas->drawConvexPolygon(pts.data(), fN, fPaint);
    }

    void onDrawHilite(GCanvas* canvas) override {
        auto r = fRect;
        float sx = r.width() * 0.5f;
        float sy = r.height() * 0.5f;
        float cx = (r.left + r.right) * 0.5f;
        float cy = (r.top + r.bottom) * 0.5f;

        std::vector<GPoint> pts(fN);
        make_regular_poly(pts.data(), fN, cx, cy, sx, sy);

        GPaint paint;
        paint.setColor({0, 0, 0, 0.35f});
        paint.setLineWidth(4);
        for (int i = 0; i < fN; ++i) {
            draw_point(canvas, pts[i], paint);
        }
        draw_hilite(canvas, fRect);
    }

    bool hitTest(GPoint p) override { return contains(fRect, p); }
    void onOffset(GVector v) override {
        fRect.left += v.x;
        fRect.top += v.y;
        fRect.right += v.x;
        fRect.bottom += v.y;
    }

    bool handleSym(unsigned sym) override {
        int n = fN;
        if (sym == SDLK_LEFT) {
            n = std::max(3, n - 1);
        } else if (sym == SDLK_RIGHT) {
            n += 1;
        } else {
            return this->INHERITED::handleSym(sym);
        }
        fN = n;
        return true;
    }

    std::unique_ptr<GClick> onFindClick(GPoint loc, unsigned fastkeys) override {
        GPoint anchor;
        if (in_resize_corner(fRect, loc, &anchor)) {
            return std::make_unique<GClick>(loc, [this, anchor](const GClick* click) {
                fRect = make_from_pts(click->curr(), anchor);
            });
        }
        return nullptr;
    }

private:
    GRect   fRect;
    int     fN;
};

struct ConvexTool : public ToolIcon {
    void onDraw(GCanvas* canvas, GRect r, const GPaint& paint) override {
        r = r.inset(2, 2);
        GPoint pts[] = {
                lerp(r.TL(), r.TR(), 0.25),
                lerp(r.TL(), r.TR(), 0.75),
                lerp(r.TR(), r.BR(), 0.5),
                lerp(r.BL(), r.BR(), 0.5),
                lerp(r.TL(), r.BL(), 0.5),
        };
        canvas->drawConvexPolygon(pts, std::size(pts), paint);
    }
    std::unique_ptr<Shape> makeShape(GPoint p) override {
        return std::make_unique<ConvexShape>(p);
    }
};

void add_tool_convex(Toolbar* bar) {
    bar->addTool(std::make_unique<ConvexTool>());
}
