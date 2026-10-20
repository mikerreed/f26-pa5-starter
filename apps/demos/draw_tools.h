

#ifndef _GDraw_tools_h_
#define _GDraw_tools_h_

#include "draw.h"
#include "../../include/GCanvas.h"
#include "../../include/GPaint.h"
#include "GWindow.h"

struct ToolIcon {
    GRect m_bounds{};

    virtual ~ToolIcon() {}

    void draw(GCanvas* canvas, bool isSelected) {
        GPaint paint;
        canvas->drawRect(m_bounds, paint);
        paint.setColor(GColor_white);
        canvas->drawRect(m_bounds.inset(1, 1), paint);

        canvas->save();
        canvas->translate(m_bounds.left + 2, m_bounds.top + 2);
        paint.setColor(isSelected ? GColor_black : GColor_gray);
        this->onDraw(canvas, GRect::WH(m_bounds.width()-4, m_bounds.height()-4), paint);
        canvas->restore();
    }

    virtual std::unique_ptr<Shape> makeShape(GPoint) = 0;

protected:
    virtual void onDraw(GCanvas*, GRect, const GPaint&) = 0;
};

struct ProcToolIcon : ToolIcon {
    using Draw = std::function<void(GCanvas*, GRect, const GPaint&)>;
    using Make = std::function<std::unique_ptr<Shape>(GPoint)>;

    Draw fDraw;
    Make fMake;

    ProcToolIcon(Draw d, Make m) : fDraw(std::move(d)), fMake(std::move(m)) {}

    std::unique_ptr<Shape> makeShape(GPoint p) override { return fMake(p); }
    void onDraw(GCanvas* canvas, GRect r, const GPaint& p) override { return fDraw(canvas, r, p); }
};

struct Toolbar {
    enum {
        kBarHeight = 10,
        kIconSize  = 32,
        kIconGap   = 4,
    };
    GPoint                                 m_topLeft{};
    std::vector<std::unique_ptr<ToolIcon>> m_tools;
    ToolIcon*                              m_selectedTool = nullptr;

    ToolIcon* addTool(std::unique_ptr<ToolIcon> tool) {
        if (!m_selectedTool) {
            m_selectedTool = tool.get();
            tool->m_bounds = {0, 0, kIconSize, kIconSize};
        } else {
            tool->m_bounds = m_tools.back()->m_bounds.offset(0, kIconSize + kIconGap);
        }
        m_tools.push_back(std::move(tool));
        return m_tools.back().get();
    }

    ToolIcon* addTool(ProcToolIcon::Draw draw, ProcToolIcon::Make make) {
        return this->addTool(std::make_unique<ProcToolIcon>(std::move(draw), std::move(make)));
    }

    void draw(GCanvas* canvas) {
        canvas->save();
        canvas->translate(m_topLeft);
        canvas->fillRect({0, 0, 32, 6}, {0.75, 0.75, 0.75, 1});
        canvas->translate(0, kBarHeight);
        for (auto& icon : m_tools) {
            icon->draw(canvas, m_selectedTool == icon.get());
        }
        canvas->restore();
    }

    std::unique_ptr<GClick> findClick(GPoint clickp) {
        const auto r = GRect::XYWH(m_topLeft.x, m_topLeft.y, 32, 6);
        if (contains(r.outset(2, 2), clickp)) {
            return std::make_unique<GClick>(clickp, [this](const GClick* c) {
               m_topLeft += c->curr() - c->prev();
            });
        }
        clickp -= m_topLeft;
        clickp.y -= kBarHeight;
        for (auto& icon : m_tools) {
            auto r = icon->m_bounds.outset(kIconGap/2, kIconGap/2);
            if (contains(r, clickp)) {
                m_selectedTool = icon.get();
                return std::make_unique<GClick>(clickp, [](const GClick*){});
            }
        }
        return nullptr;
    }

    std::unique_ptr<Shape> makeShape(GPoint p) {
        return m_selectedTool->makeShape(p);
    }
};

extern void add_tool_convex(Toolbar*);
extern void add_tool_path(Toolbar*);
extern void add_tool_curve(Toolbar*);

#endif
