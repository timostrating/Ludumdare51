#ifndef COMMON_H
#define COMMON_H

#include <cmath>
#include <limits>
#include <memory>
#include <iostream>


// most used headers

#include "ray.h"
#include "vec3.h"

// Usings

using std::shared_ptr;
using std::make_shared;
using std::sqrt;
using std::fabs;

namespace MATH
{
    const float PI = 3.1415926535f;
    const float GOLDEN_RATIO = (sqrt(5.0f) + 1.0f) / 2.0f;


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////// REMAP
    inline float remap (float value, float from1, float to1, float from2, float to2) {
        return (value - from1) / (to1 - from1) * (to2 - from2) + from2;
    }

    inline float degreesToRadians(float deg) {
        return deg * PI / 180;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////// RANDOM
    inline float random() {
        return drand48();
    }
    inline float random(const float min, const float max) {
        return remap(random(), 0, 1, min, max);
    }

    inline vec3 randomVec3() { 
        return vec3(random(), random(), random());
    }
    inline vec3 randomVec3(const float min, const float max) { 
        return vec3(random(min, max), random(min, max), random(min, max)); 
    }

    inline vec3 randomInUnitSphere() {
        while (true) {
            auto p = vec3(random(-1,1), random(-1,1), random(-1,1));
            if (p.length_squared() >= 1) continue;
            return p;
        }
    }

    inline vec3 randomUnitVector() {
        return unitVector(randomInUnitSphere());
    }
}

#endif