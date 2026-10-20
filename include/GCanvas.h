/*
 *  Copyright 2015 Mike Reed
 */

#ifndef GCanvas_DEFINED
#define GCanvas_DEFINED

#include "GMatrix.h"
#include "GPaint.h"
#include <string>

class GBitmap;
class GPath;
class GPoint;
class GRect;

class GCanvas {
public:
    virtual ~GCanvas() {}

    /**
     *  Save off a copy of the canvas state (CTM), to be later used if the balancing call to
     *  restore() is made. Calls to save/restore can be nested:
     *  save();
     *      save();
     *          concat(...);    // this modifies the CTM
     *          .. draw         // these are drawn with the modified CTM
     *      restore();          // now the CTM is as it was when the 2nd save() call was made
     *      ..
     *  restore();              // now the CTM is as it was when the 1st save() call was made
     */
    virtual void save() = 0;

    /**
     *  Copy the canvas state (CTM) that was record in the correspnding call to save() back into
     *  the canvas. It is an error to call restore() if there has been no previous call to save().
     */
    virtual void restore() = 0;

    /**
     *  Modifies the CTM by preconcatenating the specified matrix with the CTM. The canvas
     *  is constructed with an identity CTM.
     *
     *  CTM' = CTM * matrix
     */
    virtual void concat(const GMatrix& matrix) = 0;

    /**
     *  Fill the entire canvas with the specified color, using kSrc porter-duff mode.
     */
    virtual void clear(const GColor&) = 0;

    /**
     *  Draw a hairline from one point to the other, using the specified color and blendmode.
     *  If the paint's lineWidth is < 0, draw a hairline (ala PA1), else construct a stroked line,
     *  and draw it as a polygon.
     */
    virtual void drawLine(GPoint, GPoint, const GPaint&) = 0;

    /**
     *  Fill the rectangle with the color, using the specified blendmode.
     *
     *  The affected pixels are those whose centers are "contained" inside the rectangle:
     *      e.g. contained == center > min_edge && center <= max_edge
     */
    virtual void drawRect(const GRect&, const GPaint&) = 0;

    /**
     *  Fill the convex polygon with the color and blendmode,
     *  following the same "containment" rule as rectangles.
     */
    virtual void drawConvexPolygon(const GPoint[], int count, const GPaint&) = 0;

    /**
     *  Fill the path with the paint, interpreting the path using winding-fill (non-zero winding).
    */
    virtual void drawPath(const GPath&, const GPaint&) = 0;

    // Helpers

    void translate(float x, float y) {
        this->concat(GMatrix::Translate(x, y));
    }

    void translate(GVector v) {
        this->translate(v.x, v.y);
    }

    void scale(float x, float y) {
        this->concat(GMatrix::Scale(x, y));
    }

    void rotate(float radians) {
        this->concat(GMatrix::Rotate(radians));
    }

    // Compatibility helper for PA1 callers (before we had GPaint)

    void fillRect(const GRect& rect, const GColor& color) {
        this->drawRect(rect, GPaint(color));
    }

    void hairLine(GPoint a, GPoint b, const GColor& color) {
        GPaint paint(color);
        paint.setHairline();
        this->drawLine(a, b, paint);
    }

    void drawPath(const std::shared_ptr<GPath>& path, const GPaint& paint) {
        if (path) {
            this->drawPath(*path, paint);
        }
    }

    void drawOval(const GRect& oval, const GPaint&);
    void drawCircle(GPoint center, float radius, const GPaint&);
};

/**
 *  Implemnt this, returning an instance of your subclass of GCanvas.
 */
std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& bitmap);

/**
 *  Implement this, drawing into the provided canvas, and returning the title of your artwork.
 */
std::string GDrawSomething(GCanvas* canvas, GISize dim);

#endif
