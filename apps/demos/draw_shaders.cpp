/**
 *  Copyright 2026 Mike Reed
 */

#include "draw.h"

static void strokePolygon(GCanvas* canvas, const GPoint pts[], size_t n, bool closed, const GPaint& paint) {
    for (size_t i = 1; i < n; ++i) {
        canvas->drawLine(pts[i-1], pts[i], paint);
    }
    if (closed && n > 2) {
        canvas->drawLine(pts[n-1], pts[0], paint);
    }
}

struct BitmapSH : public Shader {
    GBitmap m_bm;
    GRect   m_dst;

    BitmapSH(const GBitmap& bm, const GRect& dst) : m_bm(bm), m_dst(dst) {}

    void drawControls(GCanvas* canvas) override {
        GPaint paint(GColor_black.withAlpha(0.5));
        const GPoint pts[] = {
            m_dst.TL(), m_dst.TR(), m_dst.BR(), m_dst.BL(),
        };
        strokePolygon(canvas, pts, std::size(pts), true, paint);
    }
    std::unique_ptr<GClick> findClick(GPoint clickp) override {
        GPoint anchor;
        if (in_resize_corner(m_dst, clickp, &anchor)) {
            return std::make_unique<GClick>(clickp, [this, anchor](const GClick *c) {
                m_dst = make_from_pts(c->curr(), anchor);
            });
        }
        return nullptr;
    }
    void offset(GVector v) override { m_dst = m_dst.offset(v); }
    void install(GPaint* paint) override {
        GMatrix lm = GMatrix::Translate(m_dst.TL())
                   * GMatrix::Scale(m_dst.width()/m_bm.width(), m_dst.height()/m_bm.height());
        auto sh = GShader::Bitmap(m_bm, lm, fTileMode);
        auto sh2 = GShader::ModAlpha(sh, paint->alpha());
        paint->setShader(sh2);
    }
};

std::unique_ptr<Shader> Shader::Bitmap(const GBitmap& bm, const GRect& dst) {
    return std::make_unique<BitmapSH>(bm, dst);
}

struct GradientSH : public Shader {
    std::vector<GColor> m_colors;
    std::array<GPoint, 2> m_pts;

    void drawControls(GCanvas* canvas) override {
        GPaint paint(GColor_black.withAlpha(0.5));
        canvas->drawLine(m_pts[0], m_pts[1], paint);

        paint.setLineWidth(5);
        for (auto p : m_pts) {
            draw_point(canvas, p, paint);
        }
    }
    std::unique_ptr<GClick> findClick(GPoint clickp) override {
        for (auto& p : m_pts) {
            if (hit_test(clickp, p)) {
                return std::make_unique<GClick>(clickp, [&](const GClick *c) {
                    p = c->curr();
                });
            }
        }
        return nullptr;
    }
    void offset(GVector v) override {
        for (auto& p : m_pts) {
            p += v;
        }
    }
    void setColor(const GColor& newColor) override {
        for (auto& c : m_colors) {
            c.a = newColor.a;
        }
    }

    void install(GPaint* paint) override {
        paint->setShader(GShader::LinearGradient(m_pts[0], m_pts[1], m_colors.data(), m_colors.size(),
                                                 fTileMode));
    }
};

std::unique_ptr<Shader> Shader::LinearGradient(GPoint a, GPoint b, const GColor colors[], size_t n) {
    auto sh = std::make_unique<GradientSH>();
    sh->m_colors.insert(sh->m_colors.begin(), colors, colors + n);
    sh->m_pts[0] = a;
    sh->m_pts[1] = b;
    return sh;
}
