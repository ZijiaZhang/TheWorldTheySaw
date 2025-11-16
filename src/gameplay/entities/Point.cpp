//
//  Point.cpp
//  soldier
//
//  Created by Haofeng Winter Feng on 2021-02-11.
//

#include "Point.hpp"

Points::Points() {
    // first no point
    pts = 0;
}

void Points::addPoint() {
    auto& p = pts;
    p = pts;
    pts = pts + 1;
}

int Points::getPoint() {
    auto temp = pts;
    return temp;
}
