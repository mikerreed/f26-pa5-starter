/*
 *  Copyright 2015 Mike Reed
 */

#ifndef GShader_DEFINED
#define GShader_DEFINED

#include <memory>
#include "GColor.h"
#include "GPixel.h"
#include "GPoint.h"

class GBitmap;
class GMatrix;

enum class GTileMode {
    kClamp,
    kRepeat,
    kMirror,
};

/**
 *  GShaders create colors to fill whatever geometry is being drawn to a GCanvas.
 */
class GShader : public std::enable_shared_from_this<GShader> {
public:
    virtual ~GShader() {}

    // Return true iff all of the GPixels that may be produced by this shader will be opaque.
    virtual bool isOpaque() = 0;

    class Context {
    public:
        virtual ~Context() {}

        /**
         *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], store the
         *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
         *  can hold at least [count] entries.
         */
        virtual void shadeRow(int x, int y, int count, GPixel row[]) = 0;
    };

    /*
     *  Given the CTM, return a shader context to compute src-pixel values,
     *  or on failure returns nullptr.
     */
    virtual std::unique_ptr<Context> makeContext(const GMatrix& ctm) = 0;

    /**
     *  Return a subclass of GShader that draws the specified bitmap and the local matrix.
     *  Returns null if the subclass can not be created.
     */
    static std::shared_ptr<GShader> Bitmap(const GBitmap&, const GMatrix& localMatrix,
                                           GTileMode = GTileMode::kClamp);

    /**
     *  Return a subclass of GShader that draws the specified gradient of [count] colors between
     *  the two points. Color[0] corresponds to p0, and Color[count-1] corresponds to p1, and all
     *  intermediate colors are evenly spaced between.
     *
     *  The input colors are GColors, and therefore unpremul. The output colors (in shadeRow)
     *  are GPixel, and therefore premul. The gradient has to interpolate between pairs of GColors
     *  before "pre" multiplying them into GPixels.
     *
     *  If count < 1, this should return nullptr, but count == 1 is legal
     */
    static std::shared_ptr<GShader> LinearGradient(GPoint p0, GPoint p1, const GColor[], int count,
                                                   GTileMode = GTileMode::kClamp);

    /**
     * Helper to handle the 2-color case.
     */
    static std::shared_ptr<GShader> LinearGradient(GPoint p0, GPoint p1,
                                                   const GColor& c0, const GColor& c1,
                                                   GTileMode tm = GTileMode::kClamp) {
        const GColor colors[] = { c0, c1 };
        return LinearGradient(p0, p1, colors, 2, tm);
    }

    /*
     * Wraps another shader, and modulates the alpha of the pixels it returns. Since the output
     * (from shadeRow) is in premul form, this has the effect of scaling all 4 channels.
     */
    static std::shared_ptr<GShader> ModAlpha(std::shared_ptr<GShader> base, float alphaScale);
};

#endif
