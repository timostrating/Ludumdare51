#include <emscripten.h>

#include "common.h"
#include "hittable.h"
#include "camera.h"
#include "material.h"

#include <emscripten/bind.h>
#include <emscripten/val.h> // for memory view ... emscripten::val 

#define INF 999999.9
#define COLOR_CHANNELS 3
#define IMAGE_WIDTH 250
#define IMAGE_HEIGHT 250
#define BUFFER_CHANNELS 4
#define BUFFER_LENGTH   IMAGE_WIDTH * IMAGE_HEIGHT * BUFFER_CHANNELS


// EM_JS(void, __draw, (int x, int y, int r, int g, int b), {
//     ctx.fillStyle = "rgb("+r+","+g+","+b+")";
//     ctx.fillRect(x, y, 1, 1); 
// });

// inline void draw (int x, int y, vec3 color) {
//     __draw(x,y, static_cast<int>(color.r * 255.99), static_cast<int>(color.g * 255.99), static_cast<int>(color.b * 255.99));
// }


std::vector<float> data(IMAGE_WIDTH * IMAGE_HEIGHT * COLOR_CHANNELS, 0.0f);
std::vector<float> rayCounter(IMAGE_WIDTH * IMAGE_HEIGHT, 0.0f); // int counter that is used to devide

static unsigned char byteBuffer[BUFFER_LENGTH];

inline void draw (int x, int y, const vec3 color) {
    int index = (y*IMAGE_WIDTH + x);
    data[index * COLOR_CHANNELS + 0] += color.r;
    data[index * COLOR_CHANNELS + 1] += color.g;
    data[index * COLOR_CHANNELS + 2] += color.b;
    rayCounter[index] += 1.0f;

    byteBuffer[index * BUFFER_CHANNELS + 0] = static_cast<unsigned char>((data[index * COLOR_CHANNELS + 0] / rayCounter[index]) * 255.0f);
    byteBuffer[index * BUFFER_CHANNELS + 1] = static_cast<unsigned char>((data[index * COLOR_CHANNELS + 1] / rayCounter[index]) * 255.0f);
    byteBuffer[index * BUFFER_CHANNELS + 2] = static_cast<unsigned char>((data[index * COLOR_CHANNELS + 2] / rayCounter[index]) * 255.0f);
    byteBuffer[index * BUFFER_CHANNELS + 3] = 0xff;
}

inline void clear() {
    std::fill(data.begin(), data.end(), 0.0f);
    std::fill(rayCounter.begin(), rayCounter.end(), 0.0f);
    for (int i=0; i<BUFFER_LENGTH; i++) {
        byteBuffer[i] = 0x00;
    }
}

HittableList world1();



static int g_level = 0;
static HittableList g_world = world1(); // world
static Camera g_camera(vec3(-4,-10,1), vec3(-2,0,5), vec3(0,0,1));
static vec3 g_background = vec3(0, 0, 0);

HittableList world1() {
    HittableList world;

    auto matEend1 = make_shared<Metal>(vec3(1.0, 1.0, 0.0), 0.8);
    auto matEend2 = make_shared<Metal>(vec3(1.0, 0.5, 0.0), 0.8);
    world.add(make_shared<RotateZ>(make_shared<BadEend>(matEend1, matEend2), 55.0f));

    return world;
}

HittableList world2() {
    HittableList world;

    auto ground_material = make_shared<Unlit>(color(0.5, 0.5, 0.5));
    world.add(make_shared<Sphere>(vec3(0,-1000,0), 1000, ground_material));

    auto material1 = make_shared<Light>(vec3(4.0, 4.0, 4.0));
    for (int i = -10; i <10; i++) {
        world.add(make_shared<Sphere>(vec3(-2,i,0), 1.0, material1));
    }

    auto matEend1 = make_shared<Lambertian>(vec3(0.0, 0.0, 0.0));
    auto matEend2 = make_shared<Lambertian>(vec3(0.9, 0.9, 0.9));
    world.add(make_shared<RotateZ>(make_shared<Translate>(make_shared<BadEend>(matEend1, matEend2), vec3(0,0,1)), 45.0f));

    return world;
}

HittableList world3() {
    HittableList world;

    auto ground_material = make_shared<Metal>(vec3(0.4, 0.4, 0.4), 0.1);
    world.add(make_shared<Sphere>(vec3(0,0,1000.5), 1000, ground_material));

    auto orangeLight = make_shared<Special>(vec3(1.0, 0.95, 0.1 * MATH::random()));

    int i = 0;
    for (float x = -5.0f; x<=5.0f; x+=0.9999f) {
        for (float y = -5.0f; y<=5.0f; y+=0.9999f, i++) {
            auto yellowLight = make_shared<Special>(vec3(1.0, 1.0, 0.1 * MATH::random()));
            
            if (i == 101) {
                world.add(make_shared<Translate>(make_shared<RotateZ>(make_shared<BadEend>(yellowLight, orangeLight), 220.0f), vec3(x * 3.0 - 0.5, y * 3.0 + 0.5, 0)));
            } else {
                world.add(make_shared<Sphere>(vec3(x * 3.0 + 0.1 * MATH::random(), y * 3.0 + 0.1 * MATH::random(),0), 1.1, yellowLight));
            }
        }
    }

    return world;
}

void loadWorld(int level) {
    g_level = level;
    switch (g_level) {
        case 1:
            g_camera.setPosition(vec3(0,-2,-2));
            g_camera.setLookat(vec3(0,0,-1));
            g_background = vec3(0.4,0.4,1.0);
            g_world = world1();
            break;
        case 2:
            g_camera.setPosition(vec3(-4,-10,1));
            g_camera.setLookat(vec3(-2,0,5));
            g_background = vec3(1,1,1);
            g_world = world2();
            break;
        case 3:
            g_camera.setPosition(vec3(0,0.01,-17));
            g_camera.setLookat(vec3(0,0,0));
            g_background = vec3(0.1, 0.08, 0.15);
            g_world = world3();
            break;
    }

    std::cout << "Loaded level " << g_level << std::endl;
}

vec3 trace(const Ray& r, const Hittable& hittable, int depth) {
    hit rec; 

    // end of recursive ray bounces
    if (depth <= 0) 
        return vec3(0,0,0);

    // if the ray hits nothing
    if (!hittable.trace(r, 0.001, INF, rec))
        return g_background;

    Ray scattered;
    vec3 albedo;
    vec3 emitted = rec.mat_ptr->emitted();

    if (!rec.mat_ptr->scatter(r, rec, albedo, scattered))
        return emitted;

    auto tr = trace(scattered, hittable, depth-1);

    return emitted + albedo * tr;
}

void sendRay(float u, float v, float radius) {
    for (int rx=-radius; rx<=radius; rx++) {
        for (int ry=-radius; ry<=radius; ry++) {
            if (sqrt(rx*rx + ry*ry) > radius)
                continue;
            
            float u2 = u + (rx + MATH::random()) / float(IMAGE_WIDTH);
            float v2 = v + (ry + MATH::random()) / float(IMAGE_HEIGHT);

            Ray r = g_camera.getRay(u2, v2);

            int x = int(u2 * float(IMAGE_WIDTH));
            int y = int(v2 * float(IMAGE_HEIGHT));

            if (x >= 0 && x < IMAGE_WIDTH && y >= 0 && y < IMAGE_HEIGHT)
                draw(x, y, trace(r, g_world, 3 + int(rayCounter[y*IMAGE_WIDTH + x] / 5.0f)));
        }
    }
}

void render() {
    for (int y = 0; y < IMAGE_HEIGHT; y++) {
        for (int x = 0; x < IMAGE_WIDTH; x++) {
            auto u = (float(x) + MATH::random()) / float(IMAGE_WIDTH-1);
            auto v = (float(y) + MATH::random()) / float(IMAGE_HEIGHT-1);
            Ray r = g_camera.getRay(u, v);
            draw(x, y, trace(r, g_world, 4));
        }
    }
}

void renderAt(int x, int y, int z) { // tmp
    clear();
    g_camera.setPosition(vec3(x,y,z));
    render();
}

bool raycast(float x, float y) {
    Ray r = g_camera.getRay(x, y);
    hit rec;
    if (g_world.trace(r, 0.001, INF, rec)) {
        if (rec.specialObject) {
             std::cout << "YHEEE" << std::endl;
            return true;
        } else {
            std::cout << "NOOOO" << std::endl;
        }
    } else {
        std::cout << "NO hit" << std::endl;
    }

    return false;
}

emscripten::val copy() {
    return emscripten::val(emscripten::typed_memory_view(BUFFER_LENGTH, byteBuffer));
}

EMSCRIPTEN_BINDINGS(module) {
    emscripten::function("sendRay", &sendRay);
    emscripten::function("render", &render);
    emscripten::function("renderAt", &renderAt);
    emscripten::function("raycast", &raycast);
    emscripten::function("copy", &copy);
    emscripten::function("loadWorld", &loadWorld);
    emscripten::function("clear", &clear);
}