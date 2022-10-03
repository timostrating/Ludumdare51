#ifndef CAMERA_H
#define CAMERA_H

#include "common.h"

class Camera {
public:      
    Camera(vec3 lookfrom, vec3 lookat, vec3 vup) : lookfrom(lookfrom), lookat(lookat), vup(vup) { update(); }
    ~Camera() {}

    void update() {
        const auto aspectRatio = 1.0; 

        auto viewportHeight = 2.0;
        auto viewportWidth = aspectRatio * viewportHeight;
        
        auto focalLength = 1.0;

        auto w = unitVector(lookfrom - lookat);
        auto u = unitVector(cross(vup, w));
        auto v = cross(w, u);

        origin = lookfrom;
        horizontal = viewportWidth * u;
        vertical = viewportHeight * v;
        lower_left_corner = origin - horizontal/2 - vertical/2 - w;
    }

    void setPosition(vec3 pos) { 
        lookfrom = pos; 
        update();
    }

    void setLookat(vec3 pos) { 
        lookat = pos; 
        update();
    }

    Ray getRay(float s, float t) const {
        return Ray(origin, lower_left_corner + s*horizontal + t*vertical - origin);
    }

private:
    vec3 lookfrom;
    vec3 lookat;
    vec3 vup;

    vec3 origin;
    vec3 lower_left_corner;
    vec3 horizontal;
    vec3 vertical;
};

#endif