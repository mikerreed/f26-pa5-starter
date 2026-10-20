#ifndef GPathBuilder_DEFINED
#define GPathBuilder_DEFINED

#include "GPoint.h"
#include "GRect.h"
#include "GPath.h"

class GPathBuilder {
public:
    GPathBuilder() {}

    /**
     *  Erase any previously added points/verbs, restoring the path to its initial empty state.
     */
    void reset();

    /**
     *  Start a new contour at the specified coordinate.
     */
    void moveTo(GPoint);
    void moveTo(float x, float y) { this->moveTo({x, y}); }

    /**
     *  Connect the previous point (either from a moveTo or lineTo) with a line segment to
     *  the specified coordinate.
     *  Note: it is illegal to call lineTo() as the first verb in a contour!
     */
    void lineTo(GPoint);
    void lineTo(float x, float y) { this->lineTo({x, y}); }

    /**
     *  Connect the previous point with a quadratic bezier to the specified coordinates.
     *  Note: it is illegal to call quadTo() as the first verb in a contour!
     */
    void quadTo(GPoint, GPoint);
    void quadTo(float x1, float y1, float x2, float y2) {
        this->quadTo({x1, y1}, {x2, y2});
    }

    /**
     *  Connect the previous point with a cubic bezier to the specified coordinates.
     *  Note: it is illegal to call cubicTo() as the first verb in a contour!
     */
    void cubicTo(GPoint, GPoint, GPoint);
    void cubicTo(float x1, float y1, float x2, float y2, float x3, float y3) {
        this->cubicTo({x1, y1}, {x2, y2}, {x3, y3});
    }

    /**
     *  Append a new contour to this path, made up of the 4 points of the specified rect,
     *  in the specified direction.
     *
     *  In either direction the contour must begin at the top-left corner of the rect.
     */
    void addRect(const GRect&, GPathDirection = GPathDirection::kCW);

    /**
     *  Append a new contour to this path with the specified polygon.
     *  Calling this is equivalent to calling moveTo(pts[0]), lineTo(pts[1..count-1]).
     */
    void addPolygon(const GPoint pts[], int count);

    /**
     *  Append a new contour respecting the Direction. The contour should be an approximate
     *  oval (4 cubics will suffice) bounded by the rectangle r.
     */
    void addOval(const GRect& r, GPathDirection = GPathDirection::kCW);

    void addCircle(GPoint center, float radius, GPathDirection dir = GPathDirection::kCW) {
        this->addOval({center.x - radius, center.y - radius, center.x + radius, center.y + radius}, dir);
    }

    void transformInPlace(const GMatrix&);

    /**
     * Return a GPath from the contents of this builder,
     * and then reset() the builder back to its empty state.
     */
    std::shared_ptr<GPath> detach();

private:
    std::vector<GPoint>    fPts;
    std::vector<GPathVerb> fVbs;
};

#endif

