/*
 *  Copyright 2018 Mike Reed
 */

#include "../include/GCanvas.h"
#include "../include/GPathBuilder.h"

void GCanvas::drawRect(const GRect& r, const GPaint& paint) {
    GPathBuilder bu;
    bu.addRect(r);
    this->drawPath(*bu.detach(), paint);
}

void GCanvas::drawConvexPolygon(const GPoint pts[], int count, const GPaint& paint) {
    GPathBuilder bu;
    bu.addPolygon(pts, count);
    this->drawPath(*bu.detach(), paint);
}

void GCanvas::drawOval(const GRect& oval, const GPaint& paint) {
    GPathBuilder bu;
    bu.addOval(oval, GPathDirection::kCW);
    this->drawPath(bu.detach(), paint);
}

void GCanvas::drawCircle(GPoint p, float radius, const GPaint& paint) {
    this->drawOval({p.x - radius, p.y - radius, p.x + radius, p.y + radius}, paint);
}