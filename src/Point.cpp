//
//  Point.cpp
//  salmon
//
//  Created by Haofeng Winter Feng on 2021-02-11.
//

#include "Point.hpp"

Points::Points() {
    pts = 0;
}

void Points::addPoint() {
    pts++;
}

int Points::getPoint() {
    return pts;
}
