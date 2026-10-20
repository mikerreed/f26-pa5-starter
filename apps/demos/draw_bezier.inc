/**
 *  Copyright 2026 Mike Reed
 */

#include "draw.h"
#include "draw_tools.h"
#include "../../include/GPathBuilder.h"
#include "../GTime.h"

//#define GDRAW_TEST_HITTEST

static bool hit_test_line(GPoint a, GPoint b, GPoint pt, float tol = 5) {
    auto u = b - a;
    const float ulen = u.length();
    u = u * (1.0f / ulen);  // normalize
    const auto v = pt - a;
    const float dp = dot(u, v);
    return dp >= 0 && dp <= ulen && std::abs(cross(u, v)) <= tol;
}

static GPoint eval_bezier(const GPoint pts[], size_t n, float t) {
    assert(n >= 2);
    if (0) {
        if (n == 2) {
            return lerp(pts[0], pts[1], t);
        }
        std::vector<GPoint> tmp(n - 1);
        for (size_t i = 0; i < n - 1; ++i) {
            tmp[i] = lerp(pts[i], pts[i + 1], t);
        }
        return eval_bezier(tmp.data(), n - 1, t);
    } else {
        GPoint array[16];
        GPoint* tmp = array;
        std::vector<GPoint> storage;
        if (n > std::size(array)) {
            storage.resize(n);
            tmp = storage.data();
        }

        for (size_t i = 0; i < n - 1; ++i) {
            tmp[i] = lerp(pts[i], pts[i + 1], t);
        }
        for (--n; n > 1; --n) {
            for (size_t i = 0; i < n - 1; ++i) {
                tmp[i] = lerp(tmp[i], tmp[i + 1], t);
            }
        }
        return tmp[0];
    }
}

static std::optional<float> hit_test_bezier(const GPoint pts[], size_t n, GPoint pt, float tol) {
    const size_t steps = 32;
    GPoint a = pts[0];
    for (size_t i = 1; i <= steps; ++i) {
        const float t = i * 1.0f / steps;
        GPoint b = eval_bezier(pts, n, t);
        if (hit_test_line(a, b, pt, tol)) {
            return t;
        }
        a = b;
    }
    return {};
}

static GColor bezier_color(int n) {
    auto c = (n & 1) ? GColor_blue : GColor_green.darken(0.5);
    return c.withAlpha(0.5);
}

static void draw_bezier_construction(GCanvas* canvas, const GPoint pts[], size_t n, float curr_t, bool first) {
    if (n < 2) return;

    GPaint paint;
    if (!first) {
        paint.setColor(bezier_color(n));
    }
    for (size_t j = 1; j < n; ++j) {
        GPoint a = pts[j - 1], b = pts[j];
        paint.setLineWidth(1);
        canvas->drawLine(a, b, paint);
        paint.setLineWidth(6);
        draw_point(canvas, a, paint);
        draw_point(canvas, b, paint);
    }

    if (n > 2) {
        std::vector<GPoint> tmp(n - 1);
        for (size_t i = 0; i < n-1; ++i) {
            tmp[i] = lerp(pts[i], pts[i+1], curr_t);
        }
        draw_bezier_construction(canvas, tmp.data(), n-1, curr_t, false);
    }
}

class BezierShape : public Shape {
    using INHERITED = Shape;
    std::vector<GPoint> fPts;
    int                 fCountSubdivisions = 32;
    int                 fCurrSubdivision = fCountSubdivisions >> 1;
    bool                fShowExtras = false;

    void pinSubdivisions() {
        fCurrSubdivision = std::clamp<int>(fCurrSubdivision, 1, fCountSubdivisions-1);
        this->updateLabel();
    }

    float currT() const {
        return fCurrSubdivision * 1.0f / fCountSubdivisions;
    }

    void updateLabel() const {
        if (fShowExtras) {
            char buffer[10];
            int n = std::snprintf(buffer, std::size(buffer), "%5.3f", this->currT());
            draw_set_label(buffer, n);
        } else {
            draw_set_label(nullptr, 0);
        }
    }

    void test_hittest(GCanvas* canvas) {
        GPathBuilder bu;
        bu.addPolygon(fPts.data(), fPts.size());
        auto r = bu.detach()->bounds().outset(10, 10);
        GPaint paint(GColor_blue.withAlpha(0.75));
        paint.setLineWidth(2);
        auto now = GTime::Secs();
        for (int loops = 0; loops < 1; ++loops) {
            for (float y = r.top; y < r.bottom; y += 2) {
                for (float x = r.left; x < r.right; x += 2) {
                    if (this->hitTest({x, y})) {
                        //draw_point(canvas, {x, y}, paint);
                    }
                }
            }
        }
        printf("time %g\n", GTime::Secs() - now);
    }
public:
    BezierShape(GPoint p) {
#ifdef GDRAW_TEST_HITTEST
        const GPoint pts[] = {
                {60, 50},
                {600, 50},
                {60, 400},
                {600, 400},
        };
        for (auto pt : pts) {
            fPts.push_back(pt);
        }
#else
        fPts.push_back(p);
        fPts.push_back(p);
#endif
    }

    GRect bounds() const override {
        return ::bounds(fPts.data(), fPts.size());
    }

    void onDraw(GCanvas* canvas) override {
        GPaint paint;

        if (fPts.size() >= 2) {
            paint.setColor(this->color());
            GPaint linePaint(this->color());
            linePaint.setLineWidth(2);
            GPoint a = fPts[0];
            for (int i = 1; i < fCountSubdivisions; ++i) {
                GPoint b = eval_bezier(fPts.data(), fPts.size(), i * 1.0f / fCountSubdivisions);
                canvas->drawLine(a, b, linePaint);
                canvas->drawCircle(b, 3, paint);
                a = b;
            }
            canvas->drawLine(a, fPts.back(), linePaint);
        }

        paint.setColor(GColor_gray);
        for (size_t i = 1; i < fPts.size(); ++i) {
            canvas->drawLine(fPts[i-1], fPts[i], paint);
        }
    }

    void onDrawHiliteBefore(GCanvas* canvas) override {
        if (fShowExtras) {
#ifdef GDRAW_TEST_HITTEST
            test_hittest(canvas); return;
#endif
            draw_bezier_construction(canvas, fPts.data(), fPts.size(), this->currT(), true);
        }
        GPaint paint;
        paint.setLineWidth(6);
        for (auto p : fPts) {
            draw_point(canvas, p, paint);
        }
    }

    void onDrawHilite(GCanvas* canvas) override {
        if (fShowExtras) {
            GPaint paint(GColor_red);
            canvas->drawCircle(eval_bezier(fPts.data(), fPts.size(), this->currT()), 4, paint);
        }
    }

    void onSetSelected(bool isOn) override {
        if (isOn) {
            this->updateLabel();
        } else {
            draw_set_label(nullptr, 0);
        }
    }

    bool hitTest(GPoint p) override {
        for (size_t i = 1; i < fPts.size(); ++i) {
            if (hit_test_line(fPts[i-1], fPts[i], p)) {
                return true;
            }
        }
        return hit_test_bezier(fPts.data(), fPts.size(), p, 5).has_value();
    }

    void onOffset(GVector v) override {
        for (auto& p : fPts) {
            p += v;
        }
    }

    bool handleSym(unsigned sym) override {
        switch (sym) {
            case 'e':
                fShowExtras = !fShowExtras;
                this->updateLabel();
                return true;
            case SDLK_UP:
                if (fCountSubdivisions < 256) {
                    fCountSubdivisions <<= 1;
                    fCurrSubdivision <<= 1;
                }
                return true;
            case SDLK_DOWN:
                if (fCountSubdivisions > 4) {
                    fCountSubdivisions >>= 1;
                    fCurrSubdivision >>= 1;
                }
                return true;
            case SDLK_LEFT:
                fCurrSubdivision -= 1;
                this->pinSubdivisions();
                return true;
            case SDLK_RIGHT:
                fCurrSubdivision += 1;
                this->pinSubdivisions();
                return true;
                default: break;
        }
        return this->INHERITED::handleSym(sym);
    }

    std::unique_ptr<GClick> onFindClick(GPoint loc, unsigned fastkeys) override {
        auto drag_point = [](GPoint clickp, GPoint* ptr) {
            return std::make_unique<GClick>(clickp, [ptr](const GClick* click) {
                *ptr = click->curr();
            });
        };
        for (size_t i = fPts.size(); i --> 0; ) {
            if (hit_test(fPts[i], loc)) {
                if (fastkeys & GFastKeys::ctrl_key) {
                    if (fPts.size() > 2) {
                        fPts.erase(fPts.begin() + i);
                    }
                    return GClick::Noop();
                }
                return drag_point(loc, &fPts[i]);
            }
        }
        if (fastkeys & GFastKeys::opt_key) {
            // add a point?
            for (size_t i = 1; i < fPts.size(); ++i) {
                if (hit_test_line(fPts[i - 1], fPts[i], loc)) {
                    fPts.insert(fPts.begin() + i, loc);
                    return drag_point(loc, &fPts[i]);
                }
            }
        }
        return nullptr;
    }
};
