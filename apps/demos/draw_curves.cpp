/**
 *  Copyright 2026 Mike Reed
 */

#include "draw.h"
#include "draw_tools.h"
#include "../../include/GPathBuilder.h"

static void draw_path_points(GCanvas* canvas, const GPath& path, float width = 4) {
    GPaint paint;
    paint.setLineWidth(width);
    const GPoint* pts = path.points();
    for (size_t i = 0; i < path.countPoints(); ++i) {
        draw_point(canvas, pts[i], paint);
    }
}

struct CurveTolerance {
    static constexpr int cutoff = 5;
    int fTol = cutoff - 2;  // 1/4
    bool isCoarse() const { return fTol > cutoff; }
    bool handleSym(unsigned sym) {
        switch (sym) {
            case SDLK_LEFT:
                fTol = std::max(fTol - 1, 0);
                return true;
            case SDLK_RIGHT:
                fTol = std::min(fTol + 1, 3*cutoff);
                return true;
            default:
                break;
        }
        return false;
    }

    float tolerance() const {
        if (fTol >= cutoff) {
            return 1 << (fTol - cutoff);
        } else {
            return 1.0f / (1 << (cutoff - fTol));
        }
    }

    void applyToPaint(GPaint* paint) {
        paint->setCurveTolerance(this->tolerance());
    }

    void updateLabel() const {
        char buffer[32];
        int n = std::snprintf(buffer, std::size(buffer), "%g", this->tolerance());
        draw_set_label(buffer, n);
    }
};

static GMatrix make_unit_matrix(const GRect& r) {
    return GMatrix::Translate(r.TL())
         * GMatrix::Scale(r.width(), r.height())
         * GMatrix::Translate(0.5, 0.5);
}

static std::shared_ptr<GPath> make_oval(const GRect& r) {
    GPathBuilder bu;
    bu.addOval(r, GPathDirection::kCW);
    return bu.detach();
}

class OvalShape : public RectShape {
    using INHERITED = RectShape;
    bool fShowExtra = false;
    CurveTolerance fCurveTol;
public:
    OvalShape(GPoint p) : RectShape(p) {}

    void onDraw(GCanvas* canvas) override {
        fCurveTol.applyToPaint(&fPaint);
        canvas->drawOval(fRect, fPaint);
    }

    void onDrawHiliteBefore(GCanvas* canvas) override {
        if (fCurveTol.isCoarse()) {
            GPaint paint(GColor_green.withAlpha(0.25));
            canvas->drawOval(fRect, paint);
        }
    }

    void onDrawHilite(GCanvas* canvas) override {
        if (fShowExtra) {
            draw_path_points(canvas, *make_oval(fRect));
        }
        this->INHERITED::onDrawHilite(canvas);
    }

    bool hitTest(GPoint p) override {
        p = *make_unit_matrix(fRect).invert() * p;
        return p.length() <= 0.5;
    }

    void onSetSelected(bool isOn) override {
        if (isOn) {
            fCurveTol.updateLabel();
        } else {
            draw_set_label(nullptr, 0);
        }
    }

    bool handleSym(unsigned sym) override {
        switch (sym) {
            case 'e': fShowExtra = !fShowExtra; return true;
            default: break;
        }
        if (fCurveTol.handleSym(sym)) {
            fCurveTol.updateLabel();
            return true;
        }
        return this->INHERITED::handleSym(sym);
    }
};

////////////////

static GRect control_point_bounds(const GPath& path) {
    GRect r = {0, 0, 0, 0};
    auto accumulate = [&r](GPoint p) {
        r.left   = std::min(r.left, p.x);
        r.top    = std::min(r.top, p.y);
        r.right  = std::max(r.right, p.x);
        r.bottom = std::max(r.bottom, p.y);
    };
    if (auto n = path.countPoints()) {
        const GPoint* pts = path.points();
        r = {pts[0].x, pts[0].y, pts[0].x, pts[0].y};
        for (size_t i = 1; i < n; ++i) {
            accumulate(pts[i]);
        }
    }
    return r;
}

struct Knot {
    GPoint pt;
    GVector prev, next;
};

class KnotShape : public Shape {
    using INHERITED = RectShape;
    std::vector<Knot> fKnots;
    CurveTolerance fCurveTol;
    bool           fShowExtras = false;

    enum class Mode {
        append, resize,
    };
    Mode fMode;

    std::shared_ptr<GPath> makePath() const {
        GPathBuilder bu;
        const GVector empty = {0, 0};

        auto segment = [&](const Knot& prev, const Knot& next) {
            if (prev.next == empty && next.prev == empty) {
                bu.lineTo(next.pt);
            } else {
                bu.cubicTo(prev.pt + prev.next, next.pt + next.prev, next.pt);
            }
        };

        bu.moveTo(fKnots[0].pt);
        const auto n = fKnots.size();
        for (size_t i = 1; i < n; ++i) {
            segment(fKnots[i-1], fKnots[i]);
        }
        if (n > 1) {
            segment(fKnots[n-1], fKnots[0]);
        }
        return bu.detach();
    }
public:
    KnotShape(GPoint p) {
        fMode = Mode::append;
    }

    GRect bounds() const override {
        return this->makePath()->bounds();
    }

    void onDraw(GCanvas* canvas) override {
        fCurveTol.applyToPaint(&fPaint);
        canvas->drawPath(this->makePath(), fPaint);
    }

    void onDrawHiliteBefore(GCanvas* canvas) override {
        auto stroke_rect = [canvas](const GRect& r, const GColor& c) {
            GPathBuilder bu;
            bu.addRect(r, GPathDirection::kCW);
            bu.addRect(r.inset(1, 1), GPathDirection::kCCW);
            canvas->drawPath(bu.detach(), GPaint(c));
        };
        if (fShowExtras) {
            auto path = this->makePath();
            stroke_rect(path->bounds(), GColor_blue);
            stroke_rect(control_point_bounds(*path), GColor_red);
        }
        if (fCurveTol.isCoarse()) {
            GPaint paint(GColor_green.withAlpha(0.25));
            canvas->drawPath(this->makePath(), paint);
        }
    }
    void onDrawHilite(GCanvas* canvas) override {
        GPaint linePaint, pointPaint;
        linePaint.setHairline();
        pointPaint.setLineWidth(4);
        for (const auto& k : fKnots) {
            canvas->drawLine(k.pt, k.pt + k.next, linePaint);
            canvas->drawLine(k.pt, k.pt + k.prev, linePaint);
            draw_point(canvas, k.pt, pointPaint);
            draw_point(canvas, k.pt + k.next, pointPaint);
            draw_point(canvas, k.pt + k.prev, pointPaint);
        }
    }

    void onSetSelected(bool isOn) override {
        if (isOn) {
            fCurveTol.updateLabel();
        } else {
            draw_set_label(nullptr, 0);
        }
    }

    bool hitTest(GPoint p) override {
        return hit_test(*this->makePath(), p);
    }

    void onOffset(GVector v) override {
        for (auto& k : fKnots) {
            k.pt += v;
        }
    }

    bool handleSym(unsigned sym) override {
        switch (sym) {
            case 'e': fShowExtras = !fShowExtras; return true;
            case SDLK_ESCAPE:
                if (fMode == Mode::append) {
                    fMode = Mode::resize;
                    return true;
                }
                break;
            default: break;
        }
        if (fCurveTol.handleSym(sym)) {
            fCurveTol.updateLabel();
            return true;
        }
        return this->INHERITED::handleSym(sym);
    }

    std::unique_ptr<GClick> onFindClick(GPoint loc, unsigned fastkeys) override {
        auto drag_pt = [loc](Knot* ptr) {
            return std::make_unique<GClick>(loc, [ptr](const GClick* click) {
                ptr->pt = click->curr();
            });
        };

        auto drag_vec = [loc](GVector* vec, GVector* other, GPoint pt) {
            return std::make_unique<GClick>(loc, [vec, other, pt](const GClick* click) {
                *vec = click->curr() - pt;
                if (other) {
                    *other = - *vec;
                }
            });

        };

        // change modes if we click back on the first point
        if (fKnots.size() > 1 && hit_test(fKnots.front().pt, loc)) {
            fMode = Mode::resize;
            return drag_pt(&fKnots.front());
        }

        if (fMode == Mode::append) {
            fKnots.push_back({loc, {}, {}});
            // fall through ...
        }
        for (auto& k : fKnots) {
            GVector* vec = nullptr, *other = nullptr;
            if (hit_test(k.pt + k.next, loc)) {
                vec = &k.next;
                other = &k.prev;
            } else if (hit_test(k.pt + k.prev, loc)) {
                vec = &k.prev;
                other = &k.next;
            }
            if (vec) {
                if (fastkeys & GFastKeys::opt_key) {
                    other = nullptr;
                }
                return drag_vec(vec, other, k.pt);
            }
            if (hit_test(k.pt, loc)) {
                return drag_pt(&k);
            }
        }
        return nullptr;
    }
};

#include "draw_bezier.inc"

void add_tool_curve(Toolbar* bar) {
    bar->addTool([](GCanvas* canvas, GRect r, const GPaint& paint) {
        canvas->drawOval(r.inset(4, 4), paint);
    }, [](GPoint p) {
        return std::make_unique<OvalShape>(p);
    });

    bar->addTool([](GCanvas* canvas, GRect r, const GPaint& paint) {
        r = r.inset(3, 3);
        GPathBuilder bu;
        bu.moveTo(r.TL());
        bu.cubicTo(lerp(r.TL(), r.TR(), 2), lerp(r.BR(), r.BL(), 2), r.BR());
        canvas->drawPath(bu.detach(), paint);
    }, [](GPoint p) {
        return std::make_unique<KnotShape>(p);
    });

    bar->addTool([](GCanvas* canvas, GRect r, const GPaint& paint) {
        r = r.inset(5, 5);
        const GPoint pts[] = {
            r.TL(), r.TR(), r.BL(), r.BR()
        };
        for (size_t i = 1; i < std::size(pts); ++i) {
            canvas->drawLine(pts[i-1], pts[i], paint);
        }

        GPaint p2 = paint;
        p2.setLineWidth(5);
        for (auto p : pts) {
            draw_point(canvas, p, p2);
        }
    }, [](GPoint p) {
        return std::make_unique<BezierShape>(p);
    });
}
