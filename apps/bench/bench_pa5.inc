/**
 *  Copyright 2020 Mike Reed
 */

class PathBench2 : public GBenchmark {
    const GISize    fSize;
    const float     fScale;
    const char*     fName;

    std::shared_ptr<GPath> fPath;
    GPaint          fPaint;

public:
    PathBench2(GISize size, float scale, const char* name) : fSize(size), fScale(scale), fName(name) {
        GRandom rand;
        auto rand_pt = [&]() {
            float x = rand.nextF() * fScale;
            float y = rand.nextF() * fScale;
            return GPoint{x, y};
        };

        GPathBuilder bu;
        bu.moveTo(0, 0);
        for (int i = 0; i < 100; ++i) {
            auto p0 = rand_pt();
            bu.lineTo(p0);
            auto p1 = rand_pt();
            auto p2 = rand_pt();
            bu.quadTo(p1, p2);
            auto p3 = rand_pt();
            auto p4 = rand_pt();
            auto p5 = rand_pt();
            bu.cubicTo(p3, p4, p5);
        }
        fPath = bu.detach();
    }

    const char* name() const override { return fName; }
    GISize size() const override { return fSize; }
    void draw(GCanvas* canvas) override {
        const int N = 10;
        canvas->clear({0,0,0,0});
        for (int i = 0; i < N; ++i) {
            canvas->drawPath(*fPath, fPaint);
        }
    }
};

/////////////////

namespace {
class TmpShader : public GShader {
    class Ctx : public Context {
    public:
        void shadeRow(int, int, int count, GPixel row[]) override {
            for (int i = 0; i < count; ++i) {
                unsigned r = i, g = (int)std::sqrt(i*i), b = i*i*i;
                row[i] = GPixel_PackARGB(0xFF, r & 0xFF, g & 0xFF, b & 0xFF);
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

class ModAlphaBench : public GBenchmark {
    const char*                       fName;
    std::shared_ptr<GShader>          fShader;
    std::unique_ptr<GShader::Context> fCtx;
public:
    ModAlphaBench(float scale, const char name[]) : fName(name) {
        fShader = GShader::ModAlpha(std::make_shared<TmpShader>(), scale);
        fCtx = fShader->makeContext(GMatrix());
    }
    const char* name() const override { return fName; }
    GISize size() const override { return {1, 1}; } // unimportant
    void draw(GCanvas* canvas) override {
        constexpr int LOOP = 5000;
        for (int i = 0; i < LOOP; ++i) {
            constexpr int WIDTH = 256;
            GPixel dst[WIDTH];
            fCtx->shadeRow(0, 0, std::size(dst), dst);
        }
    }
};
