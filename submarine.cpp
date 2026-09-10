#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <string>

// ============================================================
// CONSTANTS & GLOBALS
// ============================================================
#define PI 3.14159265358979323846
#define DEG_TO_RAD (PI / 180.0)

// Window
int windowWidth = 1280;
int windowHeight = 720;

// Game States
enum GameState { STATE_INTRO, STATE_DIVING, STATE_MENU, STATE_PLAYING };
GameState gameState = STATE_INTRO;

// Camera Modes
enum CameraMode {
    CAM_EXTERNAL = 0,
    CAM_INTERIOR,
    CAM_FRONT_WINDOW,
    CAM_LEFT_WINDOW,
    CAM_RIGHT_WINDOW,
    CAM_CONTROL_SCREEN,
    CAM_FREE,
    CAM_COUNT
};
CameraMode cameraMode = CAM_EXTERNAL;
const char* cameraNames[] = {
    "EXTERNAL 3D VIEW", "INTERIOR VIEW", "FRONT WINDOW",
    "LEFT WINDOW", "RIGHT WINDOW", "CONTROL SCREEN", "FREE CAMERA"
};

// Submarine State
struct Submarine {
    float x, y, z;
    float yaw, pitch, roll;
    float speed;
    float targetSpeed;
    float propellerAngle;
    bool headlightsOn;
    float depth;
    float descentRate;
} sub;
const float SUB_SCALE = 1.4f;
const float SUB_LEN = 1.5f;
const float BOW_TIP = 3.9f * 1.4f * 1.5f;

// Camera State
struct Camera {
    float orbitAngleH;
    float orbitAngleV;
    float orbitDistance;
    float freeX, freeY, freeZ;
    float freeYaw, freePitch;
} cam;

// Mission System
enum Mission { MISSION_DIVE = 0, MISSION_EXPLORE, MISSION_NAVIGATE, MISSION_OBSERVE, MISSION_RETURN, MISSION_COUNT };
Mission currentMission = MISSION_DIVE;
const char* missionNames[] = {
    "DESCEND INTO THE DEEP OCEAN",
    "EXPLORE THE UNDERWATER ENVIRONMENT",
    "NAVIGATE THROUGH ROCKS AND VEGETATION",
    "OBSERVE MARINE LIFE THROUGH WINDOWS",
    "RETURN SAFELY TOWARD THE SURFACE"
};

// Intro / Dive animation
float introTimer = 0.0f;
float diveTimer = 0.0f;
float diveTransition = 0.0f; // 0=surface, 1=fully underwater
int menuSelection = 0;
bool missionStarted = false;
float missionTimer = 0.0f;

// ============================================================
// FISH / MARINE LIFE
// ============================================================
struct Fish {
    float x, y, z;
    float speed;
    float angle;
    float size;
    int type; // 0=small, 1=large, 2=jellyfish, 3=octopus
    float animPhase;
    float r, g, b;
};
std::vector<Fish> fishes;

struct Bubble {
    float x, y, z;
    float speed;
    float size;
    float wobble;
    float wobblePhase;
};
std::vector<Bubble> bubbles;

struct Seaweed {
    float x, z;
    float height;
    float phase;
};
std::vector<Seaweed> seaweeds;

struct Rock {
    float x, y, z;
    float scaleX, scaleY, scaleZ;
    float r, g, b;
};
std::vector<Rock> rocks;

struct Coral {
    float x, y, z;
    float size;
    float r, g, b;
    int type;
};
std::vector<Coral> corals;

struct Shark {
    float cx, cy, cz;
    float radius;
    float angle;
    float speed;
    float size;
};
std::vector<Shark> sharks;

struct Ray {
    float cx, cy, cz;
    float radius;
    float angle;
    float speed;
    float size;
    float phase;
};
std::vector<Ray> rays;

struct Kelp {
    float x, z;
    float h;
    float phase;
};
std::vector<Kelp> kelp;

// Crew
struct CrewMember {
    float x, y, z;
    float facing;
    float animPhase;
    int action; // 0=sitting, 1=standing, 2=working
};
std::vector<CrewMember> crew;

// Particles for floating debris
struct Particle {
    float x, y, z;
    float vx, vy, vz;
    float size;
    float alpha;
};
std::vector<Particle> particles;

// Clouds
struct Cloud {
    float x, y, z;
    float scale;
    float speed;
};
std::vector<Cloud> clouds;

// Tree positions on surface
struct Tree {
    float x, z;
    float height;
};
std::vector<Tree> trees;

// ============================================================
// TEXT DRAWING UTILITIES
// ============================================================
void drawText(float x, float y, const char* text, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c; c++) {
        glutBitmapCharacter(font, *c);
    }
}

void drawText3D(float x, float y, float z, const char* text, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos3f(x, y, z);
    for (const char* c = text; *c; c++) {
        glutBitmapCharacter(font, *c);
    }
}

// ============================================================
// GEOMETRY PRIMITIVES
// ============================================================
void drawSphere(float radius, int slices = 16, int stacks = 12) {
    glutSolidSphere(radius, slices, stacks);
}

void drawCylinder(float radius, float height, int slices = 16) {
    GLUquadric* quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);
    gluCylinder(quad, radius, radius, height, slices, 1);
    gluDeleteQuadric(quad);
}

void drawCone(float radius, float height, int slices = 16) {
    GLUquadric* quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);
    gluCylinder(quad, radius, 0.0, height, slices, 1);
    gluDeleteQuadric(quad);
}

void drawDisk(float innerRadius, float outerRadius, int slices = 32) {
    GLUquadric* quad = gluNewQuadric();
    gluDisk(quad, innerRadius, outerRadius, slices, 1);
    gluDeleteQuadric(quad);
}

void drawCube(float size) {
    glutSolidCube(size);
}

void drawTorus(float inner, float outer, int sides = 16, int rings = 32) {
    glutSolidTorus(inner, outer, sides, rings);
}

// ============================================================
// PROCEDURAL TEXTURES (realistic sky / water / metal / sand,
// soft cloud sprites — generated in code, no image files needed)
// ============================================================
GLuint texSky = 0, texWater = 0, texSand = 0, texHull = 0, texPuff = 0;

static unsigned int hashNoise(int x, int y, int seed) {
    unsigned int h = (unsigned int)(x * 374761393 + y * 668265263 + seed * 1442695041);
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static float noise01(int x, int y, int seed) {
    return (hashNoise(x, y, seed) & 1023) / 1023.0f;
}
static float smoothNoise(float x, float y, int seed) {
    int xi = (int)floor(x), yi = (int)floor(y);
    float xf = x - xi, yf = y - yi;
    float u = xf * xf * (3 - 2 * xf), v = yf * yf * (3 - 2 * yf);
    float a = noise01(xi, yi, seed), b = noise01(xi + 1, yi, seed);
    float c = noise01(xi, yi + 1, seed), d = noise01(xi + 1, yi + 1, seed);
    return a + (b - a) * u + (c - a) * v + (a - b - c + d) * u * v;
}
static GLuint uploadTexture(int w, int h, unsigned char* px, bool rgba, bool repeat) {
    GLuint t; glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, rgba ? GL_RGBA : GL_RGB, w, h, 0,
                 rgba ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, px);
    return t;
}

void initTextures() {
    // ---- Sky dome gradient: zenith blue -> mid steel -> warm horizon ----
    {
        const int W = 64, H = 256;
        static unsigned char px[64 * 256 * 3];
        for (int y = 0; y < H; y++) {
            float v = y / (float)(H - 1); // 0 bottom, 1 top
            float r, g, b;
            if (v < 0.42f)      { r = 18; g = 38; b = 78; }
            else if (v < 0.52f) { float k = (v - 0.42f) / 0.10f; r = 18 + 237 * k; g = 38 + 138 * k; b = 78 + 32 * k; }
            else if (v < 0.60f) { float k = (v - 0.52f) / 0.08f; r = 255 - 55 * k; g = 176 + 14 * k; b = 110 + 90 * k; }
            else if (v < 0.78f) { float k = (v - 0.60f) / 0.18f; r = 200 - 110 * k; g = 190 - 60 * k; b = 200 + 10 * k; }
            else                { float k = (v - 0.78f) / 0.22f; r = 90 - 62 * k; g = 130 - 68 * k; b = 210 - 68 * k; }
            for (int x = 0; x < W; x++) {
                int o = (y * W + x) * 3;
                px[o] = (unsigned char)r; px[o + 1] = (unsigned char)g; px[o + 2] = (unsigned char)b;
            }
        }
        texSky = uploadTexture(W, H, px, false, false);
    }
    // ---- Water detail (near-white multiplier: keeps baked color, adds grain) ----
    {
        const int W = 128, H = 128;
        static unsigned char px[128 * 128 * 3];
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
            float n = smoothNoise(x * 0.15f, y * 0.15f, 7) * 0.6f + smoothNoise(x * 0.5f, y * 0.5f, 21) * 0.4f;
            float v = 218 + n * 37;
            float patch = smoothNoise(x * 0.05f, y * 0.05f, 99);
            if (patch < 0.35f) v *= 0.86f; // darker depth patches
            int o = (y * W + x) * 3;
            px[o] = px[o + 1] = px[o + 2] = (unsigned char)(v > 255 ? 255 : v);
        }
        texWater = uploadTexture(W, H, px, false, true);
    }
    // ---- Sand: warm tan speckle + pebbles ----
    {
        const int W = 128, H = 128;
        static unsigned char px[128 * 128 * 3];
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
            float n = noise01(x, y, 5);
            float r = 206 + (n - 0.5f) * 36, g = 178 + (n - 0.5f) * 32, b = 138 + (n - 0.5f) * 26;
            if (noise01(x / 4, y / 4, 6) > 0.93f) { r *= 0.62f; g *= 0.60f; b *= 0.58f; } // pebble
            if (noise01(x / 6, y / 6, 8) > 0.95f) { r = 235; g = 225; b = 205; } // shell bit
            int o = (y * W + x) * 3;
            px[o] = (unsigned char)r; px[o + 1] = (unsigned char)g; px[o + 2] = (unsigned char)b;
        }
        texSand = uploadTexture(W, H, px, false, true);
    }
    // ---- Hull metal: bright base (multiplier) + panel lines + rivets + grain ----
    {
        const int W = 128, H = 128;
        static unsigned char px[128 * 128 * 3];
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
            float v = 218 + (noise01(x, y, 11) - 0.5f) * 14;
            if (y % 21 == 0) v = 150; // plate seams
            if (x % 42 == 0) v = 160;
            if (y % 21 == 0 && x % 7 == 0) v = 120; // rivets
            int o = (y * W + x) * 3;
            px[o] = px[o + 1] = px[o + 2] = (unsigned char)v;
        }
        texHull = uploadTexture(W, H, px, false, true);
    }
    // ---- Soft puff sprite (clouds, sun glow, halos) ----
    {
        const int W = 64, H = 64;
        static unsigned char px[64 * 64 * 4];
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
            float dx = (x - 32) / 32.0f, dy = (y - 32) / 32.0f;
            float r = sqrt(dx * dx + dy * dy);
            float n = smoothNoise(x * 0.3f, y * 0.3f, 33);
            float a = 1.0f - r;
            if (a < 0) a = 0;
            a = pow(a, 1.6f) * (0.55f + 0.45f * n);
            if (r > 0.95f) a = 0;
            int o = (y * W + x) * 4;
            px[o] = px[o + 1] = px[o + 2] = 255;
            px[o + 3] = (unsigned char)(a * 255);
        }
        texPuff = uploadTexture(W, H, px, true, false);
    }
}

// Camera-facing textured quad (for clouds / glows). Caller binds texture,
// enables blending and sets color. Must be called after camera is set.
void drawBillboard(float x, float y, float z, float w, float h) {
    float m[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, m);
    float rx = m[0], ry = m[4], rz = m[8];
    float ux = m[1], uy = m[5], uz = m[9];
    float hx = rx * w * 0.5f, hy = ry * w * 0.5f, hz = rz * w * 0.5f;
    float vx = ux * h * 0.5f, vy = uy * h * 0.5f, vz = uz * h * 0.5f;
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(x - hx - vx, y - hy - vy, z - hz - vz);
    glTexCoord2f(1, 0); glVertex3f(x + hx - vx, y + hy - vy, z + hz - vz);
    glTexCoord2f(1, 1); glVertex3f(x + hx + vx, y + hy + vy, z + hz + vz);
    glTexCoord2f(0, 1); glVertex3f(x - hx + vx, y - hy + vy, z - hz + vz);
    glEnd();
}

// ============================================================
// INITIALIZATION
// ============================================================
void initSubmarine() {
    sub.x = 0; sub.y = 0.9f; sub.z = 0; // half-submerged like 1st ref
    sub.yaw = 0; sub.pitch = 0; sub.roll = 0;
    sub.speed = 0; sub.targetSpeed = 0;
    sub.propellerAngle = 0;
    sub.headlightsOn = true;
    sub.depth = 0;
    sub.descentRate = 0;
}

void initCamera() {
    cam.orbitAngleH = 30.0f;
    cam.orbitAngleV = 20.0f;
    cam.orbitDistance = 21.0f;
    cam.freeX = 0; cam.freeY = 5; cam.freeZ = 15;
    cam.freeYaw = 0; cam.freePitch = 0;
}

void initEnvironment() {
    srand((unsigned)time(NULL));

    // Fish
    fishes.clear();
    for (int i = 0; i < 160; i++) {
        Fish f;
        f.x = (rand() % 400 - 200) * 0.1f;
        f.y = -(rand() % 80) * 0.5f;
        f.z = (rand() % 400 - 200) * 0.1f;
        f.speed = 0.02f + (rand() % 100) * 0.0005f;
        f.angle = (rand() % 360) * DEG_TO_RAD;
        int sizeClass = rand() % 10;
        if (sizeClass < 4) f.size = 0.18f + (rand() % 100) * 0.0015f;
        else if (sizeClass < 8) f.size = 0.35f + (rand() % 100) * 0.0025f;
        else f.size = 0.60f + (rand() % 100) * 0.004f;
        f.type = rand() % 4;
        if (f.type == 3) f.type = 1;
        f.animPhase = (rand() % 1000) * 0.01f;
        switch (rand() % 6) {
        case 0: f.r = 1.0f; f.g = 0.55f; f.b = 0.10f; break;
        case 1: f.r = 1.0f; f.g = 0.85f; f.b = 0.20f; break;
        case 2: f.r = 0.20f; f.g = 0.45f; f.b = 1.0f; break;
        case 3: f.r = 1.0f; f.g = 0.45f; f.b = 0.05f; break;
        case 4: f.r = 0.15f; f.g = 0.75f; f.b = 0.85f; break;
        default: f.r = 0.95f; f.g = 0.95f; f.b = 0.95f; break;
        }
        if (f.type == 1) { f.size = 0.55f + (rand() % 100) * 0.004f; }
        if (f.type == 2) { f.r = 0.75f; f.g = 0.45f; f.b = 0.95f; f.y -= 2.0f; }
        fishes.push_back(f);
    }

    // Seaweed
    seaweeds.clear();
    for (int i = 0; i < 80; i++) {
        Seaweed s;
        s.x = (rand() % 600 - 300) * 0.1f;
        s.z = (rand() % 600 - 300) * 0.1f;
        s.height = 0.5f + (rand() % 100) * 0.03f;
        s.phase = (rand() % 1000) * 0.01f;
        seaweeds.push_back(s);
    }

    // Reef centers: rock piles + coral + kelp cluster here
    float reefX[] = { 8.0f, -10.0f, 14.0f, -4.0f, 2.0f, -16.0f };
    float reefZ[] = { 6.0f, 8.0f, -8.0f, -12.0f, 14.0f, -4.0f };

    // Rocks
    rocks.clear();
    for (int i = 0; i < 90; i++) {
        Rock r;
        if (i < 60) {
            int reef = rand() % 6;
            r.x = reefX[reef] + (rand() % 100 - 50) * 0.08f;
            r.z = reefZ[reef] + (rand() % 100 - 50) * 0.08f;
            r.y = -16.0f + (rand() % 100) * 0.025f;
        } else {
            r.x = (rand() % 600 - 300) * 0.1f;
            r.y = -16.0f - (rand() % 50) * 0.06f;
            r.z = (rand() % 600 - 300) * 0.1f;
        }
        r.scaleX = 0.5f + (rand() % 100) * 0.017f;
        r.scaleY = 0.4f + (rand() % 100) * 0.014f;
        r.scaleZ = 0.5f + (rand() % 100) * 0.017f;
        float algae = (rand() % 100) / 100.0f;
        r.r = 0.32f + algae * 0.10f;
        r.g = 0.30f + algae * 0.16f;
        r.b = 0.22f + algae * 0.06f;
        rocks.push_back(r);
    }

    // Kelp tufts around reefs
    kelp.clear();
    for (int i = 0; i < 150; i++) {
        Kelp k;
        int reef = rand() % 6;
        if (rand() % 100 < 75) {
            k.x = reefX[reef] + (rand() % 100 - 50) * 0.10f;
            k.z = reefZ[reef] + (rand() % 100 - 50) * 0.10f;
        } else {
            k.x = (rand() % 600 - 300) * 0.1f;
            k.z = (rand() % 600 - 300) * 0.1f;
        }
        k.h = 4.0f + (rand() % 100) * 0.05f;
        k.phase = (rand() % 1000) * 0.01f;
        kelp.push_back(k);
    }

    // Coral
    corals.clear();
    for (int i = 0; i < 70; i++) {
        Coral c;
        c.x = (rand() % 400 - 200) * 0.1f;
        c.y = -16.0f;
        c.z = (rand() % 400 - 200) * 0.1f;
        c.size = 0.7f + (rand() % 100) * 0.015f;
        c.y = -15.7f;
        c.type = rand() % 3;
        switch (rand() % 6) {
        case 0: c.r = 1.0f; c.g = 0.62f; c.b = 0.72f; break;
        case 1: c.r = 1.0f; c.g = 0.55f; c.b = 0.25f; break;
        case 2: c.r = 0.96f; c.g = 0.93f; c.b = 0.86f; break;
        case 3: c.r = 0.70f; c.g = 0.40f; c.b = 0.85f; break;
        case 4: c.r = 0.95f; c.g = 0.25f; c.b = 0.35f; break;
        default: c.r = 1.0f; c.g = 0.75f; c.b = 0.45f; break;
        }
        corals.push_back(c);
    }

    // Sharks circling mid-water
    sharks.clear();
    for (int i = 0; i < 3; i++) {
        Shark s;
        s.cx = (rand() % 200 - 100) * 0.1f;
        s.cy = -5.0f - (rand() % 50) * 0.1f;
        s.cz = (rand() % 200 - 100) * 0.1f;
        s.radius = 10.0f + (rand() % 100) * 0.08f;
        s.angle = (rand() % 360) * DEG_TO_RAD;
        s.speed = 0.002f + (rand() % 50) * 0.00004f;
        s.size = 1.2f + (rand() % 100) * 0.008f;
        sharks.push_back(s);
    }

    // Stingrays gliding near the reef
    rays.clear();
    for (int i = 0; i < 2; i++) {
        Ray r;
        r.cx = (rand() % 200 - 100) * 0.1f;
        r.cy = -7.0f - (rand() % 40) * 0.1f;
        r.cz = (rand() % 200 - 100) * 0.1f;
        r.radius = 8.0f + (rand() % 100) * 0.06f;
        r.angle = (rand() % 360) * DEG_TO_RAD;
        r.speed = 0.0015f + (rand() % 50) * 0.00003f;
        r.size = 0.9f + (rand() % 100) * 0.006f;
        r.phase = (rand() % 1000) * 0.01f;
        rays.push_back(r);
    }

    // Crew members
    crew.clear();
    CrewMember c1 = { 0.3f, 0.2f, 0.5f, 0, 0, 0 };
    CrewMember c2 = { -0.3f, 0.2f, 0.3f, 0, 0, 1 };
    CrewMember c3 = { 0.0f, 0.2f, -0.2f, PI, 0, 2 };
    CrewMember c4 = { -0.5f, 0.2f, -0.5f, PI * 0.5f, 0, 1 };
    crew.push_back(c1);
    crew.push_back(c2);
    crew.push_back(c3);
    crew.push_back(c4);

    // Clouds
    clouds.clear();
    for (int i = 0; i < 15; i++) {
        Cloud cl;
        cl.x = (rand() % 600 - 300) * 0.1f;
        cl.y = 15.0f + (rand() % 50) * 0.1f;
        cl.z = (rand() % 600 - 300) * 0.1f;
        cl.scale = 1.0f + (rand() % 100) * 0.02f;
        cl.speed = 0.005f + (rand() % 50) * 0.0002f;
        clouds.push_back(cl);
    }

    // Trees
    trees.clear();
    for (int i = 0; i < 12; i++) {
        Tree t;
        float angle = (i / 12.0f) * 2 * PI;
        t.x = cos(angle) * 25.0f + (rand() % 50 - 25) * 0.1f;
        t.z = sin(angle) * 25.0f + (rand() % 50 - 25) * 0.1f;
        t.height = 1.5f + (rand() % 100) * 0.02f;
        trees.push_back(t);
    }

    // Particles
    particles.clear();
    for (int i = 0; i < 100; i++) {
        Particle p;
        p.x = (rand() % 400 - 200) * 0.1f;
        p.y = -(rand() % 100) * 0.3f;
        p.z = (rand() % 400 - 200) * 0.1f;
        p.vx = (rand() % 100 - 50) * 0.0001f;
        p.vy = (rand() % 100 - 50) * 0.0001f;
        p.vz = (rand() % 100 - 50) * 0.0001f;
        p.size = 0.02f + (rand() % 50) * 0.0005f;
        p.alpha = 0.3f + (rand() % 50) * 0.01f;
        particles.push_back(p);
    }

    // Bubbles
    bubbles.clear();
}

// ============================================================
// DRAW SUBMARINE
// ============================================================
void drawSubmarineBody() {
    float hullR = 1.0f;
    float hullDark[3] = { 0.24f, 0.25f, 0.27f };
    float hullSide[3] = { 0.30f, 0.31f, 0.33f };
    float hullTopC[3] = { 0.42f, 0.43f, 0.45f };

    glPushMatrix();
    glColor3fv(hullDark);
    glRotatef(90, 0, 1, 0);
    glTranslatef(0, 0, -2.5f);
    GLUquadric* hullQ = gluNewQuadric();
    gluCylinder(hullQ, hullR, hullR, 5.0f, 48, 6);
    gluDeleteQuadric(hullQ);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(2.5f, 0, 0);
    glColor3fv(hullDark);
    glScalef(1.35f, 1.0f, 0.92f);
    drawSphere(hullR, 48, 32);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.8f, 0.95f, 0);
    glColor3fv(hullTopC);
    glScalef(3.4f, 0.20f, 0.45f);
    drawSphere(0.5f, 24, 12);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-2.5f, 0, 0);
    glRotatef(-90, 0, 1, 0);
    glColor3fv(hullDark);
    GLUquadric* taperQ = gluNewQuadric();
    gluCylinder(taperQ, hullR, 0.30f, 1.7f, 22, 3);
    gluDeleteQuadric(taperQ);
    glPopMatrix();

    glColor3f(0.10f, 0.10f, 0.11f);
    for (int i = 0; i < 3; i++) {
        glPushMatrix();
        glTranslatef(1.2f - i * 1.1f, 0, 0);
        glRotatef(90, 0, 1, 0);
        drawTorus(0.008f, hullR + 0.004f, 6, 36);
        glPopMatrix();
    }

    glPushMatrix();
    glTranslatef(0.9f, hullR - 0.03f, 0);
    glColor3f(0.12f, 0.12f, 0.13f);
    glScalef(1.4f, 0.05f, 0.34f);
    drawCube(1.0f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-1.1f, hullR - 0.03f, 0);
    glColor3f(0.12f, 0.12f, 0.13f);
    glScalef(0.9f, 0.05f, 0.30f);
    drawCube(1.0f);
    glPopMatrix();

    for (int s = -1; s <= 1; s += 2) {
        float zside = s * 0.97f;
        glPushMatrix();
        glTranslatef(-0.2f, 0.05f, zside);
        if (s < 0) glRotatef(180, 0, 1, 0);
        glColor3f(0.75f, 0.72f, 0.60f);
        glScalef(0.55f, 0.22f, 0.02f);
        drawCube(1.0f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(1.15f, 0.18f, zside * 0.99f);
        if (s < 0) glRotatef(180, 0, 1, 0);
        glColor3f(0.70f, 0.70f, 0.70f);
        glScalef(0.34f, 0.10f, 0.02f);
        drawCube(1.0f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(-1.4f, 0.10f, zside * 0.99f);
        if (s < 0) glRotatef(180, 0, 1, 0);
        glColor3f(0.16f, 0.25f, 0.55f);
        glScalef(0.12f, 0.16f, 0.02f);
        drawCube(1.0f);
        glPopMatrix();
    }

    glPushMatrix();
    glTranslatef(-0.1f, hullR + 0.32f, 0);
    glColor3fv(hullDark);
    glScalef(2.30f, 0.68f, 0.62f);
    drawCube(1.0f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(1.0f, hullR + 0.28f, 0);
    glColor3fv(hullDark);
    glRotatef(-20, 0, 0, 1);
    glScalef(0.30f, 0.62f, 0.58f);
    drawCube(1.0f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-0.1f, hullR + 0.68f, 0);
    glColor3f(0.10f, 0.10f, 0.11f);
    glScalef(2.20f, 0.07f, 0.56f);
    drawCube(1.0f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-1.15f, hullR + 0.28f, 0);
    glColor3fv(hullDark);
    glRotatef(22, 0, 0, 1);
    glScalef(0.28f, 0.60f, 0.56f);
    drawCube(1.0f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-0.1f, hullR + 0.72f, 0);
    glRotatef(90, 0, 1, 0);
    glColor3f(0.13f, 0.13f, 0.14f);
    drawCylinder(0.06f, 2.10f, 12);
    glPopMatrix();
    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix();
        glTranslatef(-0.35f, hullR + 0.42f, s * 0.32f);
        if (s < 0) glRotatef(180, 0, 1, 0);
        glColor3f(0.04f, 0.07f, 0.09f);
        drawDisk(0.0f, 0.09f, 12);
        glPopMatrix();
    }
    glDisable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(0.75f, hullR + 0.76f, 0);
    glColor3f(1.0f, 0.22f, 0.12f);
    drawSphere(0.09f, 10, 8);
    glColor4f(1.0f, 0.25f, 0.12f, 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    drawSphere(0.16f, 10, 8);
    glDisable(GL_BLEND);
    glPopMatrix();
    glEnable(GL_LIGHTING);

    float mastX2[] = { -0.55f, -0.33f };
    float mastH2[] = { 2.20f, 1.70f };
    float mastR2[] = { 0.030f, 0.025f };
    for (int m = 0; m < 2; m++) {
        glPushMatrix();
        glTranslatef(mastX2[m], hullR + 0.40f, 0);
        glColor3f(0.13f, 0.13f, 0.14f);
        drawCylinder(mastR2[m], mastH2[m], 8);
        glTranslatef(0, mastH2[m], 0);
        glColor3f(0.16f, 0.16f, 0.17f);
        drawSphere(mastR2[m] + 0.01f, 8, 6);
        glPopMatrix();
    }

    glPushMatrix();
    glTranslatef(3.72f, 0.02f, 0);
    glColor3f(0.62f, 0.63f, 0.65f);
    glScalef(0.20f, 0.20f, 1.10f);
    drawCube(1.0f);
    glPopMatrix();
    glDisable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(3.84f, 0.02f, 0);
    float boost = sub.headlightsOn ? 1.0f : 0.55f;
    glColor3f(1.0f * boost, 0.28f * boost, 0.10f * boost);
    glScalef(0.08f, 0.10f, 0.96f);
    drawCube(1.0f);
    glPopMatrix();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glPushMatrix();
    glTranslatef(3.86f, 0.02f, 0);
    glColor4f(1.0f, 0.30f, 0.12f, 0.45f * boost);
    glScalef(0.14f, 0.20f, 1.15f);
    drawSphere(1.0f, 12, 8);
    glPopMatrix();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix();
        glTranslatef(-3.75f, 0.0f, s * 0.45f);
        glRotatef(14, 0, 0, s);
        glColor3fv(hullDark);
        glScalef(0.70f, 0.05f, 0.60f);
        drawCube(1.0f);
        glPopMatrix();
    }
    glPushMatrix();
    glTranslatef(-3.80f, 0.45f, 0);
    glRotatef(18, 0, 0, 1);
    glColor3fv(hullDark);
    glScalef(0.70f, 0.75f, 0.05f);
    drawCube(1.0f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-3.80f, -0.38f, 0);
    glRotatef(-18, 0, 0, 1);
    glColor3fv(hullDark);
    glScalef(0.65f, 0.60f, 0.05f);
    drawCube(1.0f);
    glPopMatrix();
}

void drawPropeller() {
    // 7-blade bronze screw like ref propeller detail + hub
    glPushMatrix();
    glTranslatef(-4.35f, 0.0f, 0.0f);
    glRotatef(sub.propellerAngle, 1, 0, 0);

    glColor3f(0.32f, 0.26f, 0.14f);
    drawSphere(0.12f, 10, 8);
    glPushMatrix();
    glTranslatef(-0.10f, 0, 0);
    glRotatef(90, 0, 1, 0);
    drawCone(0.12f, 0.22f, 10);
    glPopMatrix();
    // 7 skewed blades
    for (int i = 0; i < 7; i++) {
        glPushMatrix();
        glRotatef(i * (360.0f / 7.0f), 1, 0, 0);
        glTranslatef(0, 0.32f, 0);
        glRotatef(32, 0, 0, 1);
        glRotatef(18, 1, 0, 0);
        glColor3f(0.36f, 0.29f, 0.15f);
        glScalef(0.05f, 0.36f, 0.17f);
        drawCube(1.0f);
        glPopMatrix();
    }
    glPopMatrix();
    // Shaft (stern taper -> propeller hub)
    glPushMatrix();
    glTranslatef(-4.55f, 0, 0);
    glRotatef(90, 0, 1, 0);
    glColor3f(0.20f, 0.20f, 0.22f);
    drawCylinder(0.07f, 0.50f, 10);
    glPopMatrix();
}

void setupSubHeadlight() {
    if (!sub.headlightsOn) { glDisable(GL_LIGHT2); return; }
    float yawRad = sub.yaw * DEG_TO_RAD;
    float pitchRad = sub.pitch * DEG_TO_RAD;
    float cp = cos(pitchRad);
    float fx = cos(yawRad) * cp;
    float fy = sin(pitchRad);
    float fz = -sin(yawRad) * cp;
    float bx = sub.x + fx * BOW_TIP;
    float by = sub.y + fy * BOW_TIP;
    float bz = sub.z + fz * BOW_TIP;
    GLfloat pos2[] = { bx, by, bz, 1.0f };
    GLfloat dir2[] = { fx, fy, fz };
    GLfloat amb2[] = { 0.15f, 0.15f, 0.10f, 1.0f };
    GLfloat diff2[] = { 1.0f, 0.95f, 0.80f, 1.0f };
    GLfloat spec2[] = { 1.0f, 1.0f, 0.90f, 1.0f };
    glEnable(GL_LIGHT2);
    glLightfv(GL_LIGHT2, GL_POSITION, pos2);
    glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, dir2);
    glLightfv(GL_LIGHT2, GL_AMBIENT, amb2);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, diff2);
    glLightfv(GL_LIGHT2, GL_SPECULAR, spec2);
    glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 28.0f);
    glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 10.0f);
    glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, 0.4f);
    glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, 0.05f);
    glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 0.008f);
}

void drawHeadlightBeam(float lx, float ly, float lz, float len, float farR) {
    glPushMatrix();
    glTranslatef(lx, ly, lz);
    glRotatef(90, 0, 1, 0);
    GLUquadric* q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);
    gluCylinder(q, 0.12f, farR, len, 14, 1);
    gluDeleteQuadric(q);
    glPopMatrix();
}

void drawSubmarineHeadlights() {
    if (!sub.headlightsOn) return;
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(1.0f, 0.45f, 0.20f, 0.14f);
    drawHeadlightBeam(3.88f, 0.02f, 0.0f, 9.0f, 2.0f);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawFullSubmarine() {
    glPushMatrix();
    glTranslatef(sub.x, sub.y, sub.z);
    glRotatef(sub.yaw, 0, 1, 0);
    glRotatef(sub.pitch, 0, 0, 1);
    glRotatef(sub.roll, 1, 0, 0);
    glScalef(SUB_SCALE * SUB_LEN, SUB_SCALE, SUB_SCALE);

    bool hullTex = (texHull != 0);
    if (hullTex) { glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, texHull); }
    drawSubmarineBody();
    if (hullTex) { glBindTexture(GL_TEXTURE_2D, 0); glDisable(GL_TEXTURE_2D); }
    drawPropeller();
    drawSubmarineHeadlights();

    glPopMatrix();
}

// ============================================================
// DRAW ENVIRONMENT
// ============================================================
void drawSky() {
    // Day sky MUST be vertical walls (old flat XZ quads are edge-on
    // from a low camera and invisible). Sunset left, blue right.
    float diveBlend = 1.0f - diveTransition;
    if (diveBlend <= 0) return;

    glDisable(GL_LIGHTING);
    if (texSky) {
        // Realistic gradient dome (textured, surrounds the whole scene)
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texSky);
        glColor3f(diveBlend, diveBlend, diveBlend);
        GLUquadric* skyQ = gluNewQuadric();
        gluQuadricTexture(skyQ, GL_TRUE);
        gluQuadricNormals(skyQ, GLU_NONE);
        glPushMatrix();
        glRotatef(-90, 1, 0, 0); // put texture horizon at eye level
        gluSphere(skyQ, 230, 28, 18);
        glPopMatrix();
        gluDeleteQuadric(skyQ);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    } else {
        // Fallback flat sky if textures unavailable
        glDisable(GL_DEPTH_TEST);
        glBegin(GL_QUADS);
        glColor3f(0.25f, 0.50f, 0.95f); glVertex3f(-120, 45, -80);
        glColor3f(0.25f, 0.50f, 0.95f); glVertex3f(120, 45, -80);
        glColor3f(1.00f, 0.62f, 0.30f); glVertex3f(120, 0, -80);
        glColor3f(1.00f, 0.62f, 0.30f); glVertex3f(-120, 0, -80);
        glEnd();
        glEnable(GL_DEPTH_TEST);
    }
    glEnable(GL_LIGHTING);
}

void drawSun() {
    // Low sunset sun on left like ref, not high noon
    float diveBlend = 1.0f - diveTransition;
    if (diveBlend <= 0) return;

    glDisable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(-38.0f, 5.5f, -45.0f);
    glColor3f(1.0f * diveBlend, 0.88f * diveBlend, 0.66f * diveBlend);
    drawSphere(2.6f, 18, 14);
    glPopMatrix();
    // Layered photographic glow sprites around the sun disc
    if (texPuff) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texPuff);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(1.0f, 0.85f, 0.55f, 0.55f * diveBlend);
        drawBillboard(-38.0f, 5.5f, -45.0f, 16.0f, 16.0f);
        glColor4f(1.0f, 0.60f, 0.28f, 0.30f * diveBlend);
        drawBillboard(-38.0f, 5.5f, -45.0f, 30.0f, 30.0f);
        glColor4f(1.0f, 0.45f, 0.22f, 0.14f * diveBlend);
        drawBillboard(-38.0f, 5.5f, -45.0f, 55.0f, 55.0f);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
    }
    // Sun glitter path on water toward camera
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBegin(GL_QUADS);
    glColor4f(1.0f, 0.6f, 0.3f, 0.18f * diveBlend);
    glVertex3f(-38, 0.15f, -40);
    glVertex3f(-28, 0.15f, -40);
    glVertex3f(-15, 0.15f, 10);
    glVertex3f(-25, 0.15f, 10);
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawDistantHills() {
    // Low brown-green coastline on horizon like 1st ref
    float diveBlend = 1.0f - diveTransition;
    if (diveBlend <= 0) return;
    glDisable(GL_LIGHTING);
    glPushMatrix();
    // Far hazy layer (atmospheric perspective: pale blue-violet)
    glColor3f(0.55f * diveBlend + 0.1f, 0.52f * diveBlend + 0.1f, 0.62f * diveBlend + 0.08f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(-85, 0, -62);
    for (int i = 0; i <= 14; i++) {
        float t = i / 14.0f;
        float hx = -85 + t * 170.0f;
        float hy = 3.2f + sin(t * 17.0f) * 1.1f + sin(t * 41.0f) * 0.5f;
        glVertex3f(hx, hy, -62);
    }
    glEnd();
    // Near left headland: brown-green with ridge variation
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(-80, 0, -55);
    for (int i = 0; i <= 12; i++) {
        float t = i / 12.0f;
        float hx = -80 + t * 58.0f;
        float hy = 2.2f + sin(t * 9.0f) * 1.0f + sin(t * 23.0f) * 0.45f;
        float shade = 0.85f + 0.15f * sin(t * 31.0f);
        glColor3f((0.30f * diveBlend + 0.05f) * shade,
                  (0.26f * diveBlend + 0.04f) * shade,
                  (0.15f * diveBlend + 0.02f) * shade);
        glVertex3f(hx, hy, -55);
    }
    glEnd();
    // Near right point: darker green slope
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(8, 0, -57);
    for (int i = 0; i <= 10; i++) {
        float t = i / 10.0f;
        float hx = 8 + t * 72.0f;
        float hy = 1.7f + sin(t * 12.0f) * 0.7f + sin(t * 29.0f) * 0.3f;
        float shade = 0.85f + 0.15f * sin(t * 27.0f + 2.0f);
        glColor3f((0.22f * diveBlend + 0.04f) * shade,
                  (0.30f * diveBlend + 0.04f) * shade,
                  (0.22f * diveBlend + 0.03f) * shade);
        glVertex3f(hx, hy, -57);
    }
    glEnd();
    // Vegetation speckle dots on near hills
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 120; i++) {
        float t = (i * 0.731f) - (int)(i * 0.731f);
        float hx = -78 + t * 120.0f;
        float hh = 0.4f + ((i * 37) % 100) / 100.0f * 1.6f;
        glColor3f(0.16f * diveBlend + 0.03f, 0.28f * diveBlend + 0.03f, 0.12f * diveBlend + 0.02f);
        glVertex3f(hx, hh, -54.5f);
    }
    glEnd();
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawBowFoam() {
    // White breaking wave + foam collar around surfaced hull, like ref
    float diveBlend = 1.0f - diveTransition;
    if (diveBlend <= 0.02f) return;
    if (sub.y < -0.5f) return; // only on surface
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float t = introTimer * 0.004f;
    // Foam collar: flattened white blobs along waterline
    for (int i = 0; i < 26; i++) {
        float fx = 3.4f - i * 0.32f + sin(t * 2.0f + i * 1.7f) * 0.12f;
        float fzSide = (i % 2 == 0) ? 1.0f : -1.0f;
        float fz = fzSide * (0.95f + sin(t * 3.0f + i) * 0.15f + (rand() % 10) * 0.004f);
        float fs = 0.28f + (i % 5) * 0.07f + sin(t * 4.0f + i * 2.3f) * 0.06f;
        glPushMatrix();
        glTranslatef(fx, 0.12f, fz);
        glColor4f(0.95f, 0.97f, 1.0f, 0.75f * diveBlend);
        glScalef(fs * 1.6f, fs * 0.35f, fs);
        drawSphere(1.0f, 8, 6);
        glPopMatrix();
    }
    // Bow breaking crest
    glPushMatrix();
    glTranslatef(3.6f, 0.25f, 0);
    glColor4f(1, 1, 1, 0.85f * diveBlend);
    glScalef(0.7f, 0.30f, 1.3f);
    drawSphere(1.0f, 10, 8);
    glPopMatrix();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawDeckCrewSilhouettes() {
    // Tiny black crew figures on sail + deck, like 1st ref
    float diveBlend = 1.0f - diveTransition;
    if (diveBlend <= 0.02f) return;
    if (sub.y < -0.5f) return;
    glDisable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(sub.x, sub.y, sub.z);
    glRotatef(sub.yaw, 0, 1, 0);
    glColor3f(0.02f, 0.02f, 0.03f);
    // 5 figures on deck aft of sail
    float deckX[] = { -0.6f, -0.9f, -1.2f, -1.5f, -1.8f };
    for (int i = 0; i < 5; i++) {
        glPushMatrix();
        glTranslatef(deckX[i], 1.02f, 0.02f);
        drawSphere(0.07f, 6, 5); // head
        glTranslatef(0, -0.16f, 0);
        glScalef(0.09f, 0.26f, 0.09f);
        drawCube(1.0f); // body
        glPopMatrix();
    }
    // 2 figures on top of sail
    for (int i = 0; i < 2; i++) {
        glPushMatrix();
        glTranslatef(0.35f + i * 0.25f, 2.35f, 0);
        drawSphere(0.06f, 6, 5);
        glTranslatef(0, -0.14f, 0);
        glScalef(0.08f, 0.22f, 0.08f);
        drawCube(1.0f);
        glPopMatrix();
    }
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawClouds() {
    float diveBlend = 1.0f - diveTransition;
    if (diveBlend <= 0) return;

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (texPuff) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texPuff);
        for (size_t i = 0; i < clouds.size(); i++) {
            float cx = clouds[i].x + sin(introTimer * 0.001f + i) * 0.5f;
            float cy = clouds[i].y, cz = clouds[i].z;
            float warm = 1.0f - (cx + 30.0f) / 60.0f;
            if (warm < 0) warm = 0; if (warm > 1) warm = 1;
            float s = clouds[i].scale;
            // Soft shadow base
            glColor4f(0.52f + warm * 0.22f, 0.48f + warm * 0.14f, 0.55f, 0.50f * diveBlend);
            drawBillboard(cx, cy - s * 0.25f, cz, s * 5.2f, s * 1.5f);
            // Warm sunlit body
            glColor4f(1.0f, 0.74f + warm * 0.08f, 0.58f + warm * 0.12f, 0.60f * diveBlend);
            drawBillboard(cx, cy, cz, s * 4.6f, s * 1.7f);
            // Bright top edge
            glColor4f(1.0f, 0.96f, 0.92f, 0.65f * diveBlend);
            drawBillboard(cx - s * 0.3f, cy + s * 0.45f, cz, s * 3.0f, s * 0.9f);
            // Flanking puffs
            glColor4f(1.0f, 0.84f, 0.70f, 0.5f * diveBlend);
            drawBillboard(cx + s * 1.8f, cy - s * 0.1f, cz, s * 2.0f, s * 0.9f);
            drawBillboard(cx - s * 2.0f, cy - s * 0.1f, cz, s * 2.2f, s * 1.0f);
        }
        // Thin pink cirrus streaks near horizon
        glColor4f(1.0f, 0.62f, 0.48f, 0.28f * diveBlend);
        for (int k = 0; k < 5; k++) {
            float ky = 9.0f + k * 1.8f;
            drawBillboard(-10.0f + k * 4.0f, ky, -55.0f, 26.0f - k * 2.5f, 1.1f);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    }
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void drawOceanSurface() {
    float diveBlend = 1.0f - diveTransition;

    glPushMatrix();
    // Water surface with wave effect
    int gridSize = 40;
    float gridStep = 2.0f;
    float surfaceY = 0.0f;

    // Water is hand-shaded (no GL lighting: it has no normals) so the
    // gradient + sun glitter read photographic, with a drifting detail texture.
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    bool waterTex = (texWater != 0);
    if (waterTex) { glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, texWater); }
    float uvDrift = introTimer * 0.00003f;

    glBegin(GL_QUADS);
    for (int i = -gridSize; i < gridSize; i++) {
        for (int j = -gridSize; j < gridSize; j++) {
            float x0 = i * gridStep;
            float z0 = j * gridStep;
            float x1 = (i + 1) * gridStep;
            float z1 = (j + 1) * gridStep;

            float w00 = sin(x0 * 0.35f + introTimer * 0.0022f) * 0.16f + cos(z0 * 0.23f + introTimer * 0.0016f) * 0.11f + sin((x0 + z0) * 0.12f + introTimer * 0.0009f) * 0.08f;
            float w10 = sin(x1 * 0.35f + introTimer * 0.0022f) * 0.16f + cos(z0 * 0.23f + introTimer * 0.0016f) * 0.11f + sin((x1 + z0) * 0.12f + introTimer * 0.0009f) * 0.08f;
            float w01 = sin(x0 * 0.35f + introTimer * 0.0022f) * 0.16f + cos(z1 * 0.23f + introTimer * 0.0016f) * 0.11f + sin((x0 + z1) * 0.12f + introTimer * 0.0009f) * 0.08f;
            float w11 = sin(x1 * 0.35f + introTimer * 0.0022f) * 0.16f + cos(z1 * 0.23f + introTimer * 0.0016f) * 0.11f + sin((x1 + z1) * 0.12f + introTimer * 0.0009f) * 0.08f;

            float waterAlpha = 0.55f * diveBlend + 0.45f;
            // Natural day water: deep blue-green troughs, lighter crests
            float hAvg = (w00 + w11) * 0.5f; // -0.35..0.35
            float crest = (hAvg + 0.35f) / 0.7f; // 0..1
            float r = (0.04f + crest * 0.10f) * diveBlend + 0.01f;
            float g = (0.24f + crest * 0.16f) * diveBlend + 0.05f;
            float b = (0.44f + crest * 0.20f) * diveBlend + 0.14f;
            // Sun glitter lane near sunset side (-X, toward sun)
            float lane = 1.0f - fabs((x0 + 26.0f) / 22.0f);
            if (lane < 0) lane = 0;
            float glint = pow(sin(x0 * 2.1f + introTimer * 0.01f) * sin(z0 * 1.7f - introTimer * 0.008f), 8.0f);
            if (glint < 0) glint = 0;
            r += lane * (0.25f + glint * 0.6f) * diveBlend;
            g += lane * (0.15f + glint * 0.4f) * diveBlend;
            b += lane * (0.05f + glint * 0.2f) * diveBlend;

            glColor4f(r, g, b, waterAlpha);
            if (waterTex) glTexCoord2f(x0 * 0.06f + uvDrift, z0 * 0.06f);
            glVertex3f(x0, surfaceY + w00, z0);
            if (waterTex) glTexCoord2f(x1 * 0.06f + uvDrift, z0 * 0.06f);
            glVertex3f(x1, surfaceY + w10, z0);
            if (waterTex) glTexCoord2f(x1 * 0.06f + uvDrift, z1 * 0.06f);
            glVertex3f(x1, surfaceY + w11, z1);
            if (waterTex) glTexCoord2f(x0 * 0.06f + uvDrift, z1 * 0.06f);
            glVertex3f(x0, surfaceY + w01, z1);
        }
    }
    glEnd();
    if (waterTex) { glBindTexture(GL_TEXTURE_2D, 0); glDisable(GL_TEXTURE_2D); }

    // Natural seabed: textured rippled sand, properly lit with normals,
    // plus drifting caustic light patches
    float floorY = -16.0f;
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    bool sandTex = (texSand != 0);
    if (sandTex) { glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, texSand); }
    glColor3f(1.0f, 0.96f, 0.88f);
    glBegin(GL_QUADS);
    for (int i = -gridSize; i < gridSize; i++) {
        for (int j = -gridSize; j < gridSize; j++) {
            float x0 = i * gridStep;
            float z0 = j * gridStep;
            float x1 = (i + 1) * gridStep;
            float z1 = (j + 1) * gridStep;
            float h = sin(x0 * 0.45f) * cos(z0 * 0.38f) * 0.45f + sin((x0 + z0) * 0.15f) * 0.25f;
            float ca = sin(x0 * 0.8f + introTimer * 0.0012f) * cos(z0 * 0.7f - introTimer * 0.001f);
            ca = ca * ca * (1.0f - diveTransition * 0.7f); // caustics fade with depth
            float sandTone = 0.9f + h * 0.25f;
            glColor3f((0.72f * sandTone + ca * 0.28f), (0.64f * sandTone + ca * 0.24f), (0.47f * sandTone + ca * 0.15f));
            glNormal3f(0, 1, 0);
            if (sandTex) glTexCoord2f(x0 * 0.08f, z0 * 0.08f);
            glVertex3f(x0, floorY + h, z0);
            if (sandTex) glTexCoord2f(x1 * 0.08f, z0 * 0.08f);
            glVertex3f(x1, floorY + sin(x1 * 0.5f) * cos(z0 * 0.4f) * 0.3f, z0);
            if (sandTex) glTexCoord2f(x1 * 0.08f, z1 * 0.08f);
            glVertex3f(x1, floorY + sin(x1 * 0.5f) * cos(z1 * 0.4f) * 0.3f, z1);
            if (sandTex) glTexCoord2f(x0 * 0.08f, z1 * 0.08f);
            glVertex3f(x0, floorY + h, z1);
        }
    }
    glEnd();
    if (sandTex) { glBindTexture(GL_TEXTURE_2D, 0); glDisable(GL_TEXTURE_2D); }
    glPopMatrix();
}

void drawTrees() {
    float diveBlend = 1.0f - diveTransition;
    if (diveBlend <= 0) return;

    for (size_t i = 0; i < trees.size(); i++) {
        glPushMatrix();
        glTranslatef(trees[i].x, 0, trees[i].z);

        // Trunk
        glColor3f(0.4f * diveBlend, 0.25f * diveBlend, 0.1f * diveBlend);
        drawCylinder(0.15f, trees[i].height, 8);

        // Leaves
        glTranslatef(0, trees[i].height, 0);
        glColor3f(0.1f * diveBlend, 0.5f * diveBlend, 0.1f * diveBlend);
        drawSphere(0.6f, 8, 6);

        glPopMatrix();
    }
}

void drawSeaweed() {
    for (size_t i = 0; i < seaweeds.size(); i++) {
        glPushMatrix();
        float sway = sin(introTimer * 0.003f + seaweeds[i].phase) * 0.2f;
        glTranslatef(seaweeds[i].x, -16.0f, seaweeds[i].z);

        int segments = 8;
        float segH = seaweeds[i].height / segments;
        for (int s = 0; s < segments; s++) {
            float swayAmount = sway * ((float)s / segments);
            glTranslatef(swayAmount * 0.05f, segH, 0);
            glColor3f(0.05f, 0.3f + s * 0.03f, 0.05f);
            glScalef(1.0f, 1.0f, 1.0f);
            drawCylinder(0.04f - s * 0.003f, segH, 6);
        }
        glPopMatrix();
    }
}

void drawRocks() {
    for (size_t i = 0; i < rocks.size(); i++) {
        glPushMatrix();
        glTranslatef(rocks[i].x, rocks[i].y, rocks[i].z);
        glScalef(rocks[i].scaleX, rocks[i].scaleY, rocks[i].scaleZ);
        glColor3f(rocks[i].r, rocks[i].g, rocks[i].b);
        drawSphere(1.0f, 8, 6);
        glPopMatrix();
    }
}

void drawCoral() {
    for (size_t i = 0; i < corals.size(); i++) {
        glPushMatrix();
        glTranslatef(corals[i].x, corals[i].y, corals[i].z);

        glColor3f(corals[i].r, corals[i].g, corals[i].b);
        float s = corals[i].size;

        if (corals[i].type == 0) {
            // Branching coral
            for (int b = 0; b < 5; b++) {
                glPushMatrix();
                float angle = b * 72.0f * DEG_TO_RAD;
                glTranslatef(cos(angle) * 0.2f * s, 0, sin(angle) * 0.2f * s);
                glRotatef(20 + b * 10, 0, 0, 1);
                drawCylinder(0.04f * s, 0.5f * s, 6);
                glTranslatef(0, 0.5f * s, 0);
                drawSphere(0.06f * s, 6, 4);
                glPopMatrix();
            }
        } else if (corals[i].type == 1) {
            // Fan coral
            glScalef(s, s, 0.1f * s);
            drawSphere(0.5f, 8, 6);
        } else {
            // Brain coral
            drawSphere(0.4f * s, 10, 8);
            glColor3f(corals[i].r * 0.8f, corals[i].g * 0.8f, corals[i].b * 0.8f);
            drawSphere(0.35f * s, 8, 6);
        }
        glPopMatrix();
    }
}

void drawFish() {
    float t = introTimer * 0.001f;

    for (size_t i = 0; i < fishes.size(); i++) {
        Fish& f = fishes[i];
        glPushMatrix();
        glTranslatef(f.x, f.y, f.z);

        float displayAngle = f.angle * 180.0f / PI;
        glRotatef(displayAngle, 0, 1, 0);

        float tailWag = sin(t * 10 + f.animPhase) * 15.0f;

        if (f.type == 0 || f.type == 1) {
            // Regular fish
            float s = f.size;
            glColor3f(f.r, f.g, f.b);
            // Body
            glScalef(1.0f, 0.6f, 0.5f);
            drawSphere(s, 8, 6);

            // Tail
            glPushMatrix();
            glTranslatef(-s * 1.2f, 0, 0);
            glRotatef(tailWag, 0, 1, 0);
            glColor3f(f.r * 0.8f, f.g * 0.8f, f.b * 0.8f);
            glScalef(0.6f, 0.8f, 0.3f);
            drawCone(s * 0.5f, s * 0.6f, 6);
            glPopMatrix();

            // Eye
            glPushMatrix();
            glTranslatef(s * 0.6f, s * 0.15f, s * 0.2f);
            glColor3f(1, 1, 1);
            drawSphere(s * 0.15f, 6, 4);
            glColor3f(0, 0, 0);
            drawSphere(s * 0.08f, 4, 4);
            glPopMatrix();
        } else if (f.type == 2) {
            // Jellyfish
            float s = f.size;
            float pulse = sin(t * 3 + f.animPhase) * 0.15f;

            // Bell/dome
            glColor4f(f.r, f.g, f.b, 0.6f);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glScalef(1.0f + pulse, 0.7f, 1.0f + pulse);
            drawSphere(s, 10, 8);

            // Tentacles
            glColor4f(f.r * 0.8f, f.g * 0.8f, f.b, 0.4f);
            for (int t2 = 0; t2 < 6; t2++) {
                float angle = t2 * 60.0f * DEG_TO_RAD;
                glPushMatrix();
                glTranslatef(cos(angle) * s * 0.3f, -s * 0.3f, sin(angle) * s * 0.3f);
                float tentSway = sin(t * 2 + t2) * 10.0f;
                glRotatef(tentSway, 0, 0, 1);
                drawCylinder(0.01f * s, s * 0.8f, 4);
                glPopMatrix();
            }
            glDisable(GL_BLEND);
        }
        glPopMatrix();
    }
}

void drawSharks() {
    for (size_t i = 0; i < sharks.size(); i++) {
        Shark& s = sharks[i];
        float sx = s.cx + cos(s.angle) * s.radius;
        float sz = s.cz + sin(s.angle) * s.radius;
        glPushMatrix();
        glTranslatef(sx, s.cy, sz);
        glRotatef(-(s.angle * 180.0f / PI) + 90.0f, 0, 1, 0);
        float tailWag = sin(introTimer * 0.004f + i * 2.0f) * 12.0f;
        glColor3f(0.45f, 0.52f, 0.58f);
        glScalef(s.size * 1.6f, s.size * 0.42f, s.size * 0.5f);
        drawSphere(1.0f, 12, 8);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(sx, s.cy, sz);
        glRotatef(-(s.angle * 180.0f / PI) + 90.0f, 0, 1, 0);
        glColor3f(0.42f, 0.49f, 0.55f);
        glPushMatrix();
        glTranslatef(-s.size * 1.7f, s.size * 0.1f, 0);
        glRotatef(tailWag, 0, 1, 0);
        glScalef(s.size * 0.5f, s.size * 0.55f, s.size * 0.08f);
        drawCube(1.0f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(-s.size * 0.1f, s.size * 0.55f, 0);
        glScalef(s.size * 0.45f, s.size * 0.55f, s.size * 0.08f);
        drawCube(1.0f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(s.size * 0.35f, -s.size * 0.25f, s.size * 0.35f);
        glRotatef(25, 1, 0, 0);
        glScalef(s.size * 0.55f, s.size * 0.08f, s.size * 0.3f);
        drawCube(1.0f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(s.size * 0.35f, -s.size * 0.25f, -s.size * 0.35f);
        glRotatef(-25, 1, 0, 0);
        glScalef(s.size * 0.55f, s.size * 0.08f, s.size * 0.3f);
        drawCube(1.0f);
        glPopMatrix();
        glPopMatrix();
    }
}

void drawRays() {
    for (size_t i = 0; i < rays.size(); i++) {
        Ray& r = rays[i];
        float rx = r.cx + cos(r.angle) * r.radius;
        float rz = r.cz + sin(r.angle) * r.radius;
        float flap = sin(introTimer * 0.003f + r.phase) * 0.12f;
        glPushMatrix();
        glTranslatef(rx, r.cy + flap * 2.0f, rz);
        glRotatef(-(r.angle * 180.0f / PI) + 90.0f, 0, 1, 0);
        glColor3f(0.25f, 0.30f, 0.36f);
        glScalef(r.size * 1.1f, r.size * 0.12f + fabs(flap) * 0.4f, r.size * 0.95f);
        drawSphere(1.0f, 12, 8);
        glColor3f(0.20f, 0.24f, 0.30f);
        glPushMatrix();
        glTranslatef(-r.size * 1.2f, 0, 0);
        glRotatef(90, 0, 1, 0);
        drawCylinder(0.02f * r.size, r.size * 1.6f, 6);
        glPopMatrix();
        glPopMatrix();
    }
}

void drawKelp() {
    float t = introTimer * 0.002f;
    for (size_t i = 0; i < kelp.size(); i++) {
        Kelp& k = kelp[i];
        float lean = sin(t + k.phase) * 4.0f;
        glPushMatrix();
        glTranslatef(k.x, -16.0f, k.z);
        glRotatef(lean, 0, 0, 1);
        glColor3f(0.20f, 0.30f, 0.06f);
        drawCylinder(0.09f, k.h, 6);
        for (int L = 0; L < 3; L++) {
            float ly = k.h * (0.30f + 0.22f * L);
            for (int s = -1; s <= 1; s += 2) {
                glPushMatrix();
                glTranslatef(0, ly, 0);
                glRotatef(s * (32.0f + L * 5.0f), 0, 0, 1);
                glRotatef((L * 47 + s * 30), 0, 1, 0);
                if ((L + s) % 2 == 0) glColor3f(0.42f, 0.46f, 0.09f);
                else glColor3f(0.28f, 0.38f, 0.07f);
                glScalef(1.0f, 1.0f, 0.25f);
                drawCone(0.08f, 0.7f + L * 0.1f, 5);
                glPopMatrix();
            }
        }
        glPushMatrix();
        glTranslatef(0, k.h, 0);
        glColor3f(0.46f, 0.44f, 0.09f);
        drawCone(0.09f, 0.9f, 5);
        glPopMatrix();
        glPopMatrix();
    }
}

void drawBubbles() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);

    for (size_t i = 0; i < bubbles.size(); i++) {
        Bubble& b = bubbles[i];
        glPushMatrix();
        glTranslatef(b.x + sin(b.wobblePhase) * b.wobble, b.y, b.z);
        glColor4f(0.5f, 0.7f, 1.0f, 0.3f);
        drawSphere(b.size, 8, 6);
        // Highlight
        glTranslatef(b.size * 0.3f, b.size * 0.3f, b.size * 0.3f);
        glColor4f(1, 1, 1, 0.4f);
        drawSphere(b.size * 0.25f, 4, 4);
        glPopMatrix();
    }

    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
}

void drawParticles() {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (size_t i = 0; i < particles.size(); i++) {
        Particle& p = particles[i];
        glPushMatrix();
        glTranslatef(p.x, p.y, p.z);
        glColor4f(0.7f, 0.8f, 0.9f, p.alpha * diveTransition);
        glutSolidSphere(p.size, 4, 3);
        glPopMatrix();
    }

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawLightRays() {
    // Underwater god-rays like 2nd ref: slanted translucent shafts
    // from the bright surface, swaying gently. Additive blending.
    if (diveTransition < 0.25f) return;
    float strength = (diveTransition - 0.25f) / 0.75f;
    if (strength > 1) strength = 1;
    // Fade rays as the boat goes very deep
    float depthFade = 1.0f - fabs(sub.depth - 8.0f) / 25.0f;
    if (depthFade < 0.15f) depthFade = 0.15f;
    float a = 0.04f * strength * depthFade;

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    float t = introTimer * 0.0006f;
    for (int i = 0; i < 7; i++) {
        float rx = -14.0f + i * 5.5f + sin(t * 1.3f + i * 2.1f) * 1.2f;
        float rz = -12.0f + (i % 3) * 7.0f;
        float topW = 1.1f + (i % 3) * 0.4f;
        float botW = 2.6f + (i % 4) * 0.6f;
        float sway = sin(t * 0.9f + i) * 1.5f;
        glBegin(GL_QUADS);
        glColor4f(0.45f, 0.75f, 0.95f, 0.0f);
        glVertex3f(rx - topW, 0.5f, rz);
        glVertex3f(rx + topW, 0.5f, rz);
        glColor4f(0.45f, 0.75f, 0.95f, a);
        glVertex3f(rx + botW + sway, -16.0f, rz);
        glVertex3f(rx - botW + sway, -16.0f, rz);
        glEnd();
    }
    // Bright surface sheet seen from below
    glBegin(GL_QUADS);
    glColor4f(0.35f, 0.65f, 0.9f, 0.35f * strength);
    glVertex3f(-40, 0.4f, -40);
    glVertex3f(40, 0.4f, -40);
    glVertex3f(40, 0.4f, 40);
    glVertex3f(-40, 0.4f, 40);
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

// ============================================================
// DRAW INTERIOR
// ============================================================
void drawInterior() {
    // Main room walls
    glPushMatrix();

    // Floor
    glColor3f(0.25f, 0.28f, 0.3f);
    glTranslatef(0, -0.8f, 0);
    glScalef(3.0f, 0.05f, 2.5f);
    drawCube(1.0f);
    glPopMatrix();

    // Ceiling
    glPushMatrix();
    glTranslatef(0, 1.2f, 0);
    glColor3f(0.22f, 0.25f, 0.28f);
    glScalef(3.0f, 0.05f, 2.5f);
    drawCube(1.0f);
    glPopMatrix();

    // Left wall
    glPushMatrix();
    glTranslatef(0, 0.2f, 1.25f);
    glColor3f(0.3f, 0.33f, 0.35f);
    glScalef(3.0f, 2.0f, 0.05f);
    drawCube(1.0f);
    glPopMatrix();

    // Right wall
    glPushMatrix();
    glTranslatef(0, 0.2f, -1.25f);
    glColor3f(0.3f, 0.33f, 0.35f);
    glScalef(3.0f, 2.0f, 0.05f);
    drawCube(1.0f);
    glPopMatrix();

    // Back wall
    glPushMatrix();
    glTranslatef(-1.5f, 0.2f, 0);
    glColor3f(0.28f, 0.3f, 0.33f);
    glScalef(0.05f, 2.0f, 2.5f);
    drawCube(1.0f);
    glPopMatrix();

    // Front wall with windows
    glPushMatrix();
    glTranslatef(1.5f, 0.2f, 0);
    glColor3f(0.28f, 0.3f, 0.33f);
    glScalef(0.05f, 2.0f, 2.5f);
    drawCube(1.0f);
    glPopMatrix();

    // Main control console (front)
    glPushMatrix();
    glTranslatef(1.3f, -0.3f, 0);
    glColor3f(0.2f, 0.22f, 0.25f);
    glScalef(0.3f, 0.8f, 1.8f);
    drawCube(1.0f);
    glPopMatrix();

    // Control panel screen
    glPushMatrix();
    glTranslatef(1.42f, 0.0f, 0);
    glColor3f(0.0f, 0.15f, 0.1f);
    glScalef(0.02f, 0.4f, 1.0f);
    drawCube(1.0f);
    glPopMatrix();

    // Screen glow
    glPushMatrix();
    glTranslatef(1.42f, 0.0f, 0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(0.0f, 0.4f, 0.2f, 0.15f);
    glScalef(0.02f, 0.5f, 1.2f);
    drawCube(1.0f);
    glDisable(GL_BLEND);
    glPopMatrix();

    // Buttons on console
    float buttonColors[][3] = {{0.8,0.2,0.1},{0.2,0.8,0.2},{0.2,0.2,0.8},{0.8,0.8,0.1},{0.8,0.4,0.1}};
    for (int i = 0; i < 15; i++) {
        glPushMatrix();
        float bx = 1.42f;
        float by = -0.5f + (i / 5) * 0.15f;
        float bz = -0.6f + (i % 5) * 0.3f;
        glTranslatef(bx, by, bz);
        glColor3fv(buttonColors[i % 5]);
        drawSphere(0.03f, 6, 4);
        glPopMatrix();
    }

    // Side panels with instruments
    for (int side = 0; side < 2; side++) {
        float sz = side == 0 ? 1.2f : -1.2f;
        glPushMatrix();
        glTranslatef(0, 0, sz);
        glRotatef(side == 0 ? 180 : 0, 0, 1, 0);

        // Panel
        glColor3f(0.2f, 0.22f, 0.25f);
        glScalef(2.5f, 1.5f, 0.1f);
        drawCube(1.0f);
        glPopMatrix();

        // Gauges
        for (int g = 0; g < 3; g++) {
            glPushMatrix();
            glTranslatef(-0.5f + g * 0.6f, 0.1f, sz * 0.95f);
            glColor3f(0.1f, 0.1f, 0.12f);
            drawCylinder(0.1f, 0.02f, 12);
            glColor3f(0.8f, 0.2f, 0.1f);
            glRotatef(introTimer * 0.05f + g * 45, 0, 0, 1);
            glTranslatef(0, 0, 0.01f);
            glScalef(0.08f, 0.01f, 0.01f);
            drawCube(1.0f);
            glPopMatrix();
        }
    }

    // Pipes on ceiling
    for (int p = 0; p < 4; p++) {
        glPushMatrix();
        glTranslatef(-1.0f + p * 0.7f, 1.1f, 0);
        glColor3f(0.4f, 0.35f, 0.3f);
        glRotatef(90, 0, 0, 1);
        drawCylinder(0.04f, 2.5f, 8);
        glPopMatrix();
    }

    // Chairs/seats
    for (int s = 0; s < 3; s++) {
        glPushMatrix();
        glTranslatef(0.8f, -0.55f, -0.7f + s * 0.7f);
        // Seat
        glColor3f(0.15f, 0.15f, 0.18f);
        glScalef(0.25f, 0.05f, 0.25f);
        drawCube(1.0f);
        glPopMatrix();
        // Back
        glPushMatrix();
        glTranslatef(0.65f, -0.3f, -0.7f + s * 0.7f);
        glColor3f(0.15f, 0.15f, 0.18f);
        glScalef(0.05f, 0.4f, 0.25f);
        drawCube(1.0f);
        glPopMatrix();
    }

    // Draw crew members
    for (size_t i = 0; i < crew.size(); i++) {
        CrewMember& c = crew[i];
        glPushMatrix();
        glTranslatef(c.x, c.y, c.z);

        // Body
        glColor3f(0.2f, 0.3f, 0.5f);
        glScalef(0.15f, 0.25f, 0.1f);
        drawCube(1.0f);
        glPopMatrix();

        // Head
        glPushMatrix();
        float headBob = sin(introTimer * 0.002f + i * 2) * 0.02f;
        glTranslatef(c.x, c.y + 0.35f + headBob, c.z);
        glColor3f(0.85f, 0.7f, 0.6f);
        drawSphere(0.1f, 8, 6);

        // Cap
        glColor3f(0.1f, 0.1f, 0.3f);
        glTranslatef(0, 0.06f, 0);
        drawCylinder(0.1f, 0.04f, 8);
        glPopMatrix();

        // Arms animation
        float armAnim = sin(introTimer * 0.003f + i * 3) * 15.0f;
        // Left arm
        glPushMatrix();
        glTranslatef(c.x + 0.12f, c.y + 0.1f, c.z);
        glRotatef(armAnim, 0, 0, 1);
        glColor3f(0.2f, 0.3f, 0.5f);
        glScalef(0.04f, 0.2f, 0.04f);
        drawCube(1.0f);
        glPopMatrix();
        // Right arm
        glPushMatrix();
        glTranslatef(c.x - 0.12f, c.y + 0.1f, c.z);
        glRotatef(-armAnim, 0, 0, 1);
        glColor3f(0.2f, 0.3f, 0.5f);
        glScalef(0.04f, 0.2f, 0.04f);
        drawCube(1.0f);
        glPopMatrix();
    }

    // Portholes from inside
    for (int pw = 0; pw < 2; pw++) {
        float pz = pw == 0 ? 1.22f : -1.22f;
        float pAngle = pw == 0 ? 0 : 180;
        glPushMatrix();
        glTranslatef(0.3f, 0.2f, pz);
        glRotatef(pAngle, 0, 1, 0);

        // Window frame
        glColor3f(0.4f, 0.4f, 0.42f);
        drawCylinder(0.2f, 0.08f, 16);
        // Window glass
        glColor4f(0.1f, 0.3f, 0.5f, 0.5f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        drawDisk(0.0f, 0.18f, 16);
        glDisable(GL_BLEND);
        glPopMatrix();
    }
}

// ============================================================
// DRAW HUD
// ============================================================
void drawHUD() {
    if (gameState != STATE_PLAYING) return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    // HUD background panels
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Top-left info panel
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(60, windowHeight - 270);
    glVertex2f(270, windowHeight - 270);
    glVertex2f(270, windowHeight - 420);
    glVertex2f(60, windowHeight - 420);
    glEnd();

    // Border
    glColor4f(0.0f, 0.8f, 0.8f, 0.8f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(60, windowHeight - 270);
    glVertex2f(270, windowHeight - 270);
    glVertex2f(270, windowHeight - 420);
    glVertex2f(60, windowHeight - 420);
    glEnd();

    // Info text
    glColor3f(0.0f, 0.9f, 0.9f);
    char buf[128];

    sprintf(buf, "DEPTH: %.0f m", fabs(sub.depth));
    drawText(70, windowHeight - 294, buf, GLUT_BITMAP_HELVETICA_12);

    sprintf(buf, "SPEED: %.1f knots", sub.speed * 20.0f);
    drawText(70, windowHeight - 316, buf, GLUT_BITMAP_HELVETICA_12);

    sprintf(buf, "LIGHT: %s", sub.headlightsOn ? "ON" : "OFF");
    drawText(70, windowHeight - 338, buf, GLUT_BITMAP_HELVETICA_12);

    sprintf(buf, "ENGINE: %s", fabs(sub.speed) > 0.01f ? "ACTIVE" : "IDLE");
    drawText(70, windowHeight - 360, buf, GLUT_BITMAP_HELVETICA_12);

    sprintf(buf, "CAMERA: %s", cameraNames[cameraMode]);
    drawText(70, windowHeight - 382, buf, GLUT_BITMAP_HELVETICA_12);

    sprintf(buf, "MISSION: %s", missionNames[currentMission]);
    drawText(70, windowHeight - 404, buf, GLUT_BITMAP_HELVETICA_12);

    // Bottom center - mission timer
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(windowWidth / 2 - 100, 114);
    glVertex2f(windowWidth / 2 + 100, 114);
    glVertex2f(windowWidth / 2 + 100, 74);
    glVertex2f(windowWidth / 2 - 100, 74);
    glEnd();

    glColor4f(0.0f, 0.8f, 0.8f, 0.8f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(windowWidth / 2 - 100, 114);
    glVertex2f(windowWidth / 2 + 100, 114);
    glVertex2f(windowWidth / 2 + 100, 74);
    glVertex2f(windowWidth / 2 - 100, 74);
    glEnd();

    glColor3f(0.0f, 0.9f, 0.9f);
    int mins = (int)(missionTimer / 60.0f);
    int secs = (int)missionTimer % 60;
    sprintf(buf, "TIME: %02d:%02d", mins, secs);
    drawText(windowWidth / 2 - 35, 89, buf, GLUT_BITMAP_HELVETICA_18);

    // Controls hint - bottom right
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(windowWidth - 314, 164);
    glVertex2f(windowWidth - 74, 164);
    glVertex2f(windowWidth - 74, 74);
    glVertex2f(windowWidth - 314, 74);
    glEnd();

    glColor4f(0.0f, 0.8f, 0.8f, 0.8f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(windowWidth - 314, 164);
    glVertex2f(windowWidth - 74, 164);
    glVertex2f(windowWidth - 74, 74);
    glVertex2f(windowWidth - 314, 74);
    glEnd();

    glColor3f(0.7f, 0.7f, 0.7f);
    drawText(windowWidth - 304, 149, "W/S: Fwd/Bwd  A/D: Turn", GLUT_BITMAP_HELVETICA_12);
    drawText(windowWidth - 304, 134, "R/F: Up/Down  Q/E: Roll", GLUT_BITMAP_HELVETICA_12);
    drawText(windowWidth - 304, 119, "L: Lights  C: Camera", GLUT_BITMAP_HELVETICA_12);
    drawText(windowWidth - 304, 104, "Space: Stop  ESC: Exit", GLUT_BITMAP_HELVETICA_12);
    drawText(windowWidth - 304, 89, "Mouse: Orbit (Ext view)", GLUT_BITMAP_HELVETICA_12);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawViewportBorder() {
    if (gameState != STATE_PLAYING) return;
    if (cameraMode != CAM_INTERIOR && cameraMode != CAM_FRONT_WINDOW &&
        cameraMode != CAM_LEFT_WINDOW && cameraMode != CAM_RIGHT_WINDOW &&
        cameraMode != CAM_CONTROL_SCREEN) return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float W = (float)windowWidth, H = (float)windowHeight;
    float S = 44.0f;
    float T = 44.0f;
    float B = 44.0f;
    float R = 200.0f;
    if (R > W * 0.25f) R = W * 0.25f;
    float Cy = H - T - R;

    glColor4f(0.005f, 0.012f, 0.030f, 0.96f);
    glBegin(GL_QUADS);
    glVertex2f(0, H - T); glVertex2f(W, H - T);
    glVertex2f(W, H); glVertex2f(0, H);
    glVertex2f(0, 0); glVertex2f(W, 0);
    glVertex2f(W, B); glVertex2f(0, B);
    glVertex2f(0, B); glVertex2f(S, B);
    glVertex2f(S, Cy); glVertex2f(0, Cy);
    glVertex2f(W - S, B); glVertex2f(W, B);
    glVertex2f(W, Cy); glVertex2f(W - S, Cy);
    glEnd();

    const int N = 24;
    glBegin(GL_QUADS);
    for (int i = 0; i < N; i++) {
        float dy0 = R * i / N, dy1 = R * (i + 1) / N;
        float y0 = Cy + dy0, y1 = Cy + dy1;
        float xl0 = S + R - sqrt(R * R - dy0 * dy0);
        float xl1 = S + R - sqrt(R * R - dy1 * dy1);
        glVertex2f(0, y0); glVertex2f(xl0, y0);
        glVertex2f(xl1, y1); glVertex2f(0, y1);
        glVertex2f(W, y0); glVertex2f(W - xl0, y0);
        glVertex2f(W - xl1, y1); glVertex2f(W, y1);
    }
    glEnd();

    glLineWidth(2.0f);
    glColor4f(0.35f, 0.70f, 0.85f, 0.55f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(S, B);
    glVertex2f(S, Cy);
    for (int i = 0; i <= N; i++) {
        float a = PI - (PI * 0.5f) * i / N;
        glVertex2f(S + R + R * cos(a), Cy + R * sin(a));
    }
    glVertex2f(W - S - R, H - T);
    for (int i = 0; i <= N; i++) {
        float a = (PI * 0.5f) - (PI * 0.5f) * i / N;
        glVertex2f(W - S - R + R * cos(a), Cy + R * sin(a));
    }
    glVertex2f(W - S, B);
    glVertex2f(S, B);
    glEnd();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ============================================================
// LIGHTING SETUP
// ============================================================
void setupLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);

    glShadeModel(GL_SMOOTH);
    GLfloat matSpec[] = { 0.70f, 0.70f, 0.75f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 90.0f);

    // Bright day ambient on surface, dark blue ambient when deep
    float diveBlend = 1.0f - diveTransition;
    GLfloat globalAmbient[] = {
        0.45f * diveBlend + 0.22f,
        0.45f * diveBlend + 0.25f,
        0.50f * diveBlend + 0.30f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    // Key sun from FRONT-LEFT-TOP (camera side +Z) so visible hull is lit
    GLfloat sunPos[] = { -30.0f, 28.0f, 35.0f, 0.0f };
    GLfloat sunAmb[] = { 0.35f * diveBlend + 0.15f, 0.33f * diveBlend + 0.15f, 0.30f * diveBlend + 0.16f, 1.0f };
    GLfloat sunDiff[] = { 1.15f * diveBlend + 0.35f, 1.05f * diveBlend + 0.35f, 0.95f * diveBlend + 0.35f, 1.0f };
    GLfloat sunSpec[] = { 0.7f * diveBlend, 0.7f * diveBlend, 0.65f * diveBlend, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, sunPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, sunAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDiff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpec);

    // Submarine hull light (follows sub, strengthens with depth)
    float uw = diveTransition;
    GLfloat subPos[] = { sub.x, sub.y + 0.5f, sub.z, 1.0f };
    GLfloat subAmb[] = { 0.05f + 0.14f * uw, 0.05f + 0.15f * uw, 0.08f + 0.18f * uw, 1.0f };
    GLfloat subDiff[] = { 0.15f + 0.60f * uw, 0.15f + 0.65f * uw, 0.25f + 0.70f * uw, 1.0f };

    glLightfv(GL_LIGHT1, GL_POSITION, subPos);
    glLightfv(GL_LIGHT1, GL_AMBIENT, subAmb);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, subDiff);
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.1f);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.02f);

    if (!sub.headlightsOn) {
        glDisable(GL_LIGHT2);
    }
}

void setupFog() {
    float diveBlend = diveTransition;
    if (diveBlend <= 0.01f) {
        glDisable(GL_FOG);
        return;
    }

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_EXP2);
    // Natural deep-sea blue, not black: shallow teal -> deep navy
    GLfloat fogColor[] = { 0.10f * diveBlend, 0.45f * diveBlend + 0.02f, 0.70f * diveBlend + 0.04f, 1.0f };
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_DENSITY, 0.015f * diveBlend);
    glHint(GL_FOG_HINT, GL_DONT_CARE);
}

// ============================================================
// CAMERA
// ============================================================
void setupCamera() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)windowWidth / windowHeight, 0.1, 500.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (gameState == STATE_INTRO || gameState == STATE_DIVING) {
        // IDEA from 1st ref: low starboard side view, bow to the right,
        // sunset + hills behind, slow push-in like a movie frame.
        // Sub lies along X (bow +X), so camera sits off +Z side.
        float t = introTimer * 0.001f;
        float pushIn = 1.0f - diveTransition * 0.4f;
        float camX = sub.x + 4.2f + sin(t * 0.4f) * 0.8f;
        float camY = sub.y + 2.8f * pushIn + sin(t * 0.6f) * 0.2f;
        float camZ = sub.z + 14.0f * pushIn;
        float lookX = sub.x + 0.3f;
        float lookY = sub.y + 0.6f - diveTransition * 4.0f;
        if (diveTransition > 0.4f) {
            camY = sub.y + 3.3f;
            camZ = sub.z + 12.0f;
            lookY = sub.y + 0.3f;
        }
        gluLookAt(camX, camY, camZ, lookX, lookY, sub.z, 0, 1, 0);
        return;
    }

    switch (cameraMode) {
    case CAM_EXTERNAL: {
        float radH = cam.orbitAngleH * DEG_TO_RAD;
        float radV = cam.orbitAngleV * DEG_TO_RAD;
        float dist = cam.orbitDistance;
        float cx = sub.x + dist * cos(radV) * sin(radH);
        float cy = sub.y + dist * sin(radV);
        float cz = sub.z + dist * cos(radV) * cos(radH);
        gluLookAt(cx, cy, cz, sub.x, sub.y, sub.z, 0, 1, 0);
        break;
    }
    case CAM_INTERIOR: {
        float subYawRad = sub.yaw * DEG_TO_RAD;
        float subPitchRad = sub.pitch * DEG_TO_RAD;
        // Camera inside submarine, behind controls
        float ix = sub.x + cos(subYawRad) * 0.8f;
        float iy = sub.y + 0.3f;
        float iz = sub.z - sin(subYawRad) * 0.8f;
        float fx = sub.x + cos(subYawRad) * 3.0f;
        float fy = sub.y + sin(subPitchRad) * 2.0f;
        float fz = sub.z - sin(subYawRad) * 3.0f;
        gluLookAt(ix, iy, iz, fx, fy, fz, 0, 1, 0);
        break;
    }
    case CAM_FRONT_WINDOW: {
        float subYawRad = sub.yaw * DEG_TO_RAD;
        float fx = sub.x + cos(subYawRad) * 3.0f;
        float fy = sub.y + sin(sub.pitch * DEG_TO_RAD) * 2.0f;
        float fz = sub.z - sin(subYawRad) * 3.0f;
        gluLookAt(sub.x + cos(subYawRad) * 1.5f, sub.y, sub.z - sin(subYawRad) * 1.5f,
                   fx, fy, fz, 0, 1, 0);
        break;
    }
    case CAM_LEFT_WINDOW: {
        float subYawRad = sub.yaw * DEG_TO_RAD;
        float lx = sub.x + cos(subYawRad) * 0.5f;
        float ly = sub.y;
        float lz = sub.z - sin(subYawRad) * 0.5f + 1.5f;
        float lx2 = lx + cos(subYawRad) * 10.0f;
        float lz2 = lz + sin(subYawRad + PI / 2) * 10.0f;
        gluLookAt(lx, ly, lz, lx2, ly, lz2, 0, 1, 0);
        break;
    }
    case CAM_RIGHT_WINDOW: {
        float subYawRad = sub.yaw * DEG_TO_RAD;
        float rx = sub.x + cos(subYawRad) * 0.5f;
        float ry = sub.y;
        float rz = sub.z - sin(subYawRad) * 0.5f - 1.5f;
        float rx2 = rx + cos(subYawRad) * 10.0f;
        float rz2 = rz - sin(subYawRad + PI / 2) * 10.0f;
        gluLookAt(rx, ry, rz, rx2, ry, rz2, 0, 1, 0);
        break;
    }
    case CAM_CONTROL_SCREEN: {
        float subYawRad = sub.yaw * DEG_TO_RAD;
        float sx = sub.x + cos(subYawRad) * 1.4f;
        float sy = sub.y + 0.3f;
        float sz = sub.z - sin(subYawRad) * 1.4f;
        float sx2 = sub.x + cos(subYawRad) * 30.0f;
        float sy2 = sub.y;
        float sz2 = sub.z - sin(subYawRad) * 30.0f;
        gluLookAt(sx, sy, sz, sx2, sy2, sz2, 0, 1, 0);
        break;
    }
    case CAM_FREE: {
        float fyawRad = cam.freeYaw * DEG_TO_RAD;
        float fpitchRad = cam.freePitch * DEG_TO_RAD;
        float fx = cam.freeX + cos(fpitchRad) * sin(fyawRad) * 5.0f;
        float fy = cam.freeY + sin(fpitchRad) * 5.0f;
        float fz = cam.freeZ + cos(fpitchRad) * cos(fyawRad) * 5.0f;
        gluLookAt(cam.freeX, cam.freeY, cam.freeZ, fx, fy, fz, 0, 1, 0);
        break;
    }
    }
}

// ============================================================
// UPDATE LOGIC
// ============================================================
void updateSubmarine() {
    // Smooth speed
    sub.speed += (sub.targetSpeed - sub.speed) * 0.05f;

    // Propeller
    if (fabs(sub.speed) > 0.01f) {
        sub.propellerAngle += sub.speed * 50.0f;
    }

    // Movement
    float yawRad = sub.yaw * DEG_TO_RAD;
    float pitchRad = sub.pitch * DEG_TO_RAD;
    sub.x += cos(yawRad) * cos(pitchRad) * sub.speed * 0.1f;
    sub.z -= sin(yawRad) * cos(pitchRad) * sub.speed * 0.1f;
    sub.y += sin(pitchRad) * sub.speed * 0.1f;

    // Clamp depth
    if (sub.y > 3.0f) sub.y = 3.0f;
    if (sub.y < -50.0f) sub.y = -50.0f;

    sub.depth = -sub.y;

    // Generate bubbles from submarine
    if (fabs(sub.speed) > 0.01f && diveTransition > 0.3f) {
        if (rand() % 100 < 30) {
            Bubble b;
            b.x = sub.x - cos(yawRad) * 7.5f + (rand() % 100 - 50) * 0.005f;
            b.y = sub.y + (rand() % 100 - 50) * 0.01f;
            b.z = sub.z + sin(yawRad) * 7.5f + (rand() % 100 - 50) * 0.005f;
            b.speed = 0.02f + (rand() % 100) * 0.0003f;
            b.size = 0.03f + (rand() % 100) * 0.001f;
            b.wobble = 0.1f + (rand() % 100) * 0.003f;
            b.wobblePhase = (rand() % 1000) * 0.01f;
            bubbles.push_back(b);
        }
    }

    // Environment bubbles (always in underwater)
    if (diveTransition > 0.2f && rand() % 100 < 5) {
        Bubble b;
        b.x = (rand() % 200 - 100) * 0.3f;
        b.y = -16.0f + (rand() % 100) * 0.12f;
        b.z = (rand() % 200 - 100) * 0.3f;
        b.speed = 0.01f + (rand() % 100) * 0.0002f;
        b.size = 0.02f + (rand() % 100) * 0.0008f;
        b.wobble = 0.15f;
        b.wobblePhase = (rand() % 1000) * 0.01f;
        bubbles.push_back(b);
    }

    // Update bubbles
    for (int i = (int)bubbles.size() - 1; i >= 0; i--) {
        bubbles[i].y += bubbles[i].speed;
        bubbles[i].wobblePhase += 0.05f;
        if (bubbles[i].y > (diveTransition > 0.5f ? sub.y + 5.0f : 2.0f)) {
            bubbles.erase(bubbles.begin() + i);
        }
    }
    if (bubbles.size() > 200) {
        bubbles.erase(bubbles.begin(), bubbles.begin() + (bubbles.size() - 200));
    }

    // Update fish
    for (size_t i = 0; i < fishes.size(); i++) {
        Fish& f = fishes[i];
        f.animPhase += 0.02f;
        f.x += cos(f.angle) * f.speed;
        f.z += sin(f.angle) * f.speed;

        // Slight direction changes
        f.angle += (rand() % 100 - 50) * 0.0005f;

        // Keep fish in range
        float dist = sqrt(f.x * f.x + f.z * f.z);
        if (dist > 50.0f) {
            f.angle = atan2(-f.z, -f.x);
        }

        // Fish avoid submarine
        float sdx = f.x - sub.x;
        float sdz = f.z - sub.z;
        float sdDist = sqrt(sdx * sdx + sdz * sdz);
        if (sdDist < 4.5f) {
            f.angle = atan2(sdz, sdx);
        }
    }

    // Update sharks and rays
    for (size_t i = 0; i < sharks.size(); i++) {
        sharks[i].angle += sharks[i].speed;
    }
    for (size_t i = 0; i < rays.size(); i++) {
        rays[i].angle += rays[i].speed;
        rays[i].phase += 0.02f;
    }

    // Update particles
    for (size_t i = 0; i < particles.size(); i++) {
        particles[i].x += particles[i].vx;
        particles[i].y += particles[i].vy;
        particles[i].z += particles[i].vz;
        // Subtle attraction toward submarine
        float dx = sub.x - particles[i].x;
        float dy = sub.y - particles[i].y;
        float dz = sub.z - particles[i].z;
        float dist = sqrt(dx * dx + dy * dy + dz * dz);
        if (dist > 1.0f && dist < 20.0f) {
            particles[i].vx += dx * 0.00001f;
            particles[i].vy += dy * 0.00001f;
            particles[i].vz += dz * 0.00001f;
        }
    }
}

void updateIntro() {
    introTimer += 16.0f; // ~60fps

    if (gameState == STATE_INTRO) {
        // After 5 seconds start dive
        if (introTimer > 5000.0f) {
            gameState = STATE_DIVING;
        }
    }

    if (gameState == STATE_DIVING) {
        diveTimer += 16.0f;
        diveTransition = diveTimer / 5000.0f; // 5 second dive
        if (diveTransition > 1.0f) diveTransition = 1.0f;

        // Submarine descends from surfaced waterline
        sub.y = 0.9f - diveTransition * 14.0f;
        sub.depth = -sub.y;

        if (diveTimer > 6000.0f) {
            gameState = STATE_MENU;
        }
    }

    if (gameState == STATE_PLAYING) {
        missionTimer += 0.016f;
        // Advance missions based on depth
        if (sub.depth > 5.0f && currentMission == MISSION_DIVE) {
            currentMission = MISSION_EXPLORE;
        }
        if (sub.depth > 15.0f && currentMission == MISSION_EXPLORE) {
            currentMission = MISSION_NAVIGATE;
        }
        if (fabs(sub.x) > 10.0f || fabs(sub.z) > 10.0f) {
            if (currentMission == MISSION_NAVIGATE) {
                currentMission = MISSION_OBSERVE;
            }
        }
        if (sub.depth < 5.0f && currentMission == MISSION_OBSERVE) {
            currentMission = MISSION_RETURN;
        }
    }
}

// ============================================================
// MENU SCREEN
// ============================================================
void drawMenuScreen() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    // Background
    glColor4f(0.0f, 0.02f, 0.05f, 0.9f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(windowWidth, 0);
    glVertex2f(windowWidth, windowHeight);
    glVertex2f(0, windowHeight);
    glEnd();

    // Title
    glColor3f(0.0f, 0.9f, 1.0f);
    drawText(windowWidth / 2 - 160, windowHeight - 100, "3D SUBMARINE SIMULATOR",
             GLUT_BITMAP_TIMES_ROMAN_24);

    // Subtitle
    glColor3f(0.0f, 0.6f, 0.7f);
    drawText(windowWidth / 2 - 100, windowHeight - 140, "DEEP SEA EXPLORATION",
             GLUT_BITMAP_HELVETICA_18);

    // Mission started message
    if (diveTimer > 3000.0f) {
        glColor3f(1.0f, 0.8f, 0.0f);
        drawText(windowWidth / 2 - 80, windowHeight - 180, "MISSION STARTED",
                 GLUT_BITMAP_HELVETICA_18);
    }

    // Menu items
    const char* menuItems[] = { "START MISSION", "EXTERNAL 3D VIEW", "CONTROLS", "EXIT" };
    int menuCount = 4;

    for (int i = 0; i < menuCount; i++) {
        float yPos = windowHeight - 280 - i * 50.0f;

        // Selection box
        if (i == menuSelection) {
            glColor4f(0.0f, 0.3f, 0.4f, 0.8f);
            glBegin(GL_QUADS);
            glVertex2f(windowWidth / 2 - 150, yPos - 15);
            glVertex2f(windowWidth / 2 + 150, yPos - 15);
            glVertex2f(windowWidth / 2 + 150, yPos + 15);
            glVertex2f(windowWidth / 2 - 150, yPos + 15);
            glEnd();

            glColor4f(0.0f, 0.8f, 1.0f, 0.6f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(windowWidth / 2 - 150, yPos - 15);
            glVertex2f(windowWidth / 2 + 150, yPos - 15);
            glVertex2f(windowWidth / 2 + 150, yPos + 15);
            glVertex2f(windowWidth / 2 - 150, yPos + 15);
            glEnd();
        }

        glColor3f(i == menuSelection ? 1.0f : 0.6f,
                   i == menuSelection ? 1.0f : 0.7f,
                   i == menuSelection ? 1.0f : 0.8f);
        int textW = (int)strlen(menuItems[i]) * 8;
        drawText(windowWidth / 2 - textW / 2, yPos, menuItems[i],
                 i == menuSelection ? GLUT_BITMAP_HELVETICA_18 : GLUT_BITMAP_HELVETICA_18);
    }

    // Controls hint
    glColor3f(0.4f, 0.5f, 0.6f);
    drawText(windowWidth / 2 - 120, 40, "UP/DOWN: Navigate  ENTER: Select",
             GLUT_BITMAP_HELVETICA_12);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawIntroOverlay() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    // Title
    float titleAlpha = 1.0f;
    if (introTimer > 4000.0f) {
        titleAlpha = 1.0f - (introTimer - 4000.0f) / 1000.0f;
        if (titleAlpha < 0) titleAlpha = 0;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.0f, 0.9f, 1.0f, titleAlpha);
    drawText(windowWidth / 2 - 160, windowHeight / 2 + 50, "3D SUBMARINE SIMULATOR",
             GLUT_BITMAP_TIMES_ROMAN_24);

    if (introTimer > 1500.0f && introTimer < 4500.0f) {
        float initAlpha = 1.0f;
        if (introTimer > 3500.0f) {
            initAlpha = 1.0f - (introTimer - 3500.0f) / 1000.0f;
            if (initAlpha < 0) initAlpha = 0;
        }
        glColor4f(1.0f, 0.8f, 0.2f, initAlpha);
        drawText(windowWidth / 2 - 80, windowHeight / 2, "MISSION INITIALIZING...",
                 GLUT_BITMAP_HELVETICA_18);
    }

    if (gameState == STATE_DIVING && diveTimer > 2000.0f) {
        float missionAlpha = 1.0f;
        if (diveTimer > 4000.0f) {
            missionAlpha = 1.0f - (diveTimer - 4000.0f) / 1000.0f;
            if (missionAlpha < 0) missionAlpha = 0;
        }
        glColor4f(0.0f, 1.0f, 0.5f, missionAlpha);
        drawText(windowWidth / 2 - 70, windowHeight / 2 - 40, "MISSION STARTED",
                 GLUT_BITMAP_HELVETICA_18);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ============================================================
// DISPLAY
// ============================================================
void display() {
    // Sky-blue background on surface, dark blue when deep (never black)
    float db = diveTransition;
    // Day sky blue -> shallow teal -> deep navy (matches ref 2, never black)
    float deepK = db * db; // ease into depth
    glClearColor(0.52f * (1 - db) + 0.06f * db * (1 - deepK) + 0.05f * deepK,
                 0.74f * (1 - db) + 0.35f * db * (1 - deepK) + 0.30f * deepK,
                 0.94f * (1 - db) + 0.55f * db * (1 - deepK) + 0.60f * deepK, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    setupCamera();
    setupFog();
    setupLighting();
    setupSubHeadlight();

    // Draw environments
    if (diveTransition < 1.0f) {
        drawSky();
        drawSun();
        drawDistantHills();
        drawClouds();
        drawTrees();
    }

    drawOceanSurface();
    // Surface-only cinematic dressing from 1st ref
    if (diveTransition < 0.6f) {
        drawBowFoam();
        drawDeckCrewSilhouettes();
    }

    if (diveTransition > 0.1f) {
        drawLightRays();
        drawSeaweed();
        drawKelp();
        drawRocks();
        drawCoral();
        drawParticles();
    }

    // Draw submarine (visible in INTRO surface shot too, like 1st ref)
    if (gameState == STATE_PLAYING || gameState == STATE_DIVING ||
        gameState == STATE_INTRO) {
        if (cameraMode == CAM_INTERIOR || cameraMode == CAM_FRONT_WINDOW ||
            cameraMode == CAM_LEFT_WINDOW || cameraMode == CAM_RIGHT_WINDOW ||
            cameraMode == CAM_CONTROL_SCREEN) {
            drawInterior();
        } else {
            drawFullSubmarine();
        }
    }

    // Draw marine life
    drawFish();
    drawSharks();
    drawRays();
    drawBubbles();

    // Overlays
    if (gameState == STATE_INTRO || gameState == STATE_DIVING) {
        drawIntroOverlay();
    }

    if (gameState == STATE_MENU) {
        drawMenuScreen();
    }

    if (gameState == STATE_PLAYING) {
        drawHUD();
    }

    drawViewportBorder();

    glutSwapBuffers();
}

// ============================================================
// KEYBOARD
// ============================================================
void keyboardDown(unsigned char key, int x, int y) {
    (void)x; (void)y;

    if (gameState == STATE_MENU) {
        if (key == 13) { // Enter
            switch (menuSelection) {
            case 0: // START MISSION
                gameState = STATE_PLAYING;
                currentMission = MISSION_DIVE;
                cameraMode = CAM_EXTERNAL;
                missionTimer = 0;
                sub.x = 0; sub.y = -5.0f; sub.z = 0;
                sub.depth = 5.0f;
                break;
            case 1: // EXTERNAL 3D VIEW
                gameState = STATE_PLAYING;
                cameraMode = CAM_EXTERNAL;
                currentMission = MISSION_DIVE;
                missionTimer = 0;
                sub.x = 0; sub.y = -5.0f; sub.z = 0;
                sub.depth = 5.0f;
                break;
            case 2: // CONTROLS - show info
                break;
            case 3: // EXIT
                exit(0);
                break;
            }
        }
        if (key == 27) exit(0);
        return;
    }

    if (gameState == STATE_INTRO || gameState == STATE_DIVING) {
        if (key == 27) exit(0);
        if (key == 13) {
            // Skip intro
            gameState = STATE_MENU;
            diveTimer = 6000.0f;
            diveTransition = 1.0f;
            sub.y = -13.0f;
        }
        return;
    }

    // Playing state controls
    switch (key) {
    case 'w': case 'W':
        sub.targetSpeed = 1.0f;
        break;
    case 's': case 'S':
        sub.targetSpeed = -0.5f;
        break;
    case 'a': case 'A':
        sub.yaw += 3.0f;
        break;
    case 'd': case 'D':
        sub.yaw -= 3.0f;
        break;
    case 'r': case 'R':
        sub.pitch += 2.0f;
        if (sub.pitch > 45) sub.pitch = 45;
        break;
    case 'f': case 'F':
        sub.pitch -= 2.0f;
        if (sub.pitch < -45) sub.pitch = -45;
        break;
    case 'q': case 'Q':
        sub.roll -= 2.0f;
        if (sub.roll < -30) sub.roll = -30;
        break;
    case 'e': case 'E':
        sub.roll += 2.0f;
        if (sub.roll > 30) sub.roll = 30;
        break;
    case 'l': case 'L':
        sub.headlightsOn = !sub.headlightsOn;
        break;
    case 'c': case 'C':
        cameraMode = (CameraMode)((cameraMode + 1) % CAM_COUNT);
        break;
    case 'v': case 'V':
        cameraMode = CAM_EXTERNAL;
        break;
    case 'i': case 'I':
        cameraMode = CAM_INTERIOR;
        break;
    case ' ':
        sub.targetSpeed = 0;
        break;
    case 27: // ESC
        gameState = STATE_MENU;
        menuSelection = 0;
        sub.targetSpeed = 0;
        break;
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    (void)x; (void)y;

    if (gameState != STATE_PLAYING) return;

    switch (key) {
    case 'w': case 'W':
    case 's': case 'S':
        sub.targetSpeed = 0;
        break;
    }
}

void specialKeys(int key, int x, int y) {
    (void)x; (void)y;

    if (gameState == STATE_MENU) {
        if (key == GLUT_KEY_UP) {
            menuSelection--;
            if (menuSelection < 0) menuSelection = 3;
        }
        if (key == GLUT_KEY_DOWN) {
            menuSelection++;
            if (menuSelection > 3) menuSelection = 0;
        }
        glutPostRedisplay();
        return;
    }

    if (gameState != STATE_PLAYING) return;

    // Arrow keys for submarine movement
    switch (key) {
    case GLUT_KEY_UP:
        sub.targetSpeed = 1.0f;
        break;
    case GLUT_KEY_DOWN:
        sub.targetSpeed = -0.5f;
        break;
    case GLUT_KEY_LEFT:
        sub.yaw += 3.0f;
        break;
    case GLUT_KEY_RIGHT:
        sub.yaw -= 3.0f;
        break;
    }
}

// ============================================================
// MOUSE
// ============================================================
int mouseLastX = 0, mouseLastY = 0;
int mouseDragging = 0;

void mouse(int button, int state, int mx, int my) {
    if (button == GLUT_LEFT_BUTTON) {
        mouseDragging = (state == GLUT_DOWN);
        mouseLastX = mx;
        mouseLastY = my;
    }
    // Zoom (wheel = buttons 3/4, works on all GLUT builds)
    if (button == 3) {
        cam.orbitDistance -= 1.0f;
        if (cam.orbitDistance < 3.0f) cam.orbitDistance = 3.0f;
    }
    if (button == 4) {
        cam.orbitDistance += 1.0f;
        if (cam.orbitDistance > 50.0f) cam.orbitDistance = 50.0f;
    }
    glutPostRedisplay();
}

void motion(int mx, int my) {
    if (mouseDragging) {
        int dx = mx - mouseLastX;
        int dy = my - mouseLastY;

        if (cameraMode == CAM_EXTERNAL || gameState == STATE_INTRO || gameState == STATE_DIVING) {
            cam.orbitAngleH += dx * 0.5f;
            cam.orbitAngleV += dy * 0.3f;
            if (cam.orbitAngleV > 85) cam.orbitAngleV = 85;
            if (cam.orbitAngleV < -85) cam.orbitAngleV = -85;
        } else if (cameraMode == CAM_FREE) {
            cam.freeYaw += dx * 0.3f;
            cam.freePitch -= dy * 0.3f;
            if (cam.freePitch > 85) cam.freePitch = 85;
            if (cam.freePitch < -85) cam.freePitch = -85;
        }

        mouseLastX = mx;
        mouseLastY = my;
        glutPostRedisplay();
    }
}

// ============================================================
// RESIZE
// ============================================================
void reshape(int w, int h) {
    if (h == 0) h = 1;
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);
    glutPostRedisplay();
}

// ============================================================
// TIMER / MAIN LOOP
// ============================================================
void timer(int value) {
    (void)value;
    updateIntro();
    updateSubmarine();
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); // ~60 FPS
}

// ============================================================
// MAIN
// ============================================================
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("3D Submarine Simulator - OpenGL");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    initSubmarine();
    initCamera();
    initEnvironment();
    initTextures();
    setupLighting();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutTimerFunc(16, timer, 0);

    printf("============================================\n");
    printf("     3D SUBMARINE SIMULATOR - OpenGL\n");
    printf("============================================\n");
    printf("CONTROLS:\n");
    printf("  W/S     - Forward/Backward\n");
    printf("  A/D     - Turn Left/Right\n");
    printf("  R/F     - Pitch Up/Down\n");
    printf("  Q/E     - Roll Left/Right\n");
    printf("  L       - Toggle Headlights\n");
    printf("  C       - Change Camera Mode\n");
    printf("  V       - External View\n");
    printf("  I       - Interior View\n");
    printf("  SPACE   - Emergency Stop\n");
    printf("  ESC     - Menu/Exit\n");
    printf("  Mouse   - Orbit Camera\n");
    printf("============================================\n");

    glutMainLoop();
    return 0;
}
