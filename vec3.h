#ifndef VEC3_H
#define VEC3_H
//==============================================================================================
// Originally written in 2016 by Peter Shirley <ptrshrl@gmail.com>
// https://github.com/RayTracing/raytracing.github.io/blob/master/src/common/vec3.h
// CC0 Public Domain
//==============================================================================================

struct vec3 {
public:
    union { float x, r; };
    union { float y, g; };
    union { float z, b; };

    vec3() : x(0), y(0), z(0) {}
    vec3(const vec3& v) : x(v.x), y(v.y), z(v.z) {}
    vec3(float e) : x(e), y(e), z(e) {}
    vec3(float e0, float e1, float e2) : x(e0), y(e1),z(e2) {}

    vec3 operator-() const { return vec3(-x, -y, -z); }
    // float operator[](int i) const { return e[i]; } // ff koekeloeren hoe glm dit doet als dit nodig is
    // float& operator[](int i) { return e[i]; }

    vec3& operator+=(const vec3 &v) {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }

    vec3& operator*=(const float t) {
        x *= t;
        y *= t;
        z *= t;
        return *this;
    }

    vec3& operator/=(const float t) {
        return *this *= 1/t;
    }

    float length() const {
        return sqrt(length_squared());
    }

    float length_squared() const {
        return x*x + y*y + z*z;
    }

    bool near_zero() const {
        // Return true if the vector is close to zero in all dimensions.
        const auto s = 1e-8;
        return (fabs(x) < s) && (fabs(y) < s) && (fabs(z) < s);
    }
};

using color = vec3;


// vec3 Utility Functions

inline std::ostream& operator<<(std::ostream &out, const vec3 &v) {
    return out << v.x << ' ' << v.y << ' ' << v.z;
}

inline vec3 operator+(const vec3 &u, const vec3 &v) {
    return vec3(u.x + v.x, u.y + v.y, u.z + v.z);
}

inline vec3 operator-(const vec3 &u, const vec3 &v) {
    return vec3(u.x - v.x, u.y - v.y, u.z - v.z);
}

inline vec3 operator*(const vec3 &u, const vec3 &v) {
    return vec3(u.x * v.x, u.y * v.y, u.z * v.z);
}

inline vec3 operator*(float t, const vec3 &v) {
    return vec3(t*v.x, t*v.y, t*v.z);
}

inline vec3 operator*(const vec3 &v, float t) {
    return t * v;
}

inline vec3 operator/(vec3 v, float t) {
    return (1/t) * v;
}

inline float dot(const vec3 &u, const vec3 &v) {
    return u.x * v.x
         + u.y * v.y
         + u.z * v.z;
}

inline vec3 cross(const vec3 &u, const vec3 &v) {
    return vec3(u.y * v.z - u.z * v.y,
                u.z * v.x - u.x * v.z,
                u.x * v.y - u.y * v.x);
}

inline vec3 reflect(const vec3& v, const vec3& n) {
    return v - 2*dot(v,n)*n;
}

inline vec3 refract(const vec3& uv, const vec3& n, double etai_over_etat) {
    auto cos_theta = fmin(dot(-uv, n), 1.0);
    vec3 r_out_perp =  etai_over_etat * (uv + cos_theta*n);
    vec3 r_out_parallel = -sqrt(fabs(1.0 - r_out_perp.length_squared())) * n;
    return r_out_perp + r_out_parallel;
}

inline vec3 unitVector(vec3 v) {
    return v / v.length();
}
#endif