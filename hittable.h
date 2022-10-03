#ifndef HITTABLE_H
#define HITTABLE_H

#include "common.h"

class Material;

struct hit {
    vec3 point;
    vec3 normal;
    shared_ptr<Material> mat_ptr;
    float t;
    bool specialObject = false;
};

inline std::ostream& operator<<(std::ostream &out, const hit &h) {
    return out << "hit(" << h.point << "," << h.normal << "," << h.t << "," << h.specialObject << ")";
}

class Hittable {
    public:
        virtual bool trace(const Ray& r, float t_min, float t_max, hit& hit) const = 0;
};


#include <memory>
#include <vector>

using std::shared_ptr;
using std::make_shared;

class HittableList : public Hittable 
{
    std::vector<shared_ptr<Hittable>> objects; // not intended to be public
    
public:
    HittableList() {}
    HittableList(shared_ptr<Hittable> object) { add(object); }

    void clear() { objects.clear(); }
    void add(shared_ptr<Hittable> object) { objects.push_back(object); }

    virtual bool trace(const Ray& r, float t_min, float t_max, hit& rec) const {
        hit temp_hit;
        bool hit_anything = false;
        auto closest_so_far = 9999999999.0f;

        for (const auto& object : objects) {
            if (object->trace(r, t_min, closest_so_far, temp_hit)) {
                hit_anything = true;
                closest_so_far = temp_hit.t;
                rec = temp_hit;
            }
        }

        return hit_anything;
    }

};

class Sphere : public Hittable
{
    vec3 center;
    float radius;
    shared_ptr<Material> mat_ptr;

public:
    Sphere() {}
    Sphere(vec3 cen, float r, shared_ptr<Material> m) : center(cen), radius(r), mat_ptr(m) {};


    virtual bool trace(const Ray& r, float t_min, float t_max, hit& rec) const {
        vec3 oc = r.origin - center;
        auto a = r.direction.length_squared();
        auto half_b = dot(oc, r.direction);
        auto c = oc.length_squared() - radius*radius;

        auto discriminant = half_b*half_b - a*c;
        if (discriminant < 0) return false;
        auto sqrtd = sqrt(discriminant);

        // Find the nearest root that lies in the acceptable range.
        auto root = (-half_b - sqrtd) / a;
        if (root < t_min || t_max < root) {
            root = (-half_b + sqrtd) / a;
            if (root < t_min || t_max < root)
                return false;
        }

        rec.t = root;
        rec.point = r.at(rec.t);
        rec.normal = unitVector((rec.point - center) / radius);
        rec.mat_ptr = mat_ptr;

        return true;
    }
};


#define EPSILON 0.000001

class Triangle : public Hittable 
{
    vec3 p0, p1, p2;
    shared_ptr<Material> mat_ptr;

public:
    Triangle(vec3 p0, vec3 p1, vec3 p2, shared_ptr<Material> m) : p0(p0), p1(p1), p2(p2), mat_ptr(m) {}

    virtual bool trace(const Ray& r, float t_min, float t_max, hit& rec) const {

        vec3 edge1, edge2;

        // the edges that share point_0
        edge1 = p1-p0;
        edge2 = p2-p0;

        vec3 pvec = cross(r.direction, edge2);     
        float determinant = dot(edge1, pvec);

        if (determinant > -EPSILON && determinant < EPSILON)
            return false;

        float inverse_determinant = 1.0 / determinant;

        // U and V are the barycentric UV coordinates on the triangle 
        vec3 tvec = r.origin - p0;
        float u = dot(tvec, pvec) * inverse_determinant;
        if (u < 0.0 || u > 1.0)
            return false;

        vec3 qvec = cross(tvec, edge1);
        float v = dot(r.direction, qvec) * inverse_determinant;
        if (v < 0.0 || u + v > 1.0)
            return false;

        // t is the distance from the ray origin to the triangle
        float t = dot(edge2, qvec) * inverse_determinant; 


        rec.t = t;
        rec.point = r.at(t);
        rec.normal = unitVector(cross(edge1, edge2));
        rec.mat_ptr = mat_ptr;

        // std::cout << "--------------------------------" << std::endl;
        if (t < t_min || t > t_max) {
            // std::cout << "this is triangle is ignored" << std::endl;
            return false;
        }

        // std::cout << t_min << " " << t_max << std::endl;
        // std::cout << rec.t << std::endl;
        // std::cout << rec.point << std::endl;
        // std::cout << rec.normal << std::endl;
        
        return true; 
    };
};

class Quad : public Hittable 
{
    Triangle a, b;

public:
    Quad(vec3 p0, vec3 p1, vec3 p2, vec3 p3, shared_ptr<Material> m) : a(p0,p1,p2,m), b(p2,p3,p0,m) {}
    // Quad(vec3 p0, vec3 rotation, float scale, shared_ptr<Material> m) : a(p0,p1,p2,m), b(p1,p2,p3,m) {}

    virtual bool trace(const Ray& r, float t_min, float t_max, hit& rec) const {

        if (a.trace(r, t_min, t_max, rec)) return true;
        if (b.trace(r, t_min, t_max, rec)) return true;

        return false; 
    };
};

class RectXY : public Hittable {
    vec3 pos;
    float w, h;
    shared_ptr<Material> mat_ptr;

public:
    RectXY(vec3 pos, float w, float h, shared_ptr<Material> m) : pos(pos), w(w), h(h), mat_ptr(m) {}

    virtual bool trace(const Ray& r, float t_min, float t_max, hit& rec) const {
        // P(t) = A + t*b          where P(t) is the ray    ... r.at(t)
        //                         A is ray start position  and  b is ray direction
        // P_z(t) = A_z + t*b_z    This is true for x y and z
        // pos.z = A_z + t*b_z     We know Z
        // pos.z  =  r.origin.z + t * r.direction.z
        // pos.z - r.origin.z  =  t * r.direction.z 
        // (pos.z - r.origin.z) / r.direction.z  =  t 
        float t = (pos.z - r.origin.z) / r.direction.z;

        float x = r.at(t).x;
        float y = r.at(t).y;
        
        if (pos.x + w < x || pos.x - w > x || pos.y + h < y || pos.y - h > y)
            return false;

        rec.t = t;
        rec.point = r.at(t);
        rec.normal = vec3(0,0,1);
        rec.mat_ptr = mat_ptr;

        return true;
    }
};


class BadEend : public HittableList  
{
    HittableList body;
    HittableList bekkie;

public:
    BadEend(shared_ptr<Material> m, shared_ptr<Material> m2)
    {
//////////////////////////////////////// GENERATED CODE START
body.add( make_shared<Triangle>(vec3(0.528428,-1.029138,0.015093),vec3(1.131490,-0.799040,0.015093),vec3(0.690974,0.355502,0.015093), m));
body.add( make_shared<Triangle>(vec3(1.131490,-0.799040,0.015093),vec3(0.528428,-1.029138,0.015093),vec3(0.548791,-1.024570,-0.149538), m));
body.add( make_shared<Triangle>(vec3(1.135908,-0.796847,-0.194219),vec3(0.548791,-1.024570,-0.149538),vec3(0.181312,-0.105612,-0.492830), m));
body.add( make_shared<Triangle>(vec3(0.810941,0.248472,-0.454867),vec3(0.181312,-0.105612,-0.492830),vec3(0.005545,0.355054,-0.336059), m));
body.add( make_shared<Triangle>(vec3(0.005545,0.355054,-0.336059),vec3(0.181312,-0.105612,-0.492830),vec3(0.087912,0.125404,0.015093), m));
body.add( make_shared<Triangle>(vec3(0.548791,-1.024570,-0.149538),vec3(0.087912,0.125404,0.015093),vec3(0.181312,-0.105612,-0.492830), m));
body.add( make_shared<Triangle>(vec3(0.528428,-1.029138,0.015093),vec3(0.087912,0.125404,0.015093),vec3(0.548791,-1.024570,-0.149538), m));
body.add( make_shared<Triangle>(vec3(0.690974,0.355502,0.015093),vec3(0.005545,0.355054,-0.336059),vec3(0.087912,0.125404,0.015093), m));
body.add( make_shared<Triangle>(vec3(0.005545,0.355054,-0.336059),vec3(0.690974,0.355502,0.015093),vec3(0.539749,0.624919,-0.332684), m));
body.add( make_shared<Triangle>(vec3(0.539749,0.624919,-0.332684),vec3(0.690974,0.355502,0.015093),vec3(0.810941,0.248472,-0.454867), m));
body.add( make_shared<Triangle>(vec3(0.810941,0.248472,-0.454867),vec3(0.690974,0.355502,0.015093),vec3(1.135908,-0.796847,-0.194219), m));
body.add( make_shared<Triangle>(vec3(0.690974,0.355502,0.015093),vec3(1.131490,-0.799040,0.015093),vec3(1.135908,-0.796847,-0.194219), m));
body.add( make_shared<Triangle>(vec3(0.197870,0.237193,-0.167597),vec3(0.987405,-0.750910,-0.804983),vec3(0.520474,0.871404,-0.935887), m));
body.add( make_shared<Triangle>(vec3(0.000000,0.739798,-0.619836),vec3(0.520474,0.871404,-0.935887),vec3(0.000000,1.214297,-1.055864), m));
body.add( make_shared<Triangle>(vec3(0.520474,0.871404,-0.935887),vec3(0.987405,-0.750910,-0.804983),vec3(0.304106,-0.251183,-1.636693), m));
body.add( make_shared<Triangle>(vec3(0.520474,0.871404,-0.935887),vec3(0.304106,-0.251183,-1.636693),vec3(0.000000,1.214297,-1.055864), m));
body.add( make_shared<Triangle>(vec3(0.052640,1.540373,-2.415132),vec3(0.000000,1.132595,-0.724539),vec3(0.290171,0.574738,-0.772875), m));
body.add( make_shared<Triangle>(vec3(0.206976,-0.883530,-2.054593),vec3(0.000000,-0.883530,-2.116803),vec3(0.000000,-0.449198,-2.364102), m));
body.add( make_shared<Triangle>(vec3(0.000000,-0.449198,-2.364102),vec3(0.000000,-0.025428,-2.504842),vec3(0.510330,0.024206,-2.361554), m));
body.add( make_shared<Triangle>(vec3(0.000000,-0.025428,-2.504842),vec3(0.000000,0.565457,-2.419547),vec3(0.273229,0.506053,-2.257170), m));
body.add( make_shared<Triangle>(vec3(0.314795,-0.883530,-1.911020),vec3(0.707702,-0.381498,-1.560577),vec3(0.307225,-0.883530,-1.736031), m));
body.add( make_shared<Triangle>(vec3(0.350594,0.685522,-1.946242),vec3(0.000000,0.830967,-1.876271),vec3(0.000000,0.520890,-1.422050), m));
body.add( make_shared<Triangle>(vec3(0.472005,0.437742,-1.581011),vec3(0.000000,0.520890,-1.422050),vec3(0.000000,-0.454159,-1.370782), m));
body.add( make_shared<Triangle>(vec3(0.206976,-0.883530,-1.589238),vec3(0.707702,-0.381498,-1.560577),vec3(0.000000,-0.454159,-1.370782), m));
body.add( make_shared<Triangle>(vec3(0.307225,-0.883530,-1.736031),vec3(0.707702,-0.381498,-1.560577),vec3(0.206976,-0.883530,-1.589238), m));
body.add( make_shared<Triangle>(vec3(0.314795,-0.883530,-1.911020),vec3(0.206976,-0.883530,-2.054593),vec3(0.578638,-0.444373,-2.186206), m));
body.add( make_shared<Triangle>(vec3(0.273229,0.506053,-2.257170),vec3(0.000000,0.565457,-2.419547),vec3(0.000000,0.830967,-1.876271), m));
body.add( make_shared<Triangle>(vec3(0.707702,-0.381498,-1.560577),vec3(0.510330,0.024206,-2.361554),vec3(0.578638,-0.444373,-2.186206), m));
body.add( make_shared<Triangle>(vec3(0.000000,-1.325317,-0.794878),vec3(-0.987405,-0.750910,-0.804983),vec3(-0.304106,-0.251183,-1.636693), m));
body.add( make_shared<Triangle>(vec3(0.217468,-0.557775,-0.262964),vec3(0.000000,-1.325317,-0.794878),vec3(0.987405,-0.750910,-0.804983), m));
body.add( make_shared<Triangle>(vec3(0.000000,-1.325317,-0.794878),vec3(0.217468,-0.557775,-0.262964),vec3(0.000000,-0.621679,-0.136697), m));
body.add( make_shared<Triangle>(vec3(0.217468,-0.557775,-0.262964),vec3(0.197870,0.237193,-0.167597),vec3(0.000000,-0.621679,-0.136697), m));
body.add( make_shared<Triangle>(vec3(0.000000,-0.621679,-0.136697),vec3(0.197870,0.237193,-0.167597),vec3(0.000000,0.237193,-0.152152), m));
body.add( make_shared<Triangle>(vec3(0.197870,0.237193,-0.167597),vec3(0.000000,0.739798,-0.619836),vec3(0.000000,0.237193,-0.152152), m));
body.add( make_shared<Triangle>(vec3(0.472005,0.437742,-1.581011),vec3(0.510330,0.024206,-2.361554),vec3(0.273229,0.506053,-2.257170), m));
body.add( make_shared<Triangle>(vec3(0.290171,0.574738,-0.772875),vec3(-0.052640,1.540373,-2.415132),vec3(0.052640,1.540373,-2.415132), m));
body.add( make_shared<Triangle>(vec3(-0.690974,0.355502,0.015093),vec3(-1.131490,-0.799040,0.015093),vec3(-0.528428,-1.029138,0.015093), m));
body.add( make_shared<Triangle>(vec3(-0.548791,-1.024570,-0.149538),vec3(-0.528428,-1.029138,0.015093),vec3(-1.131490,-0.799040,0.015093), m));
body.add( make_shared<Triangle>(vec3(-0.181312,-0.105612,-0.492830),vec3(-0.548791,-1.024570,-0.149538),vec3(-1.135908,-0.796847,-0.194219), m));
body.add( make_shared<Triangle>(vec3(-0.005545,0.355054,-0.336059),vec3(-0.181312,-0.105612,-0.492830),vec3(-0.810941,0.248472,-0.454867), m));
body.add( make_shared<Triangle>(vec3(-0.005545,0.355054,-0.336059),vec3(-0.087912,0.125404,0.015093),vec3(-0.181312,-0.105612,-0.492830), m));
body.add( make_shared<Triangle>(vec3(-0.548791,-1.024570,-0.149538),vec3(-0.181312,-0.105612,-0.492830),vec3(-0.087912,0.125404,0.015093), m));
body.add( make_shared<Triangle>(vec3(-0.528428,-1.029138,0.015093),vec3(-0.548791,-1.024570,-0.149538),vec3(-0.087912,0.125404,0.015093), m));
body.add( make_shared<Triangle>(vec3(-0.690974,0.355502,0.015093),vec3(-0.087912,0.125404,0.015093),vec3(-0.005545,0.355054,-0.336059), m));
body.add( make_shared<Triangle>(vec3(-0.005545,0.355054,-0.336059),vec3(-0.539749,0.624919,-0.332684),vec3(-0.690974,0.355502,0.015093), m));
body.add( make_shared<Triangle>(vec3(-0.539749,0.624919,-0.332684),vec3(-0.810941,0.248472,-0.454867),vec3(-0.690974,0.355502,0.015093), m));
body.add( make_shared<Triangle>(vec3(-0.810941,0.248472,-0.454867),vec3(-1.135908,-0.796847,-0.194219),vec3(-0.690974,0.355502,0.015093), m));
body.add( make_shared<Triangle>(vec3(-0.690974,0.355502,0.015093),vec3(-1.135908,-0.796847,-0.194219),vec3(-1.131490,-0.799040,0.015093), m));
body.add( make_shared<Triangle>(vec3(-0.197870,0.237193,-0.167597),vec3(-0.987405,-0.750910,-0.804983),vec3(-0.217468,-0.557775,-0.262964), m));
body.add( make_shared<Triangle>(vec3(0.000000,0.739798,-0.619836),vec3(-0.520474,0.871404,-0.935887),vec3(-0.197870,0.237193,-0.167597), m));
body.add( make_shared<Triangle>(vec3(-0.520474,0.871404,-0.935887),vec3(-0.304106,-0.251183,-1.636693),vec3(-0.987405,-0.750910,-0.804983), m));
body.add( make_shared<Triangle>(vec3(-0.520474,0.871404,-0.935887),vec3(0.000000,1.214297,-1.055864),vec3(-0.304106,-0.251183,-1.636693), m));
body.add( make_shared<Triangle>(vec3(0.000000,1.132595,-0.724539),vec3(-0.052640,1.540373,-2.415132),vec3(-0.290171,0.574738,-0.772875), m));
body.add( make_shared<Triangle>(vec3(0.000000,-0.449198,-2.364102),vec3(0.000000,-0.883530,-2.116803),vec3(-0.206976,-0.883530,-2.054593), m));
body.add( make_shared<Triangle>(vec3(0.000000,-0.449198,-2.364102),vec3(-0.578638,-0.444373,-2.186206),vec3(-0.510330,0.024206,-2.361554), m));
body.add( make_shared<Triangle>(vec3(0.000000,-0.025428,-2.504842),vec3(-0.510330,0.024206,-2.361554),vec3(-0.273229,0.506053,-2.257170), m));
body.add( make_shared<Triangle>(vec3(-0.314795,-0.883530,-1.911020),vec3(-0.707702,-0.381498,-1.560577),vec3(-0.578638,-0.444373,-2.186206), m));
body.add( make_shared<Triangle>(vec3(0.000000,0.520890,-1.422050),vec3(0.000000,0.830967,-1.876271),vec3(-0.350594,0.685522,-1.946242), m));
body.add( make_shared<Triangle>(vec3(0.000000,-0.454159,-1.370782),vec3(0.000000,0.520890,-1.422050),vec3(-0.472005,0.437742,-1.581011), m));
body.add( make_shared<Triangle>(vec3(-0.206976,-0.883530,-1.589238),vec3(0.000000,-0.883530,-1.503144),vec3(0.000000,-0.454159,-1.370782), m));
body.add( make_shared<Triangle>(vec3(-0.307225,-0.883530,-1.736031),vec3(-0.206976,-0.883530,-1.589238),vec3(-0.707702,-0.381498,-1.560577), m));
body.add( make_shared<Triangle>(vec3(-0.314795,-0.883530,-1.911020),vec3(-0.206976,-0.883530,-2.054593),vec3(-0.578638,-0.444373,-2.186206), m));
body.add( make_shared<Triangle>(vec3(0.000000,0.830967,-1.876271),vec3(0.000000,0.565457,-2.419547),vec3(-0.273229,0.506053,-2.257170), m));
body.add( make_shared<Triangle>(vec3(-0.510330,0.024206,-2.361554),vec3(-0.578638,-0.444373,-2.186206),vec3(-0.707702,-0.381498,-1.560577), m));
body.add( make_shared<Triangle>(vec3(0.987405,-0.750910,-0.804983),vec3(0.000000,-1.325317,-0.794878),vec3(0.304106,-0.251183,-1.636693), m));
body.add( make_shared<Triangle>(vec3(-0.217468,-0.557775,-0.262964),vec3(-0.987405,-0.750910,-0.804983),vec3(0.000000,-1.325317,-0.794878), m));
body.add( make_shared<Triangle>(vec3(0.000000,-1.325317,-0.794878),vec3(0.000000,-0.621679,-0.136697),vec3(-0.217468,-0.557775,-0.262964), m));
body.add( make_shared<Triangle>(vec3(-0.217468,-0.557775,-0.262964),vec3(0.000000,-0.621679,-0.136697),vec3(-0.197870,0.237193,-0.167597), m));
body.add( make_shared<Triangle>(vec3(0.000000,-0.621679,-0.136697),vec3(0.000000,0.237193,-0.152152),vec3(-0.197870,0.237193,-0.167597), m));
body.add( make_shared<Triangle>(vec3(-0.197870,0.237193,-0.167597),vec3(0.000000,0.237193,-0.152152),vec3(0.000000,0.739798,-0.619836), m));
body.add( make_shared<Triangle>(vec3(-0.273229,0.506053,-2.257170),vec3(-0.510330,0.024206,-2.361554),vec3(-0.472005,0.437742,-1.581011), m));
body.add( make_shared<Triangle>(vec3(0.290171,0.574738,-0.772875),vec3(0.000000,1.132595,-0.724539),vec3(-0.290171,0.574738,-0.772875), m));
body.add( make_shared<Triangle>(vec3(0.000000,1.631587,-2.353452),vec3(0.052640,1.540373,-2.415132),vec3(-0.052640,1.540373,-2.415132), m));
body.add( make_shared<Triangle>(vec3(0.304106,-0.251183,-1.636693),vec3(0.000000,-1.325317,-0.794878),vec3(-0.304106,-0.251183,-1.636693), m));
body.add( make_shared<Triangle>(vec3(0.304106,-0.251183,-1.636693),vec3(-0.304106,-0.251183,-1.636693),vec3(0.000000,1.214297,-1.055864), m));
body.add( make_shared<Triangle>(vec3(0.528428,-1.029138,0.015093),vec3(0.690974,0.355502,0.015093),vec3(0.087912,0.125404,0.015093), m));
body.add( make_shared<Triangle>(vec3(1.131490,-0.799040,0.015093),vec3(0.548791,-1.024570,-0.149538),vec3(1.135908,-0.796847,-0.194219), m));
body.add( make_shared<Triangle>(vec3(1.135908,-0.796847,-0.194219),vec3(0.181312,-0.105612,-0.492830),vec3(0.810941,0.248472,-0.454867), m));
body.add( make_shared<Triangle>(vec3(0.810941,0.248472,-0.454867),vec3(0.005545,0.355054,-0.336059),vec3(0.539749,0.624919,-0.332684), m));
body.add( make_shared<Triangle>(vec3(0.197870,0.237193,-0.167597),vec3(0.217468,-0.557775,-0.262964),vec3(0.987405,-0.750910,-0.804983), m));
body.add( make_shared<Triangle>(vec3(0.000000,0.739798,-0.619836),vec3(0.197870,0.237193,-0.167597),vec3(0.520474,0.871404,-0.935887), m));
body.add( make_shared<Triangle>(vec3(0.052640,1.540373,-2.415132),vec3(0.000000,1.631587,-2.353452),vec3(0.000000,1.132595,-0.724539), m));
body.add( make_shared<Triangle>(vec3(0.206976,-0.883530,-2.054593),vec3(0.000000,-0.449198,-2.364102),vec3(0.578638,-0.444373,-2.186206), m));
body.add( make_shared<Triangle>(vec3(0.000000,-0.449198,-2.364102),vec3(0.510330,0.024206,-2.361554),vec3(0.578638,-0.444373,-2.186206), m));
body.add( make_shared<Triangle>(vec3(0.000000,-0.025428,-2.504842),vec3(0.273229,0.506053,-2.257170),vec3(0.510330,0.024206,-2.361554), m));
body.add( make_shared<Triangle>(vec3(0.314795,-0.883530,-1.911020),vec3(0.578638,-0.444373,-2.186206),vec3(0.707702,-0.381498,-1.560577), m));
body.add( make_shared<Triangle>(vec3(0.350594,0.685522,-1.946242),vec3(0.000000,0.520890,-1.422050),vec3(0.472005,0.437742,-1.581011), m));
body.add( make_shared<Triangle>(vec3(0.472005,0.437742,-1.581011),vec3(0.000000,-0.454159,-1.370782),vec3(0.707702,-0.381498,-1.560577), m));
body.add( make_shared<Triangle>(vec3(0.206976,-0.883530,-1.589238),vec3(0.000000,-0.454159,-1.370782),vec3(0.000000,-0.883530,-1.503144), m));
body.add( make_shared<Triangle>(vec3(0.273229,0.506053,-2.257170),vec3(0.000000,0.830967,-1.876271),vec3(0.350594,0.685522,-1.946242), m));
body.add( make_shared<Triangle>(vec3(0.707702,-0.381498,-1.560577),vec3(0.510330,0.024206,-2.361554),vec3(0.472005,0.437742,-1.581011), m));
body.add( make_shared<Triangle>(vec3(0.472005,0.437742,-1.581011),vec3(0.273229,0.506053,-2.257170),vec3(0.350594,0.685522,-1.946242), m));
body.add( make_shared<Triangle>(vec3(0.290171,0.574738,-0.772875),vec3(-0.290171,0.574738,-0.772875),vec3(-0.052640,1.540373,-2.415132), m));
body.add( make_shared<Triangle>(vec3(-0.690974,0.355502,0.015093),vec3(-0.528428,-1.029138,0.015093),vec3(-0.087912,0.125404,0.015093), m));
body.add( make_shared<Triangle>(vec3(-0.548791,-1.024570,-0.149538),vec3(-1.131490,-0.799040,0.015093),vec3(-1.135908,-0.796847,-0.194219), m));
body.add( make_shared<Triangle>(vec3(-0.181312,-0.105612,-0.492830),vec3(-1.135908,-0.796847,-0.194219),vec3(-0.810941,0.248472,-0.454867), m));
body.add( make_shared<Triangle>(vec3(-0.005545,0.355054,-0.336059),vec3(-0.810941,0.248472,-0.454867),vec3(-0.539749,0.624919,-0.332684), m));
body.add( make_shared<Triangle>(vec3(-0.197870,0.237193,-0.167597),vec3(-0.520474,0.871404,-0.935887),vec3(-0.987405,-0.750910,-0.804983), m));
body.add( make_shared<Triangle>(vec3(0.000000,0.739798,-0.619836),vec3(0.000000,1.214297,-1.055864),vec3(-0.520474,0.871404,-0.935887), m));
body.add( make_shared<Triangle>(vec3(0.000000,1.132595,-0.724539),vec3(0.000000,1.631587,-2.353452),vec3(-0.052640,1.540373,-2.415132), m));
body.add( make_shared<Triangle>(vec3(0.000000,-0.449198,-2.364102),vec3(-0.206976,-0.883530,-2.054593),vec3(-0.578638,-0.444373,-2.186206), m));
body.add( make_shared<Triangle>(vec3(0.000000,-0.449198,-2.364102),vec3(-0.510330,0.024206,-2.361554),vec3(0.000000,-0.025428,-2.504842), m));
body.add( make_shared<Triangle>(vec3(0.000000,-0.025428,-2.504842),vec3(-0.273229,0.506053,-2.257170),vec3(0.000000,0.565457,-2.419547), m));
body.add( make_shared<Triangle>(vec3(-0.314795,-0.883530,-1.911020),vec3(-0.307225,-0.883530,-1.736031),vec3(-0.707702,-0.381498,-1.560577), m));
body.add( make_shared<Triangle>(vec3(0.000000,0.520890,-1.422050),vec3(-0.350594,0.685522,-1.946242),vec3(-0.472005,0.437742,-1.581011), m));
body.add( make_shared<Triangle>(vec3(0.000000,-0.454159,-1.370782),vec3(-0.472005,0.437742,-1.581011),vec3(-0.707702,-0.381498,-1.560577), m));
body.add( make_shared<Triangle>(vec3(-0.206976,-0.883530,-1.589238),vec3(0.000000,-0.454159,-1.370782),vec3(-0.707702,-0.381498,-1.560577), m));
body.add( make_shared<Triangle>(vec3(0.000000,0.830967,-1.876271),vec3(-0.273229,0.506053,-2.257170),vec3(-0.350594,0.685522,-1.946242), m));
body.add( make_shared<Triangle>(vec3(-0.510330,0.024206,-2.361554),vec3(-0.707702,-0.381498,-1.560577),vec3(-0.472005,0.437742,-1.581011), m));
body.add( make_shared<Triangle>(vec3(-0.273229,0.506053,-2.257170),vec3(-0.472005,0.437742,-1.581011),vec3(-0.350594,0.685522,-1.946242), m));

bekkie.add( make_shared<Triangle>(vec3(0.432723,-1.232116,-1.658111),vec3(0.206976,-0.883530,-1.589238),vec3(0.299420,-1.232704,-1.426848), m2));
bekkie.add( make_shared<Triangle>(vec3(0.299420,-1.232704,-2.219378),vec3(0.314795,-0.883530,-1.911020),vec3(0.432723,-1.232116,-1.996966), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.015276,-1.232116,-2.279707),vec3(0.184962,-1.233292,-2.092628),vec3(-0.015276,-1.233292,-2.134024), m2));
bekkie.add( make_shared<Triangle>(vec3(0.206976,-0.883530,-2.054593),vec3(-0.015276,-1.232116,-2.279707),vec3(0.000000,-0.883530,-2.116803), m2));
bekkie.add( make_shared<Triangle>(vec3(0.206976,-0.883530,-1.589238),vec3(-0.015276,-1.232116,-1.357659),vec3(0.299420,-1.232704,-1.426848), m2));
bekkie.add( make_shared<Triangle>(vec3(0.432723,-1.232116,-1.996966),vec3(0.307225,-0.883530,-1.736031),vec3(0.432723,-1.232116,-1.658111), m2));
bekkie.add( make_shared<Triangle>(vec3(0.184962,-1.233292,-1.543204),vec3(-0.015276,-1.232116,-1.357659),vec3(-0.015276,-1.233292,-1.494325), m2));
bekkie.add( make_shared<Triangle>(vec3(0.299420,-1.232704,-2.219378),vec3(0.294171,-1.232704,-1.946138),vec3(0.184962,-1.233292,-2.092628), m2));
bekkie.add( make_shared<Triangle>(vec3(0.432723,-1.232116,-1.658111),vec3(0.294171,-1.232704,-1.946138),vec3(0.432723,-1.232116,-1.996966), m2));
bekkie.add( make_shared<Triangle>(vec3(0.299420,-1.232704,-1.426848),vec3(0.294171,-1.232704,-1.696223),vec3(0.432723,-1.232116,-1.658111), m2));
bekkie.add( make_shared<Triangle>(vec3(0.294171,-1.232704,-1.696223),vec3(-0.015276,-1.095139,-1.928303),vec3(0.294171,-1.232704,-1.946138), m2));
bekkie.add( make_shared<Triangle>(vec3(0.294171,-1.232704,-1.946138),vec3(-0.015276,-1.095139,-1.928303),vec3(0.184962,-1.233292,-2.092628), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.015276,-1.233292,-2.134024),vec3(0.184962,-1.233292,-2.092628),vec3(-0.015276,-1.095139,-1.928303), m2));
bekkie.add( make_shared<Triangle>(vec3(0.294171,-1.232704,-1.696223),vec3(0.184962,-1.233292,-1.543204),vec3(-0.015276,-1.095139,-1.689764), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.015276,-1.233292,-1.494325),vec3(-0.015276,-1.095139,-1.689764),vec3(0.184962,-1.233292,-1.543204), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.206976,-0.883530,-1.589238),vec3(-0.463275,-1.232116,-1.658111),vec3(-0.329972,-1.232704,-1.426848), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.329972,-1.232704,-2.219378),vec3(-0.314795,-0.883530,-1.911020),vec3(-0.206976,-0.883530,-2.054593), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.215513,-1.233292,-2.092628),vec3(-0.015276,-1.232116,-2.279707),vec3(-0.015276,-1.233292,-2.134024), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.206976,-0.883530,-2.054593),vec3(-0.015276,-1.232116,-2.279707),vec3(-0.329972,-1.232704,-2.219378), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.206976,-0.883530,-1.589238),vec3(-0.015276,-1.232116,-1.357659),vec3(0.000000,-0.883530,-1.503144), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.307225,-0.883530,-1.736031),vec3(-0.463275,-1.232116,-1.996966),vec3(-0.463275,-1.232116,-1.658111), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.215513,-1.233292,-1.543204),vec3(-0.015276,-1.232116,-1.357659),vec3(-0.329972,-1.232704,-1.426848), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.329972,-1.232704,-2.219378),vec3(-0.324722,-1.232704,-1.946138),vec3(-0.463275,-1.232116,-1.996966), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.324722,-1.232704,-1.946138),vec3(-0.463275,-1.232116,-1.658111),vec3(-0.463275,-1.232116,-1.996966), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.324722,-1.232704,-1.696223),vec3(-0.329972,-1.232704,-1.426848),vec3(-0.463275,-1.232116,-1.658111), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.015276,-1.095139,-1.928303),vec3(-0.324722,-1.232704,-1.696223),vec3(-0.324722,-1.232704,-1.946138), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.324722,-1.232704,-1.946138),vec3(-0.215513,-1.233292,-2.092628),vec3(-0.015276,-1.095139,-1.928303), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.015276,-1.233292,-2.134024),vec3(-0.015276,-1.095139,-1.928303),vec3(-0.215513,-1.233292,-2.092628), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.324722,-1.232704,-1.696223),vec3(-0.015276,-1.095139,-1.689764),vec3(-0.215513,-1.233292,-1.543204), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.015276,-1.233292,-1.494325),vec3(-0.215513,-1.233292,-1.543204),vec3(-0.015276,-1.095139,-1.689764), m2));
bekkie.add( make_shared<Triangle>(vec3(0.432723,-1.232116,-1.658111),vec3(0.307225,-0.883530,-1.736031),vec3(0.206976,-0.883530,-1.589238), m2));
bekkie.add( make_shared<Triangle>(vec3(0.299420,-1.232704,-2.219378),vec3(0.206976,-0.883530,-2.054593),vec3(0.314795,-0.883530,-1.911020), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.015276,-1.232116,-2.279707),vec3(0.299420,-1.232704,-2.219378),vec3(0.184962,-1.233292,-2.092628), m2));
bekkie.add( make_shared<Triangle>(vec3(0.206976,-0.883530,-2.054593),vec3(0.299420,-1.232704,-2.219378),vec3(-0.015276,-1.232116,-2.279707), m2));
bekkie.add( make_shared<Triangle>(vec3(0.206976,-0.883530,-1.589238),vec3(0.000000,-0.883530,-1.503144),vec3(-0.015276,-1.232116,-1.357659), m2));
bekkie.add( make_shared<Triangle>(vec3(0.432723,-1.232116,-1.996966),vec3(0.314795,-0.883530,-1.911020),vec3(0.307225,-0.883530,-1.736031), m2));
bekkie.add( make_shared<Triangle>(vec3(0.184962,-1.233292,-1.543204),vec3(0.299420,-1.232704,-1.426848),vec3(-0.015276,-1.232116,-1.357659), m2));
bekkie.add( make_shared<Triangle>(vec3(0.299420,-1.232704,-2.219378),vec3(0.432723,-1.232116,-1.996966),vec3(0.294171,-1.232704,-1.946138), m2));
bekkie.add( make_shared<Triangle>(vec3(0.432723,-1.232116,-1.658111),vec3(0.294171,-1.232704,-1.696223),vec3(0.294171,-1.232704,-1.946138), m2));
bekkie.add( make_shared<Triangle>(vec3(0.299420,-1.232704,-1.426848),vec3(0.184962,-1.233292,-1.543204),vec3(0.294171,-1.232704,-1.696223), m2));
bekkie.add( make_shared<Triangle>(vec3(0.294171,-1.232704,-1.696223),vec3(-0.015276,-1.095139,-1.689764),vec3(-0.015276,-1.095139,-1.928303), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.206976,-0.883530,-1.589238),vec3(-0.307225,-0.883530,-1.736031),vec3(-0.463275,-1.232116,-1.658111), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.329972,-1.232704,-2.219378),vec3(-0.463275,-1.232116,-1.996966),vec3(-0.314795,-0.883530,-1.911020), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.215513,-1.233292,-2.092628),vec3(-0.329972,-1.232704,-2.219378),vec3(-0.015276,-1.232116,-2.279707), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.206976,-0.883530,-2.054593),vec3(0.000000,-0.883530,-2.116803),vec3(-0.015276,-1.232116,-2.279707), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.206976,-0.883530,-1.589238),vec3(-0.329972,-1.232704,-1.426848),vec3(-0.015276,-1.232116,-1.357659), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.307225,-0.883530,-1.736031),vec3(-0.314795,-0.883530,-1.911020),vec3(-0.463275,-1.232116,-1.996966), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.215513,-1.233292,-1.543204),vec3(-0.015276,-1.233292,-1.494325),vec3(-0.015276,-1.232116,-1.357659), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.329972,-1.232704,-2.219378),vec3(-0.215513,-1.233292,-2.092628),vec3(-0.324722,-1.232704,-1.946138), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.324722,-1.232704,-1.946138),vec3(-0.324722,-1.232704,-1.696223),vec3(-0.463275,-1.232116,-1.658111), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.324722,-1.232704,-1.696223),vec3(-0.215513,-1.233292,-1.543204),vec3(-0.329972,-1.232704,-1.426848), m2));
bekkie.add( make_shared<Triangle>(vec3(-0.015276,-1.095139,-1.928303),vec3(-0.015276,-1.095139,-1.689764),vec3(-0.324722,-1.232704,-1.696223), m2));
//////////////////////////////////////// GENERATED CODE END

        add(make_shared<HittableList>(body));
        add(make_shared<HittableList>(bekkie));

    }

    virtual bool trace(const Ray& r, float t_min, float t_max, hit& rec) const {

        if (HittableList::trace(r, t_min, t_max, rec)) {
            rec.specialObject = true;
            return true;
        } 

        return false; 
    };
};

class Translate : public Hittable
{
    public:
        Translate(shared_ptr<Hittable> p, const vec3& d) : ptr(p), displacement(d) {}

    virtual bool trace(const Ray& r, float t_min, float t_max, hit& rec) const {
        // we do the inverse of the translation to the ray
        Ray newR(r.origin - displacement, r.direction); 

        if (!ptr->trace(newR, t_min, t_max, rec))
            return false;

        // It was a hit but found like the ray was not changed so we apply the change
        rec.point += displacement;

        return true;
    }

    private:
        shared_ptr<Hittable> ptr;
        vec3 displacement;
};

class RotateZ : public Hittable
{
    public:
        RotateZ(shared_ptr<Hittable> p, float angle) : ptr(p) {
            auto radians = MATH::degreesToRadians(angle);
            sin_theta = sin(radians);
            cos_theta = cos(radians);
        }

    virtual bool trace(const Ray& r, float t_min, float t_max, hit& rec) const {
        auto origin = r.origin;
        auto direction = r.direction;

        origin.x = cos_theta * r.origin.x - sin_theta * r.origin.y;
        origin.y = sin_theta * r.origin.x + cos_theta * r.origin.y;

        direction.x = cos_theta * r.direction.x - sin_theta * r.direction.y;
        direction.y = sin_theta * r.direction.x + cos_theta * r.direction.y;

        Ray rotated_r(origin, direction);

        if (!ptr->trace(rotated_r, t_min, t_max, rec))
            return false;

        auto p = rec.point;
        auto newNormal = rec.normal;

        p.x =  cos_theta * rec.point.x + sin_theta * rec.point.y;
        p.y = -sin_theta * rec.point.x + cos_theta * rec.point.y;

        newNormal.x =  cos_theta * rec.normal.x + sin_theta * rec.normal.y;
        newNormal.y = -sin_theta * rec.normal.x + cos_theta * rec.normal.y;

        rec.point = p;
        rec.normal = newNormal;

        return true;
    }

    private:
        shared_ptr<Hittable> ptr;
        float sin_theta;
        float cos_theta;
};

#endif