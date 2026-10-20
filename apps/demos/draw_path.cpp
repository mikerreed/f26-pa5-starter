/**
 *  Copyright 2026 Mike Reed
 */

#include "draw.h"
#include "draw_tools.h"
#include "../../include/GPath.h"
#include "../../include/GPathBuilder.h"

static void draw_edge_parity(GCanvas* canvas, const GPoint pts[], int count) {
    GPaint paint;
    paint.setLineWidth(2);
    for (int i = 0; i < count; ++i) {
        const float alpha = 0.75;
        const GPoint a = pts[i],
                b = pts[(i+1)%count];
        float dy = b.y  - a.y;
        if (dy != 0) {
            paint.setColor(dy > 0 ? GColor{0, 0, 1, alpha} : GColor{0, 0.75, 0, alpha});
            canvas->drawLine(a, b, paint);
        }
    }
}

static void draw_arrow(GCanvas* canvas, GPoint p, GVector dir) {
    constexpr float w = 13;
    GPaint paint;
    paint.setColor({0.8f, 0, 0, 1});
    const GPoint pts[] = {
        { w*2/3, 0}, {-w/3, w/3}, {-w/3, -w/3}
    };
    canvas->save();
    canvas->translate(p);
    canvas->concat(GMatrix(dir, rotate_cw(dir), {0, 0}));
    canvas->drawConvexPolygon(pts, std::size(pts), paint);
    canvas->restore();
}

static void draw_arrows(GCanvas* canvas, GPoint a, GVector v, float spacing, float phase) {
    assert(v.length() > phase);
    auto len = v.length();
    auto unit = v * (1/len);
    a += unit * phase;
    len -= phase;
    while (len > 0) {
        draw_arrow(canvas, a, unit);
        a += unit * spacing;
        len -= spacing;
    }
}
static void draw_arrows(GCanvas* canvas, const GPoint pts[], int count, float spacing, float phase, bool arrows) {
    draw_edge_parity(canvas, pts, count);

    if (arrows) {
        GPoint a = pts[count - 1];
        for (int i = 0; i < count; ++i) {
            GPoint b = pts[i];
            GVector v = b - a;
            auto len = v.length();
            draw_arrows(canvas, a, v, spacing, phase);
            phase = std::fmod(phase + len, spacing);
            a = b;
        }
    }
}

bool hit_test(const GPath& path, GPoint p) {
    if (!contains(path.bounds(), p)) {
        return false;
    }
    GBitmap bm({3, 3});
    auto canvas = GCreateCanvas(bm);
    canvas->translate(1 - p.x, 1 - p.y);    // tol is += 1
    canvas->drawPath(path, GPaint());
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 3; ++x) {
            if (*bm.getAddr(x, y)) {
                return true;
            }
        }
    }
    return false;
}

static void add_corners(GPoint pts[4], GPoint p, float w, float h, bool reverse) {
    pts[0] = p;
    pts[1] = p + GVector{w, 0};
    pts[2] = p + GVector{w, h};
    pts[3] = p + GVector{0, h};
    if (reverse) {
        std::swap(pts[1], pts[3]);
    }
}
class PathShape : public Shape {
    using INHERITED = Shape;
    GPoint fPts[8];
    bool fShowExtra = false, fShowExtraArrows = false;
    bool fForceDragFirstClick = true;

    std::shared_ptr<GPath> makePath() const {
        GPathBuilder bu;
        bu.addPolygon(&fPts[0], 4);
        bu.addPolygon(&fPts[4], 4);
        return bu.detach();
    }
public:
    PathShape(GPoint p) {
        add_corners(&fPts[0], p, 150, 100, false);
        p += {50, 50};
        add_corners(&fPts[4], p, 150, 100, true);
    }

    void onDraw(GCanvas* canvas) override {
        auto path = this->makePath();
        canvas->drawPath(path, fPaint);
    }

    GRect bounds() const override { return this->makePath()->bounds(); }

    void onDrawHilite(GCanvas* canvas) override {
        if (fShowExtra) {
            float spacing = 50, phase = 10;
            draw_arrows(canvas, &fPts[0], 4, spacing, phase, fShowExtraArrows);
            draw_arrows(canvas, &fPts[4], 4, spacing, phase, fShowExtraArrows);
        }

        GPaint paint;
        paint.setLineWidth(5);
        for (auto p : fPts) {
            draw_point(canvas, p, paint);
        }

        if (fShader) {
            fShader->drawControls(canvas);
        }
    }

    bool hitTest(GPoint p) override {
        return hit_test(*this->makePath(), p);
    }

    void onOffset(GVector v) override {
        for (auto& p : fPts) {
            p += v;
        }
    }

    bool handleSym(unsigned sym) override {
        switch (sym) {
            case 'e': fShowExtra = !fShowExtra; return true;
            case 'd': fShowExtraArrows = !fShowExtraArrows; return true;
            default: break;
        }
        return this->INHERITED::handleSym(sym);
    }

    std::unique_ptr<GClick> onFindClick(GPoint loc, unsigned fastkeys) override {
        // Hack: the first call to findClick will be right after we were created,
        // so this kicks us into "drag" mode the first time, rather than resizing our
        // first point...
        if (fForceDragFirstClick) {
            fForceDragFirstClick = false;
            return std::make_unique<GClick>(loc, [this](const GClick* click) {
                this->offset(click->curr() - click->prev());
            });
        }

        for (auto& p : fPts) {
            if (hit_test(p, loc)) {
                return std::make_unique<GClick>(loc, [ptr = &p](const GClick* click) {
                    *ptr = click->curr();
                });
            }
        }
        return nullptr;
    }
};

std::shared_ptr<GPath> path_from_poly(const GPoint pts[], size_t n) {
    GPathBuilder bu;
    bu.addPolygon(pts, n);
    return bu.detach();
}

static GMatrix compute_scale_rotate(GPoint c, GPoint op, GPoint np) {
    auto oldv = op - c;
    auto newv = np - c;
    const auto oldLen = oldv.length();
    const auto newLen = newv.length();
    const float scale = newLen / oldLen;
    oldv = oldv * (1.0f / oldLen);
    newv = newv * (1.0f / newLen);
    auto e0 = GVector{dot(oldv, newv), cross(oldv, newv)};
    return GMatrix(e0, rotate_cw(e0), c) * GMatrix::Scale(scale, scale) * GMatrix::Translate(-c);
}

class FreehandShape : public Shape {
    using INHERITED = Shape;
    std::vector<GPoint> fPts;
    std::shared_ptr<GPath> fPath;
    GMatrix                fMX;
    bool fShowExtra = false;
    bool fResizing = false;


    void hair_poly(GCanvas* canvas) {
        for (size_t i = 1; i < fPts.size(); ++i) {
            canvas->hairLine(fPts[i-1], fPts[i], GColor_black);
        }
    }
public:
    FreehandShape(GPoint p) {
        fPts.push_back(p);
    }

    void onDraw(GCanvas* canvas) override {
        if (fPath) {
            canvas->save();
            canvas->concat(fMX);    // only needed really during a click session
            canvas->drawPath(fPath, fPaint);
            canvas->restore();
        } else {
            this->hair_poly(canvas);
        }
    }

    GRect bounds() const override {
        if (fPath) {
            return fPath->bounds();
        } else {
            return ::bounds(fPts.data(), fPts.size());
        }
    }

    void onDrawHilite(GCanvas* canvas) override {
        if (fPath) {
            GPoint pts[GPath::kMaxNextPoints];
            if (fShowExtra) {
                GPath::Edger edger(*fPath);
                while (auto v = edger.next(pts)) {
                    assert(*v == GPathVerb::kLine);
                    fMX.mapPoints(pts, pts, 2);
                    draw_edge_parity(canvas, pts, 2);
                }
            } else {
                GPaint paint;
                paint.setLineWidth(4);
                GPath::Iter iter(*fPath);
                while (auto v = iter.next(pts)) {
                    switch (*v) {
                        case GPathVerb::kMove:
                            break;
                        case GPathVerb::kLine:
                            pts[0] = pts[1];
                            break;
                        default: assert(false); break;
                    }
                    draw_point(canvas, fMX * pts[0], paint);
                }
            }
            if (fResizing) {
                auto c = fPath->bounds().center();
                canvas->drawCircle(c, 8, GPaint(GColor_black.withAlpha(0.5)));
                canvas->drawCircle(c, 6, GPaint(GColor_white.withAlpha(0.5)));
            }
        }

        if (fShader) {
            fShader->drawControls(canvas);
        }
    }

    bool handleSym(unsigned sym) override {
        switch (sym) {
            case 'e': fShowExtra = !fShowExtra; return true;
            default: break;
        }
        return this->INHERITED::handleSym(sym);
    }

    bool hitTest(GPoint p) override {
        auto path = fPath;
        if (!path) {
            path = path_from_poly(fPts.data(), fPts.size());
        }
        return hit_test(*path, p);
    }

    void onOffset(GVector v) override {
        fPath = fPath->offset(v.x, v.y);
    }

    std::unique_ptr<GClick> onFindClick(GPoint loc, unsigned fastkeys) override {
        if (fPts.size()) {
            // we're in create-mode
            return std::make_unique<GClick>(loc, [this](const GClick* c) {
                if (c->state() == GClick::State::kUp_State) {
                    fPath = path_from_poly(fPts.data(), fPts.size());
                    fPts.clear();
                } else {
                    constexpr float minLen = 3;
                    if ((c->curr() - fPts.back()).length() >= minLen) {
                        fPts.push_back(c->curr());
                    }
                }
            });
        }
        if (fastkeys & GFastKeys::ctrl_key) {
            auto center = fPath->bounds().center();
            fResizing = true;
            return std::make_unique<GClick>(loc, [this, center](const GClick* c) {
                if (c->state() == GClick::kUp_State) {
                    fPath = fPath->transform(fMX);
                    fMX = GMatrix();
                    fResizing = false;
                } else {
                    fMX = compute_scale_rotate(center, c->orig(), c->curr());
                }
            });
        }
        return nullptr;
    }
};

void add_tool_path(Toolbar* bar) {
    bar->addTool([](GCanvas* canvas, GRect r, const GPaint& paint) {
        r = r.inset(3, 3).offset(0, 1);
        r.right -= 5;
        r.bottom -= 8;
        GPathBuilder bu;
        bu.addRect(r, GPathDirection::kCW);
        bu.addRect(r.offset(6, 6), GPathDirection::kCCW);
        canvas->drawPath(bu.detach(), paint);
    }, [](GPoint p) {
        return std::make_unique<PathShape>(p);
    });

    bar->addTool([](GCanvas* canvas, GRect r, const GPaint& paint) {
        GRandom rand;
        auto rpt = [&]() -> GPoint {
            return { lerp(r.left, r.right, rand.nextF()),
                     lerp(r.top, r.bottom, rand.nextF()) };
        };

        GPathBuilder bu;
        bu.moveTo(rpt());
        for (int i = 0; i < 20; ++i) {
            bu.lineTo(rpt());
        }
        canvas->drawPath(bu.detach(), paint);
    }, [](GPoint p) {
        return std::make_unique<FreehandShape>(p);
    });
}
