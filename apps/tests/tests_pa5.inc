/**
 *  Copyright 2018 Mike Reed
 */

static void test_edger_quads(GTestStats* stats) {
    const GPoint p[] = { {10, 10}, {20, 20}, {30, 30} };
    int N = 0;

    GPathBuilder bu;
    bu.moveTo(p[0]); bu.quadTo(p[1], p[2]);  N++;
    bu.moveTo(p[0]); bu.quadTo(p[1], p[2]);  N++;
    bu.moveTo(p[0]); bu.quadTo(p[1], p[2]);  N++;
    auto path = bu.detach();

    GPath::Edger edger(*path);
    GPoint pts[3];
    for (int i = 0; i < N; ++i) {
        assert(edger.next(pts) == GPathVerb::kQuad);
        for (int j = 0; j < 3; ++j) {
            assert(pts[j] == p[j]);
        }
        assert(edger.next(pts) == GPathVerb::kLine);    // auto close
        assert(pts[0] == p[2]);
        assert(pts[1] == p[0]);
    }
    assert(!edger.next(pts));
}

static void test_path_oval(GTestStats* stats) {
    GPathBuilder bu;
    const GRect r = {-5, 5, 20, 30};
    
    for (GPathDirection dir : { GPathDirection::kCW, GPathDirection::kCCW }) {
        bu.reset();
        bu.addOval(r, dir);
        auto p = bu.detach();
        EXPECT_TRUE(stats, r == p->bounds());
    }
}

static void test_path_transform2(GTestStats* stats) {
    GPathBuilder bu;

    bu.moveTo(10, 10); bu.lineTo(20, 10); bu.lineTo(20, 40); bu.quadTo(10, 40, 10, 10);
    auto p = bu.detach();
    EXPECT_TRUE(stats, p->bounds() == GRect::LTRB(10, 10, 20, 40));

    GMatrix mx = GMatrix::Scale(2, 3);

    auto q = p->transform(mx);

    GPath::Iter iter_p(*p),
                iter_q(*q);
    GPoint      pts_p[GPath::kMaxNextPoints],
                pts_q[GPath::kMaxNextPoints];

    for (;;) {
        auto np = iter_p.next(pts_p);
        auto nq = iter_q.next(pts_q);
        EXPECT_EQ(stats, np.has_value(), nq.has_value());
        if (!np.has_value() || !nq.has_value()) {
            break;
        }
        EXPECT_TRUE(stats, np.value() == nq.value());

        int count;
        switch (np.value()) {
            case GPathVerb::kMove:  count = 1; break;
            case GPathVerb::kLine:  count = 2; break;
            case GPathVerb::kQuad:  count = 3; break;
            case GPathVerb::kCubic: count = 4; break;
        }
        mx.mapPoints(pts_p, count);
        for (int i = 0; i < count; ++i) {
            EXPECT_TRUE(stats, pts_p[i] == pts_q[i]);
        }
    }
}

static bool nearly_eq(float a, float b) {
    return fabs(a - b) <= 0.0001f;
}

static bool nearly_eq(const GRect& a, const GRect& b) {
    return nearly_eq(a.left,   b.left)  &&
           nearly_eq(a.top,    b.top)   &&
           nearly_eq(a.right,  b.right) &&
           nearly_eq(a.bottom, b.bottom);
}

static void test_path_bounds(GTestStats* stats) {
    auto check_bounds = [&](GRect bounds, GRect expected) {
        EXPECT_TRUE(stats, nearly_eq(bounds, expected));
    };

    GPathBuilder bu;
    check_bounds(bu.detach()->bounds(), {0,0,0,0});

    bu.moveTo(0, 0);
    bu.quadTo(150, 0, 75, 75);
    bu.quadTo(0, 150, 0, 0);
    check_bounds(bu.detach()->bounds(), {0, 0, 100, 100});

    bu.moveTo (50, 100);
    bu.cubicTo(100, 0, 0, 0, 50, 100);
    check_bounds(bu.detach()->bounds(), {35.5662f, 25, 64.4338f, 100});

    bu.moveTo (0, 50);
    bu.cubicTo(100, 0, 100, 100, 0, 50);
    check_bounds(bu.detach()->bounds(), {0, 35.5662f, 75, 64.4338f});
}

namespace {
constexpr GPixel gPix = 0xFFFF8800;
class PixShader : public GShader {
    class Ctx : public Context {
    public:
        void shadeRow(int, int, int count, GPixel row[]) override {
            for (int i = 0; i < count; ++i) {
                row[i] = gPix;
            }
        }
    };
public:
    bool isOpaque() override { return true; }
    std::unique_ptr<Context> makeContext(const GMatrix&) override {
        return std::make_unique<Ctx>();
    }
};
}

static bool nearly_equal_pix(GPixel a, GPixel b) {
    auto nearly_eq = [](int a, int b) {
        return std::abs(a - b) <= 1;
    };
    return nearly_eq(GPixel_GetA(a), GPixel_GetA(b))
        && nearly_eq(GPixel_GetR(a), GPixel_GetR(b))
        && nearly_eq(GPixel_GetG(a), GPixel_GetG(b))
        && nearly_eq(GPixel_GetB(a), GPixel_GetB(b));
}

static void test_modalpha(GTestStats* stats) {
    const float scale[] = { 0, 0x20/255.f, 0xC0/266.f, 1 };
    const GPixel expected[] = {
        0x00000000, 0x20201100, 0xB8B86200, 0xFFFF8800
    };
    for (size_t i = 0; i < std::size(scale); ++i) {
        auto sh = std::make_shared<PixShader>();
        auto sh2 = GShader::ModAlpha(sh, scale[i]);
        sh = nullptr;

        EXPECT_TRUE(stats, sh2->isOpaque() == (scale[i] == 1));

        auto ctx = sh2->makeContext(GMatrix());

        GPixel pix = 0x11223344;
        ctx->shadeRow(0, 0, 1, &pix);
        EXPECT_TRUE(stats, nearly_equal_pix(expected[i], pix));
    }
}