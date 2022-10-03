#ifndef RAY_H
#define RAY_H

#include "vec3.h"

class Ray
{
public:
    Ray() {}
    Ray(const vec3& origin, const vec3& direction) : origin(origin), direction(direction)
    {}
    ~Ray() {}

    vec3 at(float t) const {
        return origin + t*direction;
    }

    vec3 origin;
    vec3 direction;
};

inline std::ostream& operator<<(std::ostream &out, const Ray &r) {
    return out << "Ray(" << r.origin << "; " << r.direction << ')';
}

#endif