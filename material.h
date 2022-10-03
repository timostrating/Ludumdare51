#ifndef MATERIAL_H
#define MATERIAL_H

#include "common.h"
#include "hittable.h"

class Material {
    public:
        virtual vec3 emitted() const { return vec3(0,0,0); }
        virtual bool scatter(const Ray& r_in, const hit& rec, vec3& outColor, Ray& scattered) const = 0;
};

inline std::ostream& operator<<(std::ostream &out, const Material &r) {
    return out << "Material";
}


class Unlit : public Material {
    public:
        Unlit(const color& a) : albedo(a) {}

        vec3 emitted() const override {
            return albedo;
        }

        bool scatter(const Ray& r_in, const hit& rec, vec3& outColor, Ray& scattered) const override {
            outColor = albedo;
            return true;
        }

    public:
        color albedo;
};

// ---------------------------------------------------------------- Lambertian

class Lambertian : public Material {
    public:
        Lambertian(const color& a) : albedo(a) {}

        bool scatter(const Ray& r_in, const hit& rec, vec3& outColor, Ray& scattered) const override {
            auto scatter_direction = rec.normal + MATH::randomUnitVector();

            // Catch degenerate scatter direction
            if (scatter_direction.near_zero())
                scatter_direction = rec.normal;

            scattered = Ray(rec.point, scatter_direction);
            outColor = albedo;
            return true;
        }

    public:
        color albedo;
};

// ---------------------------------------------------------------- Metal

class Metal : public Material {
    public:
        Metal(const vec3& a, float f) : albedo(a), fuzz(f < 1 ? f : 1) {}

        bool scatter(const Ray& r_in, const hit& rec, vec3& outColor, Ray& scattered) const override {
            vec3 reflected = reflect(unitVector(r_in.direction), rec.normal);
            scattered = Ray(rec.point, reflected + fuzz*MATH::randomInUnitSphere());
            outColor = albedo;
            return (dot(scattered.direction, rec.normal) > 0);
        }

    public:
        vec3 albedo;
        float fuzz;
};

// ---------------------------------------------------------------- Light

class Light : public Material {
    public:
        Light(const vec3& a) : lightColor(a) {}

        vec3 emitted() const override {
            return lightColor;
        }

        bool scatter(const Ray& r_in, const hit& rec, vec3& outColor, Ray& scattered) const override {
            return false;
        }

    public:
        vec3 lightColor;
};

class Dielectric : public Material {
    public:
        Dielectric(double index_of_refraction) : ir(index_of_refraction) {}

        virtual bool scatter( const Ray& r_in, const hit& rec, vec3& attenuation, Ray& scattered ) const override {
            attenuation = color(1.0, 1.0, 1.0);
            // float refraction_ratio = rec.front_face ? (1.0/ir) : ir;
            float refraction_ratio = (1.0/ir);

            vec3 unit_direction = unitVector(r_in.direction);
            double cos_theta = fmin(dot(-unit_direction, rec.normal), 1.0);
            double sin_theta = sqrt(1.0 - cos_theta*cos_theta);

            bool cannot_refract = refraction_ratio * sin_theta > 1.0;
            vec3 direction;
            if (cannot_refract || reflectance(cos_theta, refraction_ratio) > MATH::random())
                direction = reflect(unit_direction, rec.normal);
            else
                direction = refract(unit_direction, rec.normal, refraction_ratio);

            scattered = Ray(rec.point, direction);
            return true;
        }

    public:
        double ir; // Index of Refraction

    private:
        static double reflectance(double cosine, double ref_idx) {
            // Use Schlick's approximation for reflectance.
            auto r0 = (1-ref_idx) / (1+ref_idx);
            r0 = r0*r0;
            return r0 + (1-r0)*pow((1 - cosine),5);
        }
};

class Special : public Material {
    public:
        Special(const vec3& a) : lightColor(a) {}

        vec3 emitted() const override {
            return lightColor;
        }

        bool scatter(const Ray& r_in, const hit& rec, vec3& outColor, Ray& scattered) const override {
            auto scatter_direction = rec.normal + MATH::randomUnitVector();

            // Catch degenerate scatter direction
            if (scatter_direction.near_zero())
                scatter_direction = rec.normal;

            scattered = Ray(rec.point, scatter_direction);
            outColor = lightColor;
            return true;
        }

    public:
        vec3 lightColor;
};

#endif