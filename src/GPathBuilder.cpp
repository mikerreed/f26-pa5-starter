/*
 *  Copyright 2025 Mike Reed
 */

#include "../include/GPathBuilder.h"
#include "../include/GMatrix.h"

void GPathBuilder::reset() {
    fPts.clear();
    fVbs.clear();
}

void GPathBuilder::moveTo(GPoint p) {
    fPts.push_back(p);
    fVbs.push_back(GPathVerb::kMove);
}

void GPathBuilder::lineTo(GPoint p) {
    assert(fVbs.size() > 0);
    fPts.push_back(p);
    fVbs.push_back(GPathVerb::kLine);
}

void GPathBuilder::quadTo(GPoint p1, GPoint p2) {
    assert(fVbs.size() > 0);
    fPts.push_back(p1);
    fPts.push_back(p2);
    fVbs.push_back(GPathVerb::kQuad);
}

void GPathBuilder::cubicTo(GPoint p1, GPoint p2, GPoint p3) {
    assert(fVbs.size() > 0);
    fPts.push_back(p1);
    fPts.push_back(p2);
    fPts.push_back(p3);
    fVbs.push_back(GPathVerb::kCubic);
}

void GPathBuilder::transformInPlace(const GMatrix& m) {
    m.mapPoints(fPts.data(), fPts.size());
}

std::shared_ptr<GPath> GPathBuilder::detach() {
    auto path = std::make_shared<GPath>(std::move(fPts), std::move(fVbs));
    this->reset();
    return path;
}
