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
bool userCameraChoice = false;
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

// Interior scale for crew room — geometry enlarged more than eye height
// so the space genuinely feels bigger when walking around.
const float INT_SX = 1.70f;   // stretch front-to-back (length)
const float INT_SY = 1.30f;   // modest height increase
const float INT_SZ = 1.50f;   // wider port-to-starboard
const float INT_EYE = 0.52f;  // base eye height (unscaled, used as-is)

// Camera State
struct Camera {
    float orbitAngleH;
    float orbitAngleV;
    float orbitDistance;
    float freeX, freeY, freeZ;
    float freeYaw, freePitch;
} cam;

// Held keys for smooth first-person walking in the crew cabin
bool walkKeys[256] = { false };

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
// short labels that fit inside the small HUD info box
const char* missionShort[] = {
    "DESCEND",
    "EXPLORE",
    "NAVIGATE",
    "OBSERVE",
    "RETURN"
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
GLuint texSky = 0, texWater = 0, texSand = 0, texHull = 0, texPuff = 0, texOceanView = 0;

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

// Rolling sea-bed hills so the floor looks like a real underwater terrain.
static float seaFloorY(float x, float z) {
    return -16.0f
        + sin(x * 0.11f) * cos(z * 0.09f) * 2.4f
        + sin((x + z) * 0.055f) * 1.3f
        + sin(x * 0.31f) * cos(z * 0.27f) * 0.35f
        + sin(x * 0.9f + z * 0.8f) * 0.12f;
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
    // ---- Hull camo: 4-tone woodland + riveted black plate overlay ----
    {
        const int W = 256, H = 256;
        static unsigned char px[256 * 256 * 3];
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
            float fx = x * 0.055f, fy = y * 0.055f;
            float wx = smoothNoise(fx * 0.35f, fy * 0.35f, 81) * 6.0f;
            float wy = smoothNoise(fx * 0.35f + 50, fy * 0.35f + 50, 82) * 6.0f;
            float ax = fx + wx, ay = fy + wy;
            float n1 = smoothNoise(ax * 0.55f, ay * 0.55f, 11);
            float n2 = smoothNoise(ax * 1.05f + 100, ay * 1.05f + 100, 12);
            float n3 = smoothNoise(ax * 2.10f, ay * 2.10f, 13) * 0.5f;
            float n = n1 * 0.60f + n2 * 0.32f + n3 * 0.08f;
            float n4 = smoothNoise(ax * 0.90f, ay * 0.45f, 14);
            n += (n4 - 0.5f) * 0.18f;
            if (n < 0) n = 0; if (n > 1) n = 1;
            float r, g, b;
            if (n < 0.28f)      { r = 26;  g = 38;  b = 18; }
            else if (n < 0.52f) { r = 88;  g = 108; b = 68; }
            else if (n < 0.74f) { r = 192; g = 178; b = 132; }
            else                { r = 102; g = 72;  b = 42; }
            float plateH = 64.0f, plateW = 64.0f;
            int row = (int)(y / plateH);
            float off = (row & 1) ? plateW * 0.5f : 0.0f;
            float lx = fmod(x + off, plateW);
            float ly = fmod(y, plateH);
            float seam = 0;
            if (lx < 1.5f || lx > plateW - 1.5f) seam = 1.0f;
            if (ly < 1.5f || ly > plateH - 1.5f) seam = 1.0f;
            float rivet = 0, shine = 0;
            float rxA[5] = {6, plateW-6, 6, plateW-6, plateW*0.5f};
            float ryA[5] = {6, 6, plateH-6, plateH-6, 6};
            if (row & 1) { ryA[4] = plateH-6; }
            for (int k = 0; k < 5; k++) {
                float dx = lx - rxA[k], dy = ly - ryA[k];
                float d = sqrt(dx*dx + dy*dy);
                if (d < 4.2f) {
                    rivet = 1.0f - d/4.2f;
                    if (d < 2.0f && dx < -0.3f && dy < -0.3f) shine = (2.0f - d)/2.0f;
                }
                float dx2 = lx - rxA[k], dy2 = ly - (ryA[k] + plateH*0.5f);
                if (fabs(dy2) < 5 && fabs(dx2) < 4) {
                    float d2 = sqrt(dx2*dx2 + dy2*dy2*0.9f);
                    if (d2 < 3.8f) { float v = 1.0f - d2/3.8f; if (v > rivet) { rivet = v; if (d2 < 1.8f && dx2 < -0.2f) shine = (1.8f - d2)/1.8f; } }
                }
            }
            if (shine > 0) { r = r*(1-shine) + 155*shine; g = g*(1-shine) + 156*shine; b = b*(1-shine) + 158*shine; }
            else if (rivet > 0) { float k = rivet*0.72f; r = r*(1-k) + 18*k; g = g*(1-k) + 19*k; b = b*(1-k) + 22*k; }
            if (seam > 0 && rivet < 0.5f) { float k = seam*0.38f; r = r*(1-k) + 42*k; g = g*(1-k) + 44*k; b = b*(1-k) + 48*k; }
            float grain = (noise01(x, y, 19) - 0.5f) * 7;
            r += grain; g += grain; b += grain;
            if (r < 0) r = 0; if (r > 255) r = 255;
            if (g < 0) g = 0; if (g > 255) g = 255;
            if (b < 0) b = 0; if (b > 255) b = 255;
            int o = (y * W + x) * 3;
            px[o] = (unsigned char)r; px[o+1] = (unsigned char)g; px[o+2] = (unsigned char)b;
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
    {
        const char* tryFiles[] = {"ocean_view.bmp","3D Submarine Simulator/ocean_view.bmp","bin/Debug/ocean_view.bmp","bin/Release/ocean_view.bmp","../ocean_view.bmp","submarine-ocean-view-stockcake.bmp"};
        for (int t=0; t<6 && !texOceanView; t++) {
            FILE* f = fopen(tryFiles[t], "rb");
            if (!f) continue;
            unsigned char hdr[54];
            if (fread(hdr,1,54,f)!=54) { fclose(f); continue; }
            int w = hdr[18]|hdr[19]<<8|hdr[20]<<16|hdr[21]<<24;
            int h = hdr[22]|hdr[23]<<8|hdr[24]<<16|hdr[25]<<24;
            int bpp = hdr[28]|hdr[29]<<8;
            int off = hdr[10]|hdr[11]<<8|hdr[12]<<16|hdr[13]<<24;
            if ((bpp!=24 && bpp!=32) || w<=0 || h<=0 || w>4096 || h>4096) { fclose(f); continue; }
            int rowPad = (bpp==24) ? (4 - (w*3)%4)%4 : 0;
            unsigned char* data = (unsigned char*)malloc(w*h*3);
            if (!data) { fclose(f); continue; }
            fseek(f, off, SEEK_SET);
            for (int y=h-1; y>=0; y--) {
                unsigned char* row = data + y*w*3;
                for (int x=0;x<w;x++) { unsigned char b= fgetc(f), g=fgetc(f), r=fgetc(f); if(bpp==32) fgetc(f); row[x*3]=r; row[x*3+1]=g; row[x*3+2]=b; }
                for (int p=0;p<rowPad;p++) fgetc(f);
            }
            fclose(f);
            texOceanView = uploadTexture(w, h, data, false, false);
            free(data);
        }
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
    sub.headlightsOn = false;
    sub.depth = 0;
    sub.descentRate = 0;
}

void initCamera() {
    cam.orbitAngleH = 30.0f;
    cam.orbitAngleV = 15.0f;
    cam.orbitDistance = 14.0f;
    cam.freeX = 0; cam.freeY = 5; cam.freeZ = 15;
    cam.freeYaw = 0; cam.freePitch = 0;
}

void initEnvironment() {
    srand((unsigned)time(NULL));

    // Fish - dense silver school like reference + scattered
    fishes.clear();
    for (int i = 0; i < 260; i++) {
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
        // weighted types: ~30% jellyfish, rest small + large fish
        int rType = rand() % 10;
        f.type = (rType < 3) ? 2 : ((rType < 8) ? (rand() % 2) : 1);
        f.animPhase = (rand() % 1000) * 0.01f;
        switch (rand() % 7) {
        case 0: f.r = 0.96f; f.g = 0.56f; f.b = 0.12f; break; // reef orange
        case 1: f.r = 0.88f; f.g = 0.20f; f.b = 0.28f; break; // scarlet
        case 2: f.r = 0.16f; f.g = 0.42f; f.b = 0.88f; break; // royal blue
        case 3: f.r = 0.14f; f.g = 0.72f; f.b = 0.82f; break; // turquoise
        case 4: f.r = 0.16f; f.g = 0.64f; f.b = 0.34f; break; // emerald
        case 5: f.r = 0.70f; f.g = 0.32f; f.b = 0.86f; break; // violet
        default: f.r = 0.98f; f.g = 0.78f; f.b = 0.20f; break; // golden
        }
        if (f.type == 1) { f.size = 0.55f + (rand() % 100) * 0.004f; }
        if (f.type == 2) {
            // bright glowing jellyfish colours: pink, violet, cyan,
            // orange, lime-green, sky blue
            int jc = rand() % 6;
            switch (jc) {
            case 0: f.r = 1.00f; f.g = 0.42f; f.b = 0.85f; break; // neon pink
            case 1: f.r = 0.85f; f.g = 0.38f; f.b = 1.00f; break; // violet
            case 2: f.r = 0.35f; f.g = 0.90f; f.b = 1.00f; break; // bright cyan
            case 3: f.r = 1.00f; f.g = 0.60f; f.b = 0.25f; break; // orange
            case 4: f.r = 0.45f; f.g = 1.00f; f.b = 0.65f; break; // lime-green
            default: f.r = 0.35f; f.g = 0.68f; f.b = 1.00f; break; // sky blue
            }
            f.y -= 2.0f;
        }
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

    // Kelp tufts: mostly spread across the whole sea floor with just a
    // smaller thicker tuft at each reef so it never all bunches in the middle
    kelp.clear();
    for (int i = 0; i < 150; i++) {
        Kelp k;
        if (rand() % 100 < 35) {
            int reef = rand() % 6;
            k.x = reefX[reef] + (rand() % 100 - 50) * 0.10f;
            k.z = reefZ[reef] + (rand() % 100 - 50) * 0.10f;
        } else {
            k.x = (rand() % 800 - 400) * 0.1f; // -40 .. 40
            k.z = (rand() % 800 - 400) * 0.1f;
        }
        k.h = 4.0f + (rand() % 100) * 0.05f;
        k.phase = (rand() % 1000) * 0.01f;
        kelp.push_back(k);
    }

    // Coral spread right across the sea floor
    corals.clear();
    for (int i = 0; i < 70; i++) {
        Coral c;
        c.x = (rand() % 700 - 350) * 0.1f;
        c.y = -16.0f;
        c.z = (rand() % 700 - 350) * 0.1f;
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
    CrewMember c1 = { 0.22f, -0.10f, 0.62f, 0, 0, 0 };   // pilot 1 (starboard seat)
    CrewMember c2 = { -1.05f, 0.22f, 0.30f, 0, 0, 1 };   // crew standing, starboard mess
    CrewMember c3 = { 0.00f, 0.20f, -0.15f, PI, 0, 2 };   // crew standing, center
    CrewMember c4 = { -0.90f, 0.22f, -0.55f, 0, 0, 1 };   // crew standing, aft port
    CrewMember c5 = { 0.22f, -0.10f, -0.62f, PI, 0, 0 };   // pilot 2 (port seat)
    crew.push_back(c1);
    crew.push_back(c2);
    crew.push_back(c3);
    crew.push_back(c4);
    crew.push_back(c5);

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

    trees.clear();
    // Forested coastline: dense trees along the near hillsides plus a
    // scattering of shoreline trees closer to the water.
    for (int i = 0; i < 110; i++) {
        Tree t;
        if (i < 70) {
            // hillside forest band
            t.x = -88.0f + (rand() % 1760) * 0.1f;
            t.z = -50.0f - (rand() % 300) * 0.1f; // -50 .. -80
        } else {
            // shoreline scatter closer to the water
            float angle = (rand() % 360) * DEG_TO_RAD;
            float rad = 56.0f + (rand() % 90) * 0.1f;
            t.x = cos(angle) * rad + (rand() % 40 - 20) * 0.1f;
            t.z = sin(angle) * rad + (rand() % 40 - 20) * 0.1f;
            if (t.z > -30.0f) t.z = -30.0f;
        }
        t.height = 2.2f + (rand() % 100) * 0.04f;
        // occasional tall landmark tree near the shore
        if (rand() % 25 == 0) t.height += 2.5f;
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
    float hullR = 1.35f;
    float hullDark[3] = { 0.19f, 0.20f, 0.22f };
    float hullSide[3] = { 0.22f, 0.23f, 0.25f };
    float hullTopC[3] = { 0.26f, 0.27f, 0.29f };
    float hullRed[3] = { 0.72f, 0.14f, 0.14f };

    bool useCamo = texHull != 0;
    if (useCamo) { glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, texHull); glColor3f(1,1,1); }
    glPushMatrix();
    if (!useCamo) glColor3fv(hullDark);
    glRotatef(90, 0, 1, 0);
    glTranslatef(0, 0, -2.5f);
    GLUquadric* hullQ = gluNewQuadric();
    gluQuadricNormals(hullQ, GLU_SMOOTH);
    if (useCamo) gluQuadricTexture(hullQ, GL_TRUE);
    gluCylinder(hullQ, hullR, hullR, 5.0f, 48, 6);
    gluDeleteQuadric(hullQ);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(2.85f, 0, 0);
    if (!useCamo) glColor3fv(hullDark);
    else glColor3f(1,1,1);
    glScalef(1.0f, 0.98f, 0.96f);
    drawSphere(hullR, 24, 18);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.8f, hullR - 0.20f, 0);
    glColor3fv(hullTopC);
    glScalef(3.4f, 0.20f, 0.45f);
    drawSphere(0.5f, 24, 12);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-2.5f, 0, 0);
    glRotatef(-90, 0, 1, 0);
    if (!useCamo) glColor3fv(hullDark);
    else glColor3f(1,1,1);
    GLUquadric* taperQ = gluNewQuadric();
    gluQuadricNormals(taperQ, GLU_SMOOTH);
    if (useCamo) gluQuadricTexture(taperQ, GL_TRUE);
    gluCylinder(taperQ, hullR, 0.30f, 1.7f, 22, 3);
    gluDeleteQuadric(taperQ);
    glPopMatrix();
    if (useCamo) { glBindTexture(GL_TEXTURE_2D, 0); glDisable(GL_TEXTURE_2D); }
    glPushMatrix();
    glTranslatef(0.0f, -hullR + 0.28f, 0);
    glColor3fv(hullRed);
    glScalef(6.8f, 0.24f, 1.12f);
    drawSphere(0.52f, 24, 12);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0f, -hullR * 0.88f, 0);
    glColor3fv(hullRed);
    glScalef(6.2f, 0.08f, 1.00f);
    drawCube(1.0f);
    glPopMatrix();
    glPushMatrix(); glTranslatef(3.55f, -hullR * 0.55f, 0); glColor3fv(hullRed); glScalef(0.55f, 0.28f, 0.55f); drawSphere(0.42f, 16, 12); glPopMatrix();
    glPushMatrix(); glTranslatef(0.35f, hullR - 0.06f, 0); glColor3f(0.08f,0.09f,0.10f); glScalef(4.85f, 0.06f, 0.22f); drawCube(1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.35f, hullR - 0.02f, 0); glColor3f(0.04f,0.05f,0.06f); glScalef(4.85f, 0.015f, 0.16f); drawCube(1.0f); glPopMatrix();
    glDisable(GL_LIGHTING);
    for(int k=0;k<4;k++){ glPushMatrix(); glTranslatef(1.75f- k*0.22f, 0.12f, hullR - 0.15f); glColor3f(0.92f,0.92f,0.92f); glScalef(0.28f,0.025f,0.01f); drawCube(1.0f); glPopMatrix(); glPushMatrix(); glTranslatef(1.75f- k*0.22f, 0.12f,-(hullR - 0.15f)); glScalef(0.28f,0.025f,0.01f); drawCube(1.0f); glPopMatrix(); }
    for(int k=0;k<5;k++){ glPushMatrix(); glTranslatef(-1.15f- k*0.20f, 0.42f, hullR - 0.15f); glColor3f(0.92f,0.92f,0.92f); glScalef(0.18f,0.02f,0.01f); drawCube(1.0f); glPopMatrix(); glPushMatrix(); glTranslatef(-1.15f- k*0.20f, 0.42f,-(hullR - 0.15f)); glScalef(0.18f,0.02f,0.01f); drawCube(1.0f); glPopMatrix(); }
    glEnable(GL_LIGHTING);

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
        float zside = s * hullR;
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

    float mastX2[] = { -0.62f, -0.48f, -0.36f, -0.24f, -0.72f };
    float mastH2[] = { 1.90f, 1.40f, 1.65f, 1.25f, 1.05f };
    float mastR2[] = { 0.022f, 0.018f, 0.020f, 0.016f, 0.014f };
    for (int m = 0; m < 5; m++) {
        glPushMatrix();
        glTranslatef(mastX2[m], hullR + 0.40f, 0);
        glColor3f(0.08f, 0.08f, 0.09f);
        drawCylinder(mastR2[m], mastH2[m], 8);
        glTranslatef(0, mastH2[m], 0);
        glColor3f(0.12f, 0.12f, 0.13f);
        if (m==1||m==3) drawSphere(mastR2[m] + 0.012f, 8, 6); else drawSphere(mastR2[m] + 0.008f, 8, 6);
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
        glColor3fv(hullRed);
        glScalef(0.70f, 0.05f, 0.60f);
        drawCube(1.0f);
        glPopMatrix();
    }
    glPushMatrix();
    glTranslatef(-3.80f, 0.45f, 0);
    glRotatef(18, 0, 0, 1);
    glColor3fv(hullRed);
    glScalef(0.70f, 0.75f, 0.05f);
    drawCube(1.0f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-3.80f, -0.38f, 0);
    glRotatef(-18, 0, 0, 1);
    glColor3fv(hullRed);
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
    if (!sub.headlightsOn || sub.y > -1.5f) { glDisable(GL_LIGHT2); return; }
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
    glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 38.0f);
    glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 3.0f);
    glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, 0.2f);
    glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, 0.02f);
    glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 0.003f);
}

void drawHeadlightBeam(float lx, float ly, float lz, float len, float farR) {
    glPushMatrix();
    glTranslatef(lx, ly, lz);
    glRotatef(90, 0, 1, 0);
    GLUquadric* q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);
    gluCylinder(q, 0.18f, farR, len, 14, 1);
    gluDeleteQuadric(q);
    glPopMatrix();
}

void drawSubmarineHeadlights() {
    if (!sub.headlightsOn || sub.y > -1.5f) return;
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(1.0f, 0.45f, 0.20f, 0.10f);
    drawHeadlightBeam(3.88f, 0.02f, 0.0f, 14.0f, 9.0f);
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
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
    drawSubmarineBody();
    glEnable(GL_FOG);
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
        glRotatef(-90, 1, 0, 0);
        glRotatef(180, 0, 0, 1);
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
    // Realistic low sun: hard bright core, layered corona and a warm
    // amber haze hugging the horizon wherever it dips.
    float diveBlend = 1.0f - diveTransition;
    if (diveBlend <= 0) return;

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    float t = introTimer * 0.001f;
    float puls = 1.0f + sin(t * 0.7f) * 0.03f;

    // hard bright core
    glColor3f(1.0f * diveBlend, 0.96f * diveBlend, 0.85f * diveBlend);
    glPushMatrix();
    glTranslatef(-165.0f, 22.0f, -180.0f);
    drawSphere(9.5f * puls, 24, 18);
    glPopMatrix();

    if (texPuff) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texPuff);
        // layered corona: tight warm, wide orange, huge faint
        glColor4f(1.0f, 0.90f, 0.62f, 0.55f * diveBlend);
        drawBillboard(-165.0f, 22.0f, -180.0f, 26.0f, 26.0f);
        glColor4f(1.0f, 0.75f, 0.42f, 0.34f * diveBlend);
        drawBillboard(-165.0f, 22.0f, -180.0f, 48.0f, 48.0f);
        glColor4f(1.0f, 0.60f, 0.30f, 0.16f * diveBlend);
        drawBillboard(-165.0f, 22.0f, -180.0f, 88.0f, 88.0f);
        // low amber haze hugging the horizon around the sun
        glColor4f(1.0f, 0.55f, 0.28f, 0.10f * diveBlend);
        drawBillboard(-165.0f, 18.0f, -180.0f, 150.0f, 34.0f);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    }
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    // Faint distant sun shimmer only, well away from the boat: the old code
    // painted a broad orange-brown fan right in front of the submarine on
    // the start scene, which read as mud floating in the sea.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(1.0f, 0.94f, 0.80f, 0.06f * diveBlend);
    glBegin(GL_QUADS);
    glVertex3f(-38, 0.15f, -40);
    glVertex3f(-28, 0.15f, -40);
    glVertex3f(-22, 0.15f, -25);
    glVertex3f(-30, 0.15f, -25);
    glEnd();
    glColor4f(1.0f, 0.90f, 0.72f, 0.04f * diveBlend);
    glBegin(GL_QUADS);
    glVertex3f(-44, 0.15f, -40);
    glVertex3f(-22, 0.15f, -40);
    glVertex3f(-18, 0.15f, -25);
    glVertex3f(-34, 0.15f, -25);
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawDistantHills() {
    // Real coastline: layered ridgelines with atmospheric haze at the
    // waterline, mottled vegetation, and warm sunlight on the left (sun side).
    float diveBlend = 1.0f - diveTransition;
    if (diveBlend <= 0) return;
    glDisable(GL_LIGHTING);
    glPushMatrix();

    // farthest hazy ridge (drawn first so nearer hills overlap it)
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(-110, 0, -80);
    for (int i = 0; i <= 20; i++) {
        float t = i / 20.0f;
        float hx = -110 + t * 260.0f;
        float hy = 6.4f + sin(t * 11.0f) * 1.9f + sin(t * 31.0f) * 0.8f + sin(t * 73.0f) * 0.3f;
        float m = noise01(i * 3, 7, 91);
        glColor3f((0.20f * diveBlend + 0.06f) * (0.8f + m * 0.4f),
                  (0.30f * diveBlend + 0.07f) * (0.8f + m * 0.4f),
                  (0.38f * diveBlend + 0.08f) * (0.8f + m * 0.4f));
        glVertex3f(hx, hy, -80);
    }
    glEnd();

    // left long ridge, warm sunlit side, hazy at the waterline
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(-92, 0, -68);
    for (int i = 0; i <= 18; i++) {
        float t = i / 18.0f;
        float hx = -92 + t * 184.0f;
        float hy = 4.8f + sin(t * 13.0f) * 1.4f + sin(t * 29.0f) * 0.7f + sin(t * 53.0f) * 0.35f;
        float grad = (hy + 6.0f) / 15.0f; if (grad > 1) grad = 1; if (grad < 0) grad = 0;
        float warm = 1.0f - (hx + 92.0f) / 184.0f; if (warm < 0) warm = 0; if (warm > 1) warm = 1;
        float m = noise01(i * 5, 3, 12);
        float haze = 1.0f - grad; haze *= haze;
        float vr = (0.32f * diveBlend + 0.06f) * (0.66f + 0.36f * grad) + warm * 0.10f;
        float vg = (0.28f * diveBlend + 0.05f) * (0.70f + 0.24f * grad) + warm * 0.05f;
        float vb = (0.15f * diveBlend + 0.03f) * (0.76f + 0.20f * grad);
        float hR = (0.60f * diveBlend + 0.13f) * (1.0f - warm * 0.12f);
        float hG = (0.70f * diveBlend + 0.15f);
        float hB = (0.85f * diveBlend + 0.19f);
        glColor3f((vr * (1.0f - haze) + hR * haze) * (0.9f + m * 0.2f),
                  (vg * (1.0f - haze) + hG * haze) * (0.9f + m * 0.2f),
                  (vb * (1.0f - haze) + hB * haze) * (0.9f + m * 0.2f));
        glVertex3f(hx, hy, -68);
    }
    glEnd();

    // nearer blue-green ridge
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(-82, 0, -55);
    for (int i = 0; i <= 16; i++) {
        float t = i / 16.0f;
        float hx = -82 + t * 62.0f;
        float hy = 3.4f + sin(t * 7.0f) * 1.1f + sin(t * 19.0f) * 0.55f + sin(t * 37.0f) * 0.25f;
        float grad = (hy + 4.0f) / 11.0f; if (grad > 1) grad = 1; if (grad < 0) grad = 0;
        float m = noise01(i * 7, 13, 23);
        float haze = 1.0f - grad; haze *= haze;
        float vr = (0.30f * diveBlend + 0.05f) * (0.64f + 0.38f * grad);
        float vg = (0.25f * diveBlend + 0.04f) * (0.70f + 0.24f * grad);
        float vb = (0.14f * diveBlend + 0.02f) * (0.76f + 0.20f * grad);
        float hR = (0.62f * diveBlend + 0.13f);
        float hG = (0.72f * diveBlend + 0.15f);
        float hB = (0.86f * diveBlend + 0.19f);
        glColor3f((vr * (1.0f - haze) + hR * haze) * (0.88f + m * 0.24f),
                  (vg * (1.0f - haze) + hG * haze) * (0.88f + m * 0.24f),
                  (vb * (1.0f - haze) + hB * haze) * (0.88f + m * 0.24f));
        glVertex3f(hx, hy, -55);
    }
    glEnd();

    // near green ridge on the right
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(6, 0, -57);
    for (int i = 0; i <= 14; i++) {
        float t = i / 14.0f;
        float hx = 6 + t * 78.0f;
        float hy = 3.0f + sin(t * 9.0f) * 0.85f + sin(t * 23.0f) * 0.40f + sin(t * 43.0f) * 0.20f;
        float grad = (hy + 3.5f) / 10.0f; if (grad > 1) grad = 1; if (grad < 0) grad = 0;
        float m = noise01(i * 11, 17, 33);
        float haze = 1.0f - grad; haze *= haze;
        float vr = (0.20f * diveBlend + 0.04f) * (0.66f + 0.34f * grad);
        float vg = (0.31f * diveBlend + 0.05f) * (0.70f + 0.22f * grad);
        float vb = (0.18f * diveBlend + 0.03f) * (0.76f + 0.18f * grad);
        float hR = (0.60f * diveBlend + 0.13f);
        float hG = (0.70f * diveBlend + 0.15f);
        float hB = (0.84f * diveBlend + 0.19f);
        glColor3f((vr * (1.0f - haze) + hR * haze) * (0.86f + m * 0.26f),
                  (vg * (1.0f - haze) + hG * haze) * (0.86f + m * 0.26f),
                  (vb * (1.0f - haze) + hB * haze) * (0.86f + m * 0.26f));
        glVertex3f(hx, hy, -57);
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
        glTranslatef(deckX[i], 1.38f, 0.02f);
        drawSphere(0.07f, 6, 5); // head
        glTranslatef(0, -0.16f, 0);
        glScalef(0.09f, 0.26f, 0.09f);
        drawCube(1.0f); // body
        glPopMatrix();
    }
    // 2 figures on top of sail
    for (int i = 0; i < 2; i++) {
        glPushMatrix();
        glTranslatef(0.35f + i * 0.25f, 2.15f, 0);
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

void drawBirds() {
    // Small seabird flocks gliding and flapping over the water
    float diveBlend = 1.0f - diveTransition;
    if (diveBlend <= 0) return;

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float t = introTimer * 0.001f;

    for (int f = 0; f < 4; f++) {
        float bx = -95.0f + f * 52.0f + sin(t * 0.05f + f * 2.1f) * 7.0f;
        float by = 30.0f + f * 12.0f;
        float bz = -72.0f - f * 16.0f;
        for (int b = 0; b < 6; b++) {
            float offx = (b - 2) * 1.5f + sin(t * 0.6f + b + f) * 0.5f;
            float offy = sin(t * 0.8f + b * 1.7f + f) * 0.5f;
            float offz = (b % 2) * 1.4f;
            float flapY = sin(t * 9.0f + b * 1.3f + f * 2.0f) * 0.55f + 0.22f;
            float ws = 0.95f + (b % 3) * 0.14f;
            float heading = (b % 2) ? -7.0f : 7.0f;
            glPushMatrix();
            glTranslatef(bx + offx, by + offy, bz + offz);
            glRotatef(heading, 0, 1, 0);
            glColor4f(0.08f, 0.10f, 0.12f, 0.85f * diveBlend);
            // left wing (flaps up and down around the body axis)
            glBegin(GL_TRIANGLES);
            glVertex3f(0, 0, 0);
            glVertex3f(-ws, flapY, -ws * 0.42f);
            glVertex3f(-ws * 0.28f, flapY * 0.6f, -ws * 0.95f);
            glEnd();
            // right wing
            glBegin(GL_TRIANGLES);
            glVertex3f(0, 0, 0);
            glVertex3f(ws * 0.28f, flapY * 0.6f, -ws * 0.95f);
            glVertex3f(ws, flapY, -ws * 0.42f);
            glEnd();
            // tail
            glBegin(GL_TRIANGLES);
            glVertex3f(0, 0, 0);
            glVertex3f(-ws * 0.26f, 0.05f, -ws * 0.30f);
            glVertex3f(ws * 0.26f, 0.05f, -ws * 0.30f);
            glEnd();
            // body
            glBegin(GL_TRIANGLES);
            glVertex3f(0, 0, 0);
            glVertex3f(-ws * 0.28f, -0.03f, ws * 0.55f);
            glVertex3f(ws * 0.28f, -0.03f, ws * 0.55f);
            glEnd();
            glPopMatrix();
        }
    }

    glDisable(GL_BLEND);
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
            // Sun glimmer near sunset side (-X, toward sun): soft warm-silver
            // sparkle only, kept subtle so it never reads as mud in the water
            float lane = 1.0f - fabs((x0 + 26.0f) / 22.0f);
            if (lane < 0) lane = 0;
            float glint = pow(sin(x0 * 2.1f + introTimer * 0.01f) * sin(z0 * 1.7f - introTimer * 0.008f), 8.0f);
            if (glint < 0) glint = 0;
            r += lane * (0.06f + glint * 0.22f) * diveBlend;
            g += lane * (0.07f + glint * 0.24f) * diveBlend;
            b += lane * (0.04f + glint * 0.18f) * diveBlend;

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

    // Rolling sea-bed hills: real underwater terrain with slope lighting,
    // drifting caustic patches that fade with depth, and a floor that
    // darkens to deep algal blue the deeper the boat goes.
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    bool sandTex = (texSand != 0);
    if (sandTex) { glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, texSand); }
    glBegin(GL_QUADS);
    for (int i = -gridSize; i < gridSize; i++) {
        for (int j = -gridSize; j < gridSize; j++) {
            float x0 = i * gridStep;
            float z0 = j * gridStep;
            float x1 = (i + 1) * gridStep;
            float z1 = (j + 1) * gridStep;
            float hA = seaFloorY(x0, z0);
            float hB = seaFloorY(x1, z0);
            float hC = seaFloorY(x1, z1);
            float hD = seaFloorY(x0, z1);

            // true slope normal computed from the quad's edges
            float nx = hA - hB;
            float nz = hA - hD;
            float ny = gridStep;
            float nl = sqrt(nx * nx + ny * ny + nz * nz);
            nx /= nl; ny /= nl; nz /= nl;

            float ca = sin(x0 * 0.8f + introTimer * 0.0012f) * cos(z0 * 0.7f - introTimer * 0.001f);
            ca = ca * ca * (1.0f - diveTransition * 0.7f); // caustics fade with depth
            float sandTone = 0.9f + (hA + 16.0f) * 0.08f;
            // bright sand near the surface -> dark algal blue at depth
            float deep = diveTransition;
            float rr = (0.78f * sandTone + ca * 0.22f) * (1.0f - 0.45f * deep) + 0.05f * deep;
            float gg = (0.73f * sandTone + ca * 0.18f) * (1.0f - 0.45f * deep) + 0.15f * deep;
            float bb = (0.56f * sandTone + ca * 0.12f) * (1.0f - 0.45f * deep) + 0.22f * deep;
            glColor3f(rr, gg, bb);
            glNormal3f(nx, ny, nz);
            if (sandTex) glTexCoord2f(x0 * 0.08f, z0 * 0.08f);
            glVertex3f(x0, hA, z0);
            if (sandTex) glTexCoord2f(x1 * 0.08f, z0 * 0.08f);
            glVertex3f(x1, hB, z0);
            if (sandTex) glTexCoord2f(x1 * 0.08f, z1 * 0.08f);
            glVertex3f(x1, hC, z1);
            if (sandTex) glTexCoord2f(x0 * 0.08f, z1 * 0.08f);
            glVertex3f(x0, hD, z1);
        }
    }
    glEnd();
    if (sandTex) { glBindTexture(GL_TEXTURE_2D, 0); glDisable(GL_TEXTURE_2D); }
    glPopMatrix();
}

void drawTrees() {
    // Orientation-safe trees built from stacked spheres (no gluCylinder, so
    // they never render as horizontal brown sticks floating in the water).
    // A forested band climbs the near hillsides; bases sit near the waterline.
    float diveBlend = 1.0f - diveTransition;
    if (diveBlend <= 0) return;
    glDisable(GL_LIGHTING);
    for (size_t i = 0; i < trees.size(); i++) {
        Tree& tr = trees[i];
        float h = tr.height;
        // bases climb gently up the hillside the further inland they sit
        float baseY = -0.20f + (-tr.z - 48.0f) * 0.045f;
        if (baseY < -0.20f) baseY = -0.20f;
        float lean = sin(i * 1.7f) * 2.0f;
        float hue = ((i * 37) % 10) / 10.0f;
        bool pine = ((i * 7) % 3) != 0;

        glPushMatrix();
        glTranslatef(tr.x, baseY, tr.z);
        glRotatef(lean, 0, 1, 0);

        // tapered trunk (stacked spheres)
        glColor3f(0.30f * diveBlend, 0.19f * diveBlend, 0.10f * diveBlend);
        drawSphere(0.10f, 6, 4);
        glTranslatef(0, h * 0.10f, 0);
        drawSphere(0.07f, 6, 4);
        glTranslatef(0, h * 0.09f, 0);
        glColor3f(0.22f * diveBlend, 0.14f * diveBlend, 0.08f * diveBlend);
        drawSphere(0.045f, 6, 4);

        if (pine) {
            // conifer: tapering layered foliage
            for (int L = 0; L < 4; L++) {
                float fy = h * (0.20f + L * 0.19f);
                float fr = h * (0.17f - L * 0.027f);
                if (fr < 0.03f) fr = 0.03f;
                float sh = 1.0f - L * 0.10f;
                float rr = (0.10f + hue * 0.06f) * sh;
                float gg = (0.34f + hue * 0.12f) * sh;
                float bb = (0.10f + hue * 0.04f) * sh;
                glPushMatrix();
                glTranslatef(0, fy, 0);
                glScalef(1.0f, 1.0f, 0.82f);
                glColor3f(rr * diveBlend, gg * diveBlend, bb * diveBlend);
                drawSphere(fr, 8, 6);
                glPopMatrix();
            }
            // crown spike
            glPushMatrix();
            glTranslatef(0, h * 0.96f, 0);
            glColor3f(0.16f * diveBlend, 0.32f * diveBlend, 0.12f * diveBlend);
            drawSphere(h * 0.06f, 6, 4);
            glPopMatrix();
        } else {
            // broadleaf: irregular canopy of overlapping green puffs
            for (int p = 0; p < 6; p++) {
                float ang = p * 1.047f;
                float px = sin(ang) * h * 0.14f;
                float pz = cos(ang) * h * 0.12f;
                float pr = h * (0.15f + ((p * 53) % 9) * 0.012f);
                float sh = 0.9f + ((p * 29) % 5) * 0.06f;
                float rr = (0.10f + hue * 0.06f) * sh;
                float gg = (0.30f + hue * 0.10f) * sh;
                float bb = (0.09f + hue * 0.03f) * sh;
                glPushMatrix();
                glTranslatef(px, h * (0.32f + p * 0.045f), pz);
                glColor3f(rr * diveBlend, gg * diveBlend, bb * diveBlend);
                drawSphere(pr, 8, 6);
                glPopMatrix();
            }
        }
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
}

void drawSeaweed() {
    for (size_t i = 0; i < seaweeds.size(); i++) {
        glPushMatrix();
        float sway = sin(introTimer * 0.003f + seaweeds[i].phase) * 0.2f;
        glTranslatef(seaweeds[i].x, seaFloorY(seaweeds[i].x, seaweeds[i].z), seaweeds[i].z);

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
        glTranslatef(rocks[i].x, seaFloorY(rocks[i].x, rocks[i].z) - rocks[i].scaleY * 0.25f, rocks[i].z);
        glScalef(rocks[i].scaleX, rocks[i].scaleY, rocks[i].scaleZ);
        glColor3f(rocks[i].r, rocks[i].g, rocks[i].b);
        drawSphere(1.0f, 8, 6);
        glPopMatrix();
    }
}

void drawCoral() {
    for (size_t i = 0; i < corals.size(); i++) {
        glPushMatrix();
        glTranslatef(corals[i].x, seaFloorY(corals[i].x, corals[i].z), corals[i].z);

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

// Crew cabin occupies world space around the sub - keep sea life out of
// the room in crew view (they still swim past right outside the window)
bool insideCrewCabin(float x, float y, float z) {
    if (cameraMode != CAM_CONTROL_SCREEN) return false;
    if (fabs(x - sub.x) > 3.4f) return false;
    if (fabs(z - sub.z) > 2.6f) return false;
    if (y < sub.y - 1.4f || y > sub.y + 2.6f) return false;
    return true;
}

void drawFish() {
    float t = introTimer * 0.001f;

    // Keep fish colours rich and saturated (no bright white specular wash)
    GLfloat fishSpec[] = { 0.10f, 0.12f, 0.16f, 1.0f };
    GLfloat fishShine[] = { 6.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, fishSpec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, fishShine);

    for (size_t i = 0; i < fishes.size(); i++) {
        Fish& f = fishes[i];
        if (insideCrewCabin(f.x, f.y, f.z)) continue;
        glPushMatrix();
        glTranslatef(f.x, f.y, f.z);

        float displayAngle = f.angle * 180.0f / PI;
        glRotatef(displayAngle, 0, 1, 0);

        float tailWag = sin(t * 10 + f.animPhase) * 15.0f;

        if (f.type == 0 || f.type == 1) {
            // Realistic fish: tapered fusiform body, forked tail,
            // dorsal + pectoral fins and a real eye on each side.
            float s = f.size;
            // darker shade for the fins
            float dr = f.r * 0.55f, dg = f.g * 0.55f, db = f.b * 0.55f;

            glPushMatrix();
            glScalef(s, s, s);

            // ---- Body: overlapping spheres tapered from head to peduncle ----
            // (x offset along the fish, cross-section radius at that point)
            const float bx[7] = { 0.72f, 0.48f, 0.20f, -0.08f, -0.36f, -0.62f, -0.85f };
            const float br[7] = { 0.26f, 0.46f, 0.56f, 0.58f, 0.50f, 0.36f, 0.19f };
            glPushMatrix();
            glScalef(1.0f, 0.62f, 0.50f);
            for (int k = 0; k < 7; k++) {
                glPushMatrix();
                glTranslatef(bx[k], 0, 0);
                glColor3f(f.r, f.g, f.b);
                drawSphere(br[k], 14, 9);
                glPopMatrix();
            }
            glPopMatrix();

            // ---- Forked tail (same side-to-side wag) ----
            glPushMatrix();
            glTranslatef(-1.02f, 0, 0);
            glRotatef(tailWag, 0, 1, 0);
            glColor3f(dr, dg, db);
            glBegin(GL_TRIANGLES);
            // upper lobe
            glNormal3f(0, 0, 1);
            glVertex3f(0.02f, -0.02f, 0);
            glVertex3f(-0.62f, 0.46f, 0);
            glVertex3f(-0.20f, 0.00f, 0);
            // lower lobe
            glVertex3f(0.02f, 0.02f, 0);
            glVertex3f(-0.62f, -0.46f, 0);
            glVertex3f(-0.20f, 0.00f, 0);
            // mirrored side so the fin reads from both directions
            glNormal3f(0, 0, -1);
            glVertex3f(0.02f, -0.02f, 0);
            glVertex3f(-0.20f, 0.00f, 0);
            glVertex3f(-0.62f, 0.46f, 0);
            glVertex3f(0.02f, 0.02f, 0);
            glVertex3f(-0.20f, 0.00f, 0);
            glVertex3f(-0.62f, -0.46f, 0);
            glEnd();
            glPopMatrix();

            // ---- Dorsal fin ----
            for (int zOff = -1; zOff <= 1; zOff++) {
                if (zOff == 0) continue;
                glColor3f(dr, dg, db);
                glBegin(GL_TRIANGLES);
                glNormal3f(0, 1, 0);
                glVertex3f(-0.58f, 0.26f, zOff * 0.02f);
                glVertex3f(0.32f, 0.30f, zOff * 0.02f);
                glVertex3f(-0.14f, 0.64f, zOff * 0.02f);
                glEnd();
            }

            // ---- Pectoral fins (one on each side) ----
            for (int side = -1; side <= 1; side += 2) {
                glPushMatrix();
                glTranslatef(0.20f, -0.04f, side * 0.22f);
                glRotatef(side * 30.0f, 0, 0, 1);
                glColor3f(dr, dg, db);
                glScalef(0.55f, 0.12f, 0.30f);
                drawSphere(0.30f, 6, 4);
                glPopMatrix();
            }

            // ---- Eyes (both sides) ----
            for (int side = -1; side <= 1; side += 2) {
                glPushMatrix();
                glTranslatef(0.46f, 0.20f, side * 0.30f);
                glColor3f(0.95f, 0.97f, 1.0f);
                drawSphere(0.10f, 6, 4);
                glTranslatef(0.03f, 0.01f, side * 0.05f);
                glColor3f(0.06f, 0.06f, 0.08f);
                drawSphere(0.05f, 6, 4);
                glPopMatrix();
            }

            glPopMatrix();
        } else if (f.type == 2) {
            // Real jellyfish: translucent pulsing dome, glowing core,
            // frilly oral arms and long trailing stinging tentacles.
            float s = f.size;
            float pulse = sin(t * 3 + f.animPhase) * 0.15f;
            float bellR = 0.95f * (1.0f + pulse * 0.10f);
            float bellH = 0.80f * (1.0f - pulse * 0.12f);

            glDisable(GL_LIGHTING);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glPushMatrix();
            glScalef(s, s, s);

            // ---- translucent glowing bell dome (each jelly its own colour) ----
            const int LON = 16;
            glColor4f(f.r, f.g, f.b, 0.45f);
            glBegin(GL_TRIANGLE_FAN);
            glVertex3f(0, bellH, 0);
            for (int n = 0; n <= LON; n++) {
                float th = (float)n / LON * 2.0f * PI;
                float r = sin(12.0f * DEG_TO_RAD) * bellR;
                float y = cos(12.0f * DEG_TO_RAD) * bellH;
                glVertex3f(cos(th) * r, y, sin(th) * r);
            }
            glEnd();
            for (int k = 0; k < 6; k++) {
                float lat0 = (12.0f + k * 13.0f) * DEG_TO_RAD;
                float lat1 = (12.0f + (k + 1) * 13.0f) * DEG_TO_RAD;
                glBegin(GL_QUAD_STRIP);
                for (int n = 0; n <= LON; n++) {
                    float th = (float)n / LON * 2.0f * PI;
                    float a0 = cos(th) * sin(lat0) * bellR, b0 = cos(lat0) * bellH;
                    float a1 = cos(th) * sin(lat1) * bellR, b1 = cos(lat1) * bellH;
                    glVertex3f(a0, b0, sin(th) * sin(lat0) * bellR);
                    glVertex3f(a1, b1, sin(th) * sin(lat1) * bellR);
                }
                glEnd();
            }

            // ---- bright glowing core inside the bell ----
            glPushMatrix();
            glTranslatef(0, bellH * 0.35f, 0);
            glColor4f(f.r * 0.7f + 0.3f, f.g * 0.7f + 0.3f, f.b * 0.7f + 0.3f, 0.55f);
            glScalef(0.55f, 0.35f, 0.55f);
            drawSphere(1.0f, 8, 6);
            glPopMatrix();

            // ---- oral arms: short frilly skirt hanging from the rim ----
            for (int k = 0; k < 4; k++) {
                float ang = k * 90.0f * DEG_TO_RAD;
                float swayA = sin(t * 1.6f + k + f.animPhase) * 8.0f;
                glPushMatrix();
                glTranslatef(cos(ang) * 0.10f, -0.22f, sin(ang) * 0.10f);
                glRotatef(swayA, cos(ang), 0, sin(ang));
                glColor4f(f.r * 0.8f, f.g * 0.8f, f.b * 0.8f, 0.45f);
                glScalef(0.09f, 0.24f, 0.09f);
                drawSphere(1.0f, 6, 4);
                glPopMatrix();
            }

            // ---- long trailing stinging tentacles around the rim ----
            for (int k = 0; k < 18; k++) {
                float th = (float)k / 18.0f * 2.0f * PI;
                float lenT = 0.7f + (k % 5) * 0.14f;
                float tipX = cos(th) * bellR * 0.97f;
                float tipZ = sin(th) * bellR * 0.97f;
                float sway1 = sin(t * 2.2f + k) * 0.05f;
                float sway2 = sin(t * 1.7f + k * 1.3f) * 0.08f;
                glBegin(GL_LINE_STRIP);
                glColor4f(f.r, f.g, f.b, 0.10f);
                glVertex3f(tipX, 0.05f, tipZ);
                glColor4f(f.r, f.g, f.b, 0.35f);
                glVertex3f(tipX + sway1, -lenT * 0.45f, tipZ + sway1);
                glColor4f(f.r * 0.8f, f.g * 0.8f, f.b * 0.8f, 0.12f);
                glVertex3f(tipX + sway1 + sway2, -lenT, tipZ - sway2);
                glEnd();
            }

            // ---- additive halo: a soft bright blob around the whole jelly ----
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glPushMatrix();
            glColor4f(f.r, f.g, f.b, 0.20f);
            glScalef(1.4f, 0.95f, 1.4f);
            drawSphere(1.0f, 12, 8);
            glPopMatrix();
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glPopMatrix();
            glDisable(GL_BLEND);
            glEnable(GL_LIGHTING);
        }
        glPopMatrix();
    }

    // restore neutral material so the rest of the scene is not glossy
    GLfloat noSpec[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat noShine[] = { 0.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, noSpec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, noShine);
}

void drawSharks() {
    for (size_t i = 0; i < sharks.size(); i++) {
        Shark& s = sharks[i];
        float sx = s.cx + cos(s.angle) * s.radius;
        float sz = s.cz + sin(s.angle) * s.radius;
        if (insideCrewCabin(sx, s.cy, sz)) continue;
        float tailWag = sin(introTimer * 0.004f + i * 2.0f) * 12.0f;
        glPushMatrix();
        glTranslatef(sx, s.cy, sz);
        glRotatef(-(s.angle * 180.0f / PI) + 90.0f, 0, 1, 0);
        glScalef(s.size, s.size, s.size);

        // deep steel blue-grey — no more pale / white-looking sharks
        float topR = 0.30f, topG = 0.38f, topB = 0.48f;
        float finR = 0.24f, finG = 0.30f, finB = 0.38f;
        GLfloat sharkSpec[] = { 0.10f, 0.12f, 0.16f, 1.0f };
        GLfloat sharkShine[] = { 6.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, sharkSpec);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, sharkShine);

        // ---- tapered fusiform body (head +x) ----
        const float bx[9] = { 1.18f, 1.00f, 0.78f, 0.50f, 0.20f, -0.10f, -0.42f, -0.74f, -1.02f };
        const float br[9] = { 0.03f, 0.10f, 0.24f, 0.36f, 0.44f, 0.42f, 0.34f, 0.24f, 0.13f };
        glPushMatrix();
        glScalef(1.0f, 0.52f, 0.42f);
        for (int k = 0; k < 9; k++) {
            glPushMatrix();
            glTranslatef(bx[k], 0, 0);
            glColor3f(topR, topG, topB);
            drawSphere(br[k], 14, 9);
            glPopMatrix();
        }
        // pointed snout
        glColor3f(topR, topG, topB);
        glTranslatef(1.06f, 0, 0); drawSphere(0.15f, 8, 6);
        glTranslatef(0.14f, 0, 0); drawSphere(0.09f, 8, 6);
        glTranslatef(0.12f, 0, 0); drawSphere(0.035f, 8, 6);
        glPopMatrix();

        // ---- crescent tail (tall upper lobe, short lower) ----
        glPushMatrix();
        glTranslatef(-1.12f, 0, 0);
        glRotatef(tailWag, 0, 1, 0);
        glColor3f(finR, finG, finB);
        glBegin(GL_TRIANGLES);
        glNormal3f(0, 0, 1);
        glVertex3f(0.05f, -0.03f, 0);
        glVertex3f(-0.30f, 0.60f, 0);
        glVertex3f(-0.34f, 0.10f, 0);
        glVertex3f(0.05f, 0.03f, 0);
        glVertex3f(-0.34f, 0.10f, 0);
        glVertex3f(-0.30f, -0.25f, 0);
        glNormal3f(0, 0, -1);
        glVertex3f(0.05f, -0.03f, 0);
        glVertex3f(-0.34f, 0.10f, 0);
        glVertex3f(-0.30f, 0.60f, 0);
        glVertex3f(0.05f, 0.03f, 0);
        glVertex3f(-0.30f, -0.25f, 0);
        glVertex3f(-0.34f, 0.10f, 0);
        glEnd();
        glPopMatrix();

        // ---- tall raked dorsal fin ----
        for (int zOff = -1; zOff <= 1; zOff++) {
            if (zOff == 0) continue;
            glColor3f(finR, finG, finB);
            glBegin(GL_TRIANGLES);
            glNormal3f(0, 1, 0);
            glVertex3f(-0.20f, 0.34f, zOff * 0.03f);
            glVertex3f(0.42f, 0.36f, zOff * 0.03f);
            glVertex3f(0.00f, 1.05f, zOff * 0.03f);
            glEnd();
        }

        // ---- small second dorsal near the tail ----
        for (int zOff = -1; zOff <= 1; zOff++) {
            if (zOff == 0) continue;
            glColor3f(finR, finG, finB);
            glBegin(GL_TRIANGLES);
            glNormal3f(0, 1, 0);
            glVertex3f(-0.78f, 0.24f, zOff * 0.02f);
            glVertex3f(-0.52f, 0.26f, zOff * 0.02f);
            glVertex3f(-0.66f, 0.52f, zOff * 0.02f);
            glEnd();
        }

        // ---- long pectoral fins spread on the sides ----
        for (int side = -1; side <= 1; side += 2) {
            glColor3f(finR, finG, finB);
            glBegin(GL_TRIANGLES);
            glNormal3f(0, 0, (float)side);
            glVertex3f(0.42f, -0.14f, side * 0.24f);
            glVertex3f(-0.15f, -0.28f, side * 0.42f);
            glVertex3f(0.52f, -0.30f, side * 0.30f);
            glEnd();
            glBegin(GL_TRIANGLES);
            glNormal3f(0, 0, (float)side);
            glVertex3f(0.42f, -0.14f, side * 0.24f);
            glVertex3f(0.52f, -0.30f, side * 0.30f);
            glVertex3f(0.10f, -0.34f, side * 0.12f);
            glEnd();
        }

        // ---- small dark eyes low on the head ----
        for (int side = -1; side <= 1; side += 2) {
            glPushMatrix();
            glTranslatef(0.62f, 0.14f, side * 0.26f);
            glColor3f(0.05f, 0.06f, 0.08f);
            drawSphere(0.06f, 6, 4);
            glPopMatrix();
        }

        // ---- three dark gill slits on each side ----
        for (int side = -1; side <= 1; side += 2) {
            for (int g = 0; g < 3; g++) {
                float gx = 0.26f + g * 0.13f;
                glColor3f(0.10f, 0.13f, 0.17f);
                glBegin(GL_QUADS);
                glVertex3f(gx, 0.10f, side * 0.20f - 0.01f);
                glVertex3f(gx, 0.24f, side * 0.20f - 0.01f);
                glVertex3f(gx + 0.04f, 0.23f, side * 0.20f - 0.01f);
                glVertex3f(gx + 0.04f, 0.09f, side * 0.20f - 0.01f);
                glEnd();
            }
        }

        glPopMatrix();
    }
}

void drawRays() {
    for (size_t i = 0; i < rays.size(); i++) {
        Ray& r = rays[i];
        float rx = r.cx + cos(r.angle) * r.radius;
        float rz = r.cz + sin(r.angle) * r.radius;
        if (insideCrewCabin(rx, r.cy, rz)) continue;
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
        float baseY = seaFloorY(k.x, k.z);
        glPushMatrix();
        glTranslatef(k.x, baseY, k.z);
        glRotatef(lean, 0, 0, 1);

        // Tall leafy stem: big broad blades fan out alternately like a
        // giant sea palm, swaying with the current.
        glColor3f(0.16f, 0.26f, 0.05f);
        drawCylinder(0.10f, k.h, 8);

        for (int L = 0; L < 6; L++) {
            float ly = k.h * (0.35f + L * 0.10f);
            float swayL = sin(t * 2.0f + k.phase + L) * 6.0f;
            if (L % 2 == 0) {
                // one giant broad leaf
                glPushMatrix();
                glTranslatef(0, ly, 0);
                glRotatef(40.0f + swayL, 0, 0, 1);
                glRotatef(L * 60.0f, 0, 1, 0);
                glPushMatrix();
                glScalef(1.7f * (1.0f - L * 0.05f), 0.5f, 0.16f);
                glColor3f(0.30f, 0.42f, 0.08f);
                drawSphere(0.75f, 8, 5);
                glPopMatrix();
                glPopMatrix();
            } else {
                // pair of broad leaves opposite each other
                for (int s = -1; s <= 1; s += 2) {
                    glPushMatrix();
                    glTranslatef(0, ly, 0);
                    glRotatef(s * (38.0f - L * 2.0f) + swayL * s, 0, 0, 1);
                    glRotatef((L * 47 + s * 30), 0, 1, 0);
                    glPushMatrix();
                    glScalef(1.5f - L * 0.04f, 0.45f, 0.14f);
                    glColor3f(0.26f, 0.40f, 0.07f);
                    drawSphere(0.72f, 8, 5);
                    glPopMatrix();
                    glPopMatrix();
                }
            }
        }

        // Big top frond
        glPushMatrix();
        glTranslatef(0, k.h, 0);
        glPushMatrix();
        glScalef(0.30f, 1.6f, 0.20f);
        glColor3f(0.34f, 0.42f, 0.08f);
        drawSphere(0.7f, 8, 5);
        glPopMatrix();
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
        if (insideCrewCabin(b.x, b.y, b.z)) continue;
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
    float a = 0.032f * strength * depthFade;
    // The crew room's front bow window is the live external camera feed, so a
    // subtle shimmer is kept but the additive haze is heavily damped there.
    // For every other camera the full god-ray beam stays as it was.
    float camScale = (cameraMode == CAM_CONTROL_SCREEN) ? 0.30f : 1.0f;
    a *= camScale;

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
        glColor4f(0.30f, 0.60f, 0.92f, 0.0f);
        glVertex3f(rx - topW, 0.5f, rz);
        glVertex3f(rx + topW, 0.5f, rz);
        glColor4f(0.30f, 0.60f, 0.92f, a);
        glVertex3f(rx + botW + sway, -16.0f, rz);
        glVertex3f(rx - botW + sway, -16.0f, rz);
        glEnd();
    }
    // Bright surface sheet seen from below
    glBegin(GL_QUADS);
    glColor4f(0.35f, 0.65f, 0.9f, 0.35f * strength * camScale);
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
    GLfloat matSpec[] = { 0.55f, 0.57f, 0.62f, 1.0f };
    GLfloat matShine[] = { 48.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShine);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Floor / ceiling / back wall are drawn by drawCrewCabin in crew-room mode
    // so the two halves merge into one continuous hull with no z-fighting.
    if (cameraMode != CAM_CONTROL_SCREEN) {
    // Floor - dark brushed steel with subtle seams
    glPushMatrix();
    glColor3f(0.23f, 0.25f, 0.28f);
    glTranslatef(0.1f, -0.86f, 0);
    glScalef(3.2f, 0.06f, 2.6f);
    drawCube(1.0f);
    glPopMatrix();
    glPushMatrix(); glTranslatef(0.1f, -0.82f, 0); glColor3f(0.19f,0.20f,0.22f); glScalef(3.2f,0.005f,2.6f); drawCube(1.0f); glPopMatrix();
    for (int r = 0; r < 24; r++) {
        float rx = -1.3f + r * 0.11f;
        for (int k = 0; k < 2; k++) {
            float rz = k == 0 ? 1.22f : -1.22f;
            glPushMatrix();
            glTranslatef(rx, -0.82f, rz);
            glColor3f(0.13f, 0.14f, 0.16f);
            drawSphere(0.015f, 5, 4);
            glPopMatrix();
        }
    }

    // Ceiling - cold blue-grey steel, panel seams + white overhead duct like ref
    glPushMatrix();
    glTranslatef(0.05f, 1.22f, 0);
    glColor3f(0.27f, 0.31f, 0.36f);
    glScalef(3.15f, 0.06f, 2.55f);
    drawCube(1.0f);
    glPopMatrix();
    glPushMatrix(); glTranslatef(-0.65f, 1.18f, 0); glColor3f(0.82f,0.82f,0.83f); glScalef(1.25f,0.09f,0.22f); drawCube(1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.65f, 1.12f, 0); glColor3f(0.62f,0.62f,0.63f); glScalef(1.27f,0.01f,0.24f); drawCube(1.0f); glPopMatrix();
    for (int p = 0; p < 3; p++) {
        glPushMatrix();
        glTranslatef(-0.9f + p * 0.9f, 1.18f, 0);
        glColor3f(0.16f, 0.19f, 0.22f);
        glScalef(0.02f, 0.015f, 2.55f);
        drawCube(1.0f);
        glPopMatrix();
    }

    // Overhead pipe + 4 warm tungsten spots like reference (yellow, soft bloom)
    glPushMatrix();
    glTranslatef(0.0f, 1.12f, 0);
    glColor3f(0.68f, 0.67f, 0.65f);
    glScalef(2.9f, 0.04f, 0.04f);
    drawCube(1.0f);
    glPopMatrix();
    for (int b=0;b<4;b++){ glPushMatrix(); glTranslatef(-0.9f+b*0.62f,1.10f,0); glColor3f(0.45f,0.45f,0.46f); drawCylinder(0.02f,0.015f,8); glPopMatrix(); }
    {
        float lx[3] = { 0.35f, 1.02f, 0.55f };
        float lx2[3] = { 0.35f, 1.02f, -0.55f };
        float lx3[3] = { -0.55f, 1.02f, 0.35f };
        float lx4[3] = { -0.55f, 1.02f, -0.35f };
        float* lpos[4] = { lx, lx2, lx3, lx4 };
        for (int i = 0; i < 4; i++) {
            glPushMatrix();
            glTranslatef(lpos[i][0], lpos[i][1], lpos[i][2]);
            glColor3f(0.38f, 0.38f, 0.39f);
            drawCylinder(0.085f, 0.045f, 14);
            glTranslatef(0, -0.045f, 0);
            glDisable(GL_LIGHTING);
            glColor3f(1.0f, 0.90f, 0.60f);
            drawSphere(0.068f, 12, 10);
            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glColor4f(1.0f, 0.84f, 0.42f, 0.22f);
            drawSphere(0.15f, 12, 10);
            glColor4f(1.0f, 0.80f, 0.35f, 0.08f);
            drawSphere(0.22f, 12, 10);
            glDisable(GL_BLEND);
            glEnable(GL_LIGHTING);
            glPopMatrix();
        }
    }

    // Back wall - riveted
    glPushMatrix();
    glTranslatef(-1.55f, 0.18f, 0);
    glColor3f(0.24f, 0.27f, 0.30f);
    glScalef(0.06f, 2.05f, 2.6f);
    drawCube(1.0f);
    glPopMatrix();
    for (int r = 0; r < 18; r++) {
        float ry = -0.7f + r * 0.09f;
        for (int k = 0; k < 2; k++) {
            float rz = k == 0 ? 1.2f : -1.2f;
            glPushMatrix(); glTranslatef(-1.51f, ry, rz); glColor3f(0.14f, 0.15f, 0.17f); drawSphere(0.013f, 4, 3); glPopMatrix();
            glPushMatrix(); glTranslatef(-1.51f, ry, 0); glColor3f(0.14f, 0.15f, 0.17f); drawSphere(0.011f, 4, 3); glPopMatrix();
        }
    }
    }

    auto drawRivetRow = [](float x, float y0, float y1, float z, int n) {
        for (int i = 0; i < n; i++) { float y = y0 + (y1 - y0) * i / (n - 1); glPushMatrix(); glTranslatef(x, y, z); glColor3f(0.11f, 0.12f, 0.14f); drawSphere(0.012f, 4, 3); glPopMatrix(); }
    };

    // Front dashboard console - brushed steel, beveled top, riveted
    glPushMatrix();
    glTranslatef(1.18f, -0.48f, 0);
    glColor3f(0.30f, 0.32f, 0.35f);
    glScalef(0.42f, 0.68f, 2.35f);
    drawCube(1.0f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(1.12f, -0.14f, 0);
    glColor3f(0.42f, 0.44f, 0.46f);
    glScalef(0.36f, 0.015f, 2.30f);
    drawCube(1.0f);
    glPopMatrix();
    glPushMatrix(); glTranslatef(1.12f, -0.16f, 0); glColor3f(0.22f,0.23f,0.25f); glScalef(0.36f,0.005f,2.30f); drawCube(1.0f); glPopMatrix();
    for (int i = 0; i < 18; i++) { float z = -1.08f + i * 0.126f; drawRivetRow(1.28f, -0.75f, -0.12f, z, 7); }
    glPushMatrix(); glTranslatef(0.85f, -0.18f, 0.0f); glColor3f(0.72f,0.18f,0.18f); glScalef(0.32f,0.018f,0.018f); drawCube(1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.85f, -0.18f, 0.0f); glColor3f(0.85f,0.82f,0.70f); glScalef(0.06f,0.04f,0.02f); drawCube(1.0f); glPopMatrix();

    // Two warm console spots + soft bloom (reference amber)
    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix();
        glTranslatef(1.05f, -0.22f, s * 0.92f);
        glColor3f(0.42f, 0.42f, 0.43f);
        drawCylinder(0.06f, 0.04f, 10);
        glTranslatef(0.02f, 0.06f, 0);
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 0.91f, 0.62f);
        drawSphere(0.058f, 10, 8);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(1.0f, 0.82f, 0.38f, 0.24f);
        drawSphere(0.12f, 10, 8);
        glColor4f(1.0f, 0.78f, 0.30f, 0.08f);
        drawSphere(0.18f, 10, 8);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
        glPopMatrix();
    }

    // Analog gauges cluster - brass bezels, glass reflections, cream faces
    for (int g = 0; g < 3; g++) {
        float gz = 0.55f + g * 0.32f;
        glPushMatrix();
        glTranslatef(1.02f, -0.32f, gz);
        glRotatef(90, 0, 1, 0);
        glColor3f(0.32f, 0.31f, 0.28f);
        drawCylinder(0.115f, 0.020f, 18);
        glColor3f(0.14f, 0.14f, 0.15f);
        drawCylinder(0.112f, 0.016f, 18);
        glTranslatef(0, 0, 0.017f);
        glColor3f(0.96f, 0.95f, 0.89f);
        drawDisk(0.0f, 0.105f, 18);
        glColor3f(0.20f, 0.20f, 0.21f);
        for (int t = 0; t < 12; t++) { glPushMatrix(); glRotatef(t * 30, 0, 0, 1); glTranslatef(0.087f, 0, 0.001f); glScalef(0.013f, 0.018f, 0.004f); drawCube(1.0f); glPopMatrix(); }
        glColor3f(0.80f, 0.14f, 0.14f);
        glRotatef(-35 + g * 18 + sin(introTimer * 0.0015f + g) * 8, 0, 0, 1);
        glTranslatef(0, 0, 0.003f);
        glScalef(0.068f, 0.009f, 0.004f);
        drawCube(1.0f);
        glPopMatrix();
        glPushMatrix(); glTranslatef(1.02f, -0.32f, gz); glRotatef(90, 0, 1, 0); glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glColor4f(1.0f,1.0f,1.0f,0.18f); drawDisk(0.03f,0.05f,10); glDisable(GL_BLEND); glEnable(GL_LIGHTING); glPopMatrix();
    }

    // Side consoles
    for (int side = 0; side < 2; side++) {
        float sz = side == 0 ? 1.02f : -1.02f;
        glPushMatrix();
        glTranslatef(0.05f, -0.10f, sz);
        glColor3f(0.22f, 0.24f, 0.27f);
        glScalef(2.3f, 0.55f, 0.08f);
        drawCube(1.0f);
        glPopMatrix();
        for (int i = 0; i < 10; i++) { float x = -0.95f + i * 0.20f; glPushMatrix(); glTranslatef(x, 0.18f, sz); glColor3f(0.13f, 0.13f, 0.14f); drawSphere(0.010f, 4, 3); glPopMatrix(); }
    }

    // Panoramic window frames - reference: large front + angled side + top small
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    auto frameColor = [](){ glColor3f(0.26f, 0.31f, 0.36f); };
    auto glass = [](){ glColor4f(0.62f, 0.78f, 0.88f, 0.08f); };
    glEnable(GL_LIGHTING);

    // Draw crew - muted, less cartoon (desaturated uniforms)
    for (size_t i = 0; i < crew.size(); i++) {
        CrewMember& c = crew[i];
        glPushMatrix();
        glTranslatef(c.x, c.y, c.z);
        glColor3f(0.22f, 0.26f, 0.31f);
        glScalef(0.14f, 0.24f, 0.09f);
        drawCube(1.0f);
        glPopMatrix();
        // Head
        glPushMatrix();
        float headBob = sin(introTimer * 0.002f + i * 2) * 0.015f;
        glTranslatef(c.x, c.y + 0.33f + headBob, c.z);
        glColor3f(0.82f, 0.70f, 0.60f);
        drawSphere(0.085f, 10, 8);
        glColor3f(0.16f, 0.18f, 0.24f);
        glTranslatef(0, 0.055f, 0);
        drawCylinder(0.085f, 0.03f, 10);
        glPopMatrix();
    }

    // Two pilot seats like reference (black with headrests) + side crew benches
    for(int seat=-1; seat<=1; seat+=2){
        glPushMatrix(); glTranslatef(0.25f, -0.35f, seat*0.62f);
        glColor3f(0.10f,0.10f,0.11f); glScalef(0.42f,0.05f,0.42f); drawCube(1.0f);
        glTranslatef(0,0.28f, -0.12f); glScalef(1.0f,1.9f,0.22f); glColor3f(0.14f,0.14f,0.15f); drawCube(1.0f);
        glTranslatef(0,0.32f,0); glColor3f(0.08f,0.08f,0.09f); glScalef(1.0f,0.5f,1.0f); drawCube(1.0f);
        glPopMatrix();
    }
    if (cameraMode != CAM_CONTROL_SCREEN) {
    for(size_t i=0;i<crew.size() && i<3;i++){
        glPushMatrix(); glTranslatef(crew[i].x-0.16f, crew[i].y-0.22f, crew[i].z);
        glColor3f(0.26f,0.27f,0.29f); glScalef(0.18f,0.04f,0.16f); drawCube(1.0f);
        glColor3f(0.20f,0.21f,0.23f); glTranslatef( -0.08f, -0.14f, 0); glScalef(1.0f,1.8f,0.08f); drawCube(1.0f);
        glPopMatrix();
    }
    }
    glPushMatrix(); glTranslatef(-0.15f, -0.08f, 0.52f); glColor3f(0.32f,0.31f,0.29f); glScalef(0.85f,0.02f,0.42f); drawCube(1.0f); glPopMatrix();
    glDisable(GL_LIGHTING);
    glPushMatrix(); glTranslatef(-0.15f, -0.065f, 0.52f); glColor3f(0.92f,0.92f,0.88f); glScalef(0.70f,0.001f,0.32f); drawCube(1.0f); glPopMatrix();
    for(int l=0;l<4;l++){ glPushMatrix(); glTranslatef(-0.28f + l*0.11f, -0.064f, 0.52f); glColor3f(0.20f,0.22f,0.28f); glScalef(0.06f,0.002f,0.28f); drawCube(1.0f); glPopMatrix(); }
    glPushMatrix(); glTranslatef(0.05f, -0.065f, -0.48f); glColor3f(0.88f,0.85f,0.78f); glScalef(0.45f,0.001f,0.32f); drawCube(1.0f); glPopMatrix();
    for(int s=-1;s<=1;s+=2){ for(int k=0;k<2;k++){ glPushMatrix(); glTranslatef(-0.95f + k*0.55f, 0.15f, s*1.18f); glColor3f(0.58f,0.58f,0.59f); drawCylinder(0.08f,0.95f,10); glColor3f(0.32f,0.32f,0.33f); glTranslatef(0,0.48f,0); drawCylinder(0.085f,0.04f,10); glTranslatef(0,-0.96f,0); drawCylinder(0.085f,0.04f,10); glPopMatrix(); } }
    glPushMatrix(); glTranslatef(0.45f, 1.05f, 0.0f); glColor3f(0.28f,0.28f,0.30f); drawCylinder(0.06f,0.35f,8); glTranslatef(0,-0.18f,0); glColor3f(0.22f,0.22f,0.23f); drawCylinder(0.14f,0.08f,12); glDisable(GL_LIGHTING); glColor3f(1.0f,0.92f,0.70f); drawSphere(0.10f,10,8); glEnable(GL_LIGHTING); glPopMatrix();
    for(int w=0;w<4;w++){ glPushMatrix(); glTranslatef(-0.55f + w*0.38f, 0.18f, 0.98f); glColor3f(0.12f,0.12f,0.14f); glScalef(0.32f,0.18f,0.02f); drawCube(1.0f); glTranslatef(0,0.02f,0.015f); glDisable(GL_LIGHTING); glColor3f(0.15f,0.45f,0.85f); glScalef(0.88f,0.78f,0.01f); drawCube(1.0f); glEnable(GL_LIGHTING); glPopMatrix(); }
    glPushMatrix(); glTranslatef(1.25f, 0.15f, 1.28f); glRotatef(90,0,1,0); glColor3f(0.22f,0.24f,0.26f); drawCylinder(0.42f,0.04f,18); glColor3f(0.15f,0.35f,0.75f); drawDisk(0.08f,0.38f,18); glColor3f(0.30f,0.32f,0.34f); glTranslatef(0,0,0.02f); for(int s=0;s<6;s++){ glPushMatrix(); glRotatef(s*60,0,0,1); glTranslatef(0.30f,0,0); glScalef(0.06f,0.01f,0.01f); drawCube(1.0f); glPopMatrix(); } glPopMatrix();
    glEnable(GL_LIGHTING);
    // Panoramic windows - large riveted frames, faint glass so outside ocean/fish/manta visible
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    auto drawWindow = [&](float cx, float cy, float cz, float w, float h, float yaw) {
        glPushMatrix();
        glTranslatef(cx, cy, cz);
        glRotatef(yaw, 0, 1, 0);
        // frame
        glColor3f(0.30f, 0.34f, 0.38f);
        glPushMatrix(); glTranslatef(0.02f, h/2+0.025f, 0); glScalef(0.05f, 0.05f, w+0.08f); drawCube(1.0f); glPopMatrix();
        glPushMatrix(); glTranslatef(0.02f, -h/2-0.025f, 0); glScalef(0.05f, 0.05f, w+0.08f); drawCube(1.0f); glPopMatrix();
        glPushMatrix(); glTranslatef(0.02f, 0, w/2+0.025f); glScalef(0.05f, h+0.06f, 0.05f); drawCube(1.0f); glPopMatrix();
        glPushMatrix(); glTranslatef(0.02f, 0, -w/2-0.025f); glScalef(0.05f, h+0.06f, 0.05f); drawCube(1.0f); glPopMatrix();
        for (int k = 0; k < 14; k++) { float zz = -w/2 + w * k / 13.0f; glPushMatrix(); glTranslatef(0.035f, h/2+0.025f, zz); glColor3f(0.14f,0.15f,0.16f); drawSphere(0.011f,4,3); glPopMatrix(); glPushMatrix(); glTranslatef(0.035f, -h/2-0.025f, zz); drawSphere(0.011f,4,3); glPopMatrix(); glColor3f(0.30f,0.34f,0.38f); }
        glColor4f(0.72f, 0.84f, 0.92f, 0.10f);
        glPushMatrix(); glTranslatef(0.015f, 0, 0); glScalef(0.01f, h, w); drawCube(1.0f); glPopMatrix();
        glColor4f(1.0f,1.0f,1.0f,0.07f);
        glPushMatrix(); glTranslatef(0.016f, h*0.35f, 0); glScalef(0.012f, h*0.15f, w*0.92f); drawCube(1.0f); glPopMatrix();
        glPopMatrix();
    };
    // Front center large (like reference center)
    drawWindow(1.32f, 0.08f, 0.0f, 1.05f, 0.68f, 0);
    // Front upper small
    drawWindow(1.32f, 0.62f, 0.0f, 1.05f, 0.18f, 0);
    // Front-left angled
    drawWindow(1.18f, 0.08f, 0.78f, 0.85f, 0.68f, -32);
    // Front-right angled
    drawWindow(1.18f, 0.08f, -0.78f, 0.85f, 0.68f, 32);
    // Side large windows (right side shows manta like reference)
    drawWindow(0.35f, 0.08f, 1.28f, 1.22f, 0.70f, 90);
    drawWindow(0.35f, 0.08f, -1.28f, 1.22f, 0.70f, 90);
    // Top narrow side windows
    drawWindow(0.35f, 0.63f, 1.28f, 1.22f, 0.18f, 90);
    drawWindow(0.35f, 0.63f, -1.28f, 1.22f, 0.18f, 90);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
}

// ============================================================
// DRAW CREW CABIN (distinct quarters behind the bridge)
// ============================================================
void drawCrewCabin() {
    // matte interior metal: zero specular so the lamps don't paint soft
    // white smear lobes across the curved ceiling (room stays lit by diffuse)
    GLfloat matSpec[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat matShine[] = { 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShine);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Room bounds in local (pre-scale) units. The scene is later scaled by
    // INT_SX/INT_SY/INT_SZ inside display(), so 1 local metER ~ 1.7/1.3/1.5 world.
    const float CR_AY = 1.45f;   // vertical hull semi-axis
    const float CR_BZ = 1.44f;   // horizontal hull semi-axis
    const float CR_CY = 0.22f;   // hull vertical centre
    const float CR_AX = 1.75f;   // hull longitudinal half extents
    const float CR_FY = -0.52f;  // deck top
    const float CR_XF = 1.55f;   // forward bulkhead
    const float CR_XA = -1.55f;  // aft bulkhead
    const int   CR_SEG = 30;     // angular tessellation of the tube

    auto hullYZ = [&](float t, float &y, float &z) {
        y = CR_CY + CR_AY * sin(t);
        z = CR_BZ * cos(t);
    };
    auto hullNormal = [&](float y, float z, float &ny, float &nz) {
        float ey = (y - CR_CY) / (CR_AY * CR_AY);
        float ez = z / (CR_BZ * CR_BZ);
        float L = sqrt(ey * ey + ez * ez) + 1e-6f;
        ny = -ey / L;   // inward
        nz = -ez / L;
    };

    // ------------------------------------------------------------------
    // 1. CURVED HULL SKIN - continuous elliptical metal tube with round
    //    porthole openings cut out. Visible from inside; closes off the
    //    ocean everywhere except through the porthole glass.
    // ------------------------------------------------------------------
    const float xStride = (2.0f * CR_AX) / 25.0f;
    const float tStep = (2.0f * (float)PI) / CR_SEG;
    const float portholeX[3] = { -0.75f, 0.0f, 0.75f };
    const float portholeY = 0.50f;
    const float holeR = 0.135f;

    glBegin(GL_QUADS);
    for (int ix = 0; ix < 25; ix++) {
        float x0 = -CR_AX + ix * xStride;
        float x1 = x0 + xStride;
        for (int it = 0; it < CR_SEG; it++) {
            float t0 = it * tStep;
            float t1 = t0 + tStep;
            float yA, zA, yB, zB, yC, zC, yD, zD;
            hullYZ(t0, yA, zA);     // (x0, t0)
            hullYZ(t1, yB, zB);     // (x0, t1)
            hullYZ(t1, yC, zC);     // (x1, t1)
            hullYZ(t0, yD, zD);     // (x1, t0)
            float yMid, zMid;
            hullYZ(0.5f * (t0 + t1), yMid, zMid);

            // nose region ahead of the forward bulkhead -> re-drawn as glass below
            float xMid = 0.5f * (x0 + x1);
            if (xMid > CR_XF - 0.03f) continue;

            // cut out round portholes in the side walls (|t| near 0 or PI)
            bool skip = false;
            if (fabs(zMid) > 0.5f) {
                for (int p = 0; p < 3 && !skip; p++) {
                    float dx = xMid - portholeX[p];
                    float dy = yMid - portholeY;
                    if (dx * dx + dy * dy < holeR * holeR) skip = true;
                }
            }
            if (skip) continue;

            float midRatio = (yMid - CR_CY) / CR_AY;
            float lum = 0.86f + 0.35f * midRatio;
            int hash = (int)(hashNoise(ix * 3 + it, ix + it * 5, 7) & 3);
            lum += ((float)hash - 1.5f) * 0.03f;
            if (lum < 0.5f) lum = 0.5f;
            if (lum > 1.25f) lum = 1.25f;
            float cR = 0.32f * lum, cG = 0.34f * lum, cB = 0.37f * lum;
            glColor3f(cR, cG, cB);

            float nAy, nAz, nBy, nBz, nCy, nCz, nDy, nDz;
            hullNormal(yA, zA, nAy, nAz);
            hullNormal(yB, zB, nBy, nBz);
            hullNormal(yC, zC, nCy, nCz);
            hullNormal(yD, zD, nDy, nDz);
            glNormal3f(0.0f, nAy, nAz); glVertex3f(x0, yA, zA);
            glNormal3f(0.0f, nBy, nBz); glVertex3f(x0, yB, zB);
            glNormal3f(0.0f, nCy, nCz); glVertex3f(x1, yC, zC);
            glNormal3f(0.0f, nDy, nDz); glVertex3f(x1, yD, zD);
        }
    }
    glEnd();

    // Nose viewing glass - the band ahead of the forward bulkhead becomes a
    // bow window (the underwater world is already painted by display()). A
    // solid ash-gray metal brow covers the whole upper sector above eye
    // level so the bright ocean surface / sky never washes across the top
    // of the room - glass stays only down in the lower half toward the sea.
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
    int ixNose = (int)((CR_XF - 0.03f + CR_AX) / xStride);
    for (int ix = ixNose; ix < 25; ix++) {
        float x0 = -CR_AX + ix * xStride;
        float x1 = x0 + xStride;
        for (int it = 0; it < CR_SEG; it++) {
            float t0 = it * tStep;
            float t1 = t0 + tStep;
            float yA, zA, yB, zB, yC, zC, yD, zD;
            hullYZ(t0, yA, zA);
            hullYZ(t1, yB, zB);
            hullYZ(t1, yC, zC);
            hullYZ(t0, yD, zD);
            float nAy, nAz, nBy, nBz, nCy, nCz, nDy, nDz;
            hullNormal(yA, zA, nAy, nAz);
            hullNormal(yB, zB, nBy, nBz);
            hullNormal(yC, zC, nCy, nCz);
            hullNormal(yD, zD, nDy, nDz);
            float tMid = 0.5f * (t0 + t1);
            if (sin(tMid) > 0.10f) {
                // solid ash-gray metal roof everywhere above eye level, so
                // the curved interior hull closes off the top of the room
                glColor4f(0.30f, 0.32f, 0.35f, 1.0f);
            } else {
                glColor4f(0.22f, 0.50f, 0.70f, 0.07f);
            }
            glNormal3f(0.0f, nAy, nAz); glVertex3f(x0, yA, zA);
            glNormal3f(0.0f, nBy, nBz); glVertex3f(x0, yB, zB);
            glNormal3f(0.0f, nCy, nCz); glVertex3f(x1, yC, zC);
            glNormal3f(0.0f, nDy, nDz); glVertex3f(x1, yD, zD);
        }
    }
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

    // Rib seams (circumferential) + weld meridians - crisp dark lines
    glDisable(GL_LIGHTING);
    glLineWidth(1.6f);
    glColor3f(0.10f, 0.11f, 0.13f);
    for (int ix = 1; ix < 24; ix += 3) {
        float x = -CR_AX + ix * xStride;
        if (ix % 3 != 1) continue;
        glBegin(GL_LINE_LOOP);
        for (int it = 0; it < CR_SEG; it++) {
            float t = (float)it * tStep;
            float y, z, ny, nz;
            hullYZ(t, y, z);
            hullNormal(y, z, ny, nz);
            glVertex3f(x, y - ny * 0.005f, z - nz * 0.005f);
        }
        glEnd();
    }
    for (int m = 0; m < 4; m++) {
        float t = (m == 0) ? (0.5f * PI) : (m == 1) ? (-0.5f * PI) : (m == 2) ? 0.0f : PI;
        glBegin(GL_LINE_STRIP);
        for (int ix = 0; ix < 26; ix++) {
            float x = -CR_AX + ix * xStride;
            float y, z, ny, nz;
            hullYZ(t, y, z);
            hullNormal(y, z, ny, nz);
            glVertex3f(x, y - ny * 0.005f, z - nz * 0.005f);
        }
        glEnd();
    }
    glEnable(GL_LIGHTING);

    // Closed end caps so the tube is sealed in every direction
    auto capFan = [&](float capX, float dirX, bool glass, float solidSin) {
        glDisable(GL_LIGHTING);
        glPushMatrix();
        if (glass) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(0.22f, 0.50f, 0.70f, 0.07f);
        } else {
            glColor3f(0.14f, 0.15f, 0.16f);
        }
        glNormal3f(dirX, 0.0f, 0.0f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(capX, CR_CY, 0.0f);
        for (int it = 0; it <= CR_SEG; it++) {
            float t = (float)it * tStep;
            float y, z;
            hullYZ(t, y, z);
            if (glass && sin(t) > solidSin) {
                // upper sectors of the bow dome use the same ash-gray metal
                // brow, so no bright water or sky washes over the room top
                glColor4f(0.30f, 0.32f, 0.35f, 1.0f);
            }
            glVertex3f(capX, y, z);
        }
        glEnd();
        if (glass) glDisable(GL_BLEND);
        glPopMatrix();
        glEnable(GL_LIGHTING);
    };
    capFan(-CR_AX,  1.0f, false, 0.10f);
    capFan( CR_AX, -1.0f, true,  0.10f);

    // ------------------------------------------------------------------
    // 2. DECK - solid metal platform, walking plates, centre aisle stripe
    // ------------------------------------------------------------------
    glPushMatrix();
    glTranslatef(0.0f, CR_FY - 0.10f, 0.0f);
    glColor3f(0.12f, 0.13f, 0.13f);
    glScalef(3.50f, 0.20f, 2.46f);
    drawCube(1.0f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0f, CR_FY, 0.0f);
    glColor3f(0.22f, 0.24f, 0.23f);
    glScalef(3.30f, 0.03f, 2.42f);
    drawCube(1.0f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0f, CR_FY + 0.012f, 0.0f);
    glColor3f(0.30f, 0.32f, 0.30f);
    glScalef(3.00f, 0.012f, 0.64f);
    drawCube(1.0f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0f, CR_FY + 0.016f, 0.0f);
    glColor3f(0.45f, 0.43f, 0.20f);
    glScalef(2.75f, 0.006f, 0.09f);
    drawCube(1.0f);
    glPopMatrix();
    for (int j = 0; j < 14; j++) {
        glPushMatrix();
        glTranslatef(-1.30f + j * 0.20f, CR_FY + 0.017f, 0.0f);
        glColor3f(0.12f, 0.13f, 0.12f);
        glScalef(0.015f, 0.008f, 0.62f);
        drawCube(1.0f);
        glPopMatrix();
    }
    for (int s = -1; s <= 1; s += 2) {
        for (int zz = 0; zz < 2; zz++) {
            glPushMatrix();
            glTranslatef(0.0f, CR_FY + 0.017f, s * (0.42f + zz * 0.43f));
            glColor3f(0.13f, 0.14f, 0.13f);
            glScalef(3.10f, 0.008f, 0.014f);
            drawCube(1.0f);
            glPopMatrix();
        }
        glPushMatrix();
        glTranslatef(0.0f, CR_FY + 0.014f, s * 1.14f);
        glColor3f(0.10f, 0.11f, 0.11f);
        glScalef(3.10f, 0.02f, 0.06f);
        drawCube(1.0f);
        glPopMatrix();
    }

    // ------------------------------------------------------------------
    // 3. PORTHOLES - six small circular metal-rimmed ocean windows
    // ------------------------------------------------------------------
    auto porthole = [&](float px, float py, float pz, float side) {
        glPushMatrix();
        glTranslatef(px, py, pz);
        if (side < 0.0f) glRotatef(180.0f, 0, 1, 0);
        // Solid metal mount collar: the hull cut-out is a coarse quad grid, so
        // its jagged rim can reach ~0.31 from the window centre. This plate
        // covers the whole rough opening and makes the wall one continuous
        // ash-gray metal surface - the ocean can now only show through the
        // circular glass pane inside the rim, never through broken edges.
        glColor3f(0.31f, 0.33f, 0.36f);
        drawDisk(0.075f, 0.34f, 36);
        // outer rim ring + glass seat
        glColor3f(0.26f, 0.28f, 0.31f);
        drawTorus(0.070f, 0.118f, 10, 20);
        glColor3f(0.30f, 0.33f, 0.36f);
        drawDisk(0.02f, 0.075f, 20);
        // translucent ocean glass
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_LIGHTING);
        glColor4f(0.30f, 0.66f, 0.86f, 0.45f);
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.010f);
        drawDisk(0.0f, 0.075f, 20);
        glPopMatrix();
        glColor4f(0.85f, 0.95f, 1.0f, 0.22f);
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.016f);
        drawDisk(0.0f, 0.035f, 16);
        glPopMatrix();
        glEnable(GL_LIGHTING);
        glDisable(GL_BLEND);
        // clamping bolts
        for (int b = 0; b < 8; b++) {
            float a = (float)b * 0.25f * PI + 0.125f * PI;
            glPushMatrix();
            glTranslatef(0.090f * cos(a), 0.090f * sin(a), 0.0f);
            glColor3f(0.13f, 0.14f, 0.15f);
            drawSphere(0.009f, 4, 3);
            glPopMatrix();
        }
        glPopMatrix();
    };
    porthole(-0.75f, 0.50f,  1.40f,  1.0f);
    porthole( 0.00f, 0.50f,  1.40f,  1.0f);
    porthole( 0.75f, 0.50f,  1.40f,  1.0f);
    porthole(-0.75f, 0.50f, -1.40f, -1.0f);
    porthole( 0.00f, 0.50f, -1.40f, -1.0f);
    porthole( 0.75f, 0.50f, -1.40f, -1.0f);

    // ------------------------------------------------------------------
    // 4. OVERHEAD - light housing, ducts, pipes, cable trays, valves
    // ------------------------------------------------------------------
    glPushMatrix();
    glTranslatef(0.0f, 0.80f, 0.0f);
    glColor3f(0.24f, 0.25f, 0.27f);
    glScalef(2.70f, 0.06f, 0.20f);
    drawCube(1.0f);
    glPopMatrix();
    // Overhead lighting is matte and low-glare: a recessed ash-gray strip
    // with plain lamp caps. No additive blending, so no white blur lobes
    // wash across the curved ceiling at the top of the room.
    glPushMatrix();
    glTranslatef(0.0f, 0.775f, 0.0f);
    glColor3f(0.46f, 0.47f, 0.49f);
    glScalef(2.40f, 0.010f, 0.13f);
    drawCube(1.0f);
    glPopMatrix();
    for (int h = 0; h < 5; h++) {
        glPushMatrix();
        glColor3f(0.42f, 0.43f, 0.45f);
        glTranslatef(-1.20f + h * 0.60f, 0.79f, 0.0f);
        drawSphere(0.05f, 8, 6);
        glPopMatrix();
    }

    for (int s = -1; s <= 1; s += 2) {
        // ventilation ducts
        glPushMatrix();
        glTranslatef(0.0f, 0.62f, s * 0.55f);
        glColor3f(0.34f, 0.36f, 0.39f);
        glScalef(2.30f, 0.06f, 0.14f);
        drawCube(1.0f);
        glPopMatrix();
        for (int g = 0; g < 9; g++) {
            glPushMatrix();
            glTranslatef(-1.05f + g * 0.26f, 0.585f, s * 0.55f);
            glColor3f(0.12f, 0.13f, 0.14f);
            glScalef(0.18f, 0.012f, 0.03f);
            drawCube(1.0f);
            glPopMatrix();
        }
        // pipe runs along the curved hull
        glPushMatrix();
        glTranslatef(0.0f, 0.74f, s * 0.82f);
        glColor3f(0.30f, 0.32f, 0.35f);
        glScalef(2.90f, 0.045f, 0.045f);
        drawCube(1.0f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0.0f, 0.66f, s * 0.90f);
        glColor3f(0.20f, 0.21f, 0.23f);
        glScalef(2.90f, 0.028f, 0.028f);
        drawCube(1.0f);
        glPopMatrix();
        // cable tray on the opposite side
        glPushMatrix();
        glTranslatef(0.0f, 0.74f, -s * 0.66f);
        glColor3f(0.16f, 0.17f, 0.19f);
        glScalef(2.70f, 0.03f, 0.06f);
        drawCube(1.0f);
        glPopMatrix();
    }

    auto valve = [&](float vx, float vy, float vz) {
        glPushMatrix();
        glTranslatef(vx, vy, vz);
        glColor3f(0.34f, 0.36f, 0.39f);
        glPushMatrix();
        glRotatef(90, 0, 1, 0);
        drawCylinder(0.05f, 0.16f, 10);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0.0f, 0.10f, 0.0f);
        glRotatef(-90, 1, 0, 0);
        glColor3f(0.20f, 0.12f, 0.08f);
        drawTorus(0.05f, 0.090f, 8, 14);
        for (int spv = 0; spv < 3; spv++) {
            glPushMatrix();
            glColor3f(0.25f, 0.16f, 0.10f);
            glRotatef((float)spv * 120.0f, 0, 1, 0);
            glTranslatef(0.050f, 0.0f, 0.0f);
            glScalef(0.10f, 0.016f, 0.016f);
            drawCube(1.0f);
            glPopMatrix();
        }
        glPopMatrix();
        glPopMatrix();
    };
    valve(-0.42f, 0.74f, -0.82f);
    valve( 0.30f, 0.74f,  0.82f);

    // ------------------------------------------------------------------
    // 5. FORWARD BULKHEAD + BIG CONTROL SCREEN + SIDE DISPLAYS
    // ------------------------------------------------------------------
    // Lower part of the forward bulkhead stays solid metal; above it the hull
    // nose band + cap (already drawn as glass) become the 'front view'.
    glPushMatrix();
    glTranslatef(CR_XF, -0.185f, 0.0f);
    glColor3f(0.36f, 0.40f, 0.44f);
    glScalef(0.14f, 0.68f, 2.46f);
    drawCube(1.0f);
    glPopMatrix();
    for (int rv = 0; rv < 3; rv++) {
        for (int rz = -1; rz <= 1; rz += 2) {
            glPushMatrix();
            glTranslatef(CR_XF - 0.035f, -0.34f + rv * 0.18f, rz * 1.10f);
            glColor3f(0.12f, 0.13f, 0.15f);
            drawSphere(0.011f, 4, 3);
            glPopMatrix();
        }
    }
    // top header beam spanning the observation glass
    glPushMatrix();
    glTranslatef(CR_XF - 0.05f, 1.12f, 0.0f);
    glColor3f(0.30f, 0.34f, 0.38f);
    glScalef(0.14f, 0.06f, 1.90f);
    drawCube(1.0f);
    glPopMatrix();
    // side status displays flanking the main screen
    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix();
        glColor3f(0.10f, 0.11f, 0.13f);
        glTranslatef(CR_XF - 0.06f, 0.34f, s * 1.04f);
        glScalef(0.02f, 0.42f, 0.30f);
        drawCube(1.0f);
        glPopMatrix();
        glDisable(GL_LIGHTING);
        glPushMatrix();
        glColor3f(0.14f, 0.50f, 0.36f);
        glTranslatef(CR_XF - 0.05f, 0.34f, s * 1.04f);
        glScalef(0.01f, 0.38f, 0.26f);
        drawCube(1.0f);
        glPopMatrix();
        glEnable(GL_LIGHTING);
    }
    // main front-view screen: slim metal bezel frame around the bow window
    for (int bd = 0; bd < 4; bd++) {
        glPushMatrix();
        if (bd == 0) { glTranslatef(CR_XF - 0.13f, 0.93f, 0.0f);   glScalef(0.06f, 0.10f, 1.90f); }
        if (bd == 1) { glTranslatef(CR_XF - 0.13f, 0.135f, 0.0f);  glScalef(0.06f, 0.13f, 1.90f); }
        if (bd == 2) { glTranslatef(CR_XF - 0.13f, 0.54f, -0.90f); glScalef(0.06f, 0.68f, 0.12f); }
        if (bd == 3) { glTranslatef(CR_XF - 0.13f, 0.54f,  0.90f); glScalef(0.06f, 0.68f, 0.12f); }
        glColor3f(0.10f, 0.12f, 0.14f);
        drawCube(1.0f);
        glPopMatrix();
    }
    // big bow window: fully open glass, no veils or glows - the ocean stays
    // crisp. Only the thin structural nose glass tints the view slightly.
    // EXTERNAL CAMERA - LIVE caption banner under the feed
    glDisable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(CR_XF - 0.16f, 0.235f, 0.0f);
    glColor3f(0.0f, 0.02f, 0.05f);
    glScalef(0.02f, 0.055f, 0.95f);
    drawCube(1.0f);
    glPopMatrix();
    glColor3f(0.35f, 0.95f, 1.0f);
    drawText3D(CR_XF - 0.175f, 0.225f, -0.30f, "EXT CAM - LIVE", GLUT_BITMAP_HELVETICA_12);
    glPushMatrix();
    glTranslatef(CR_XF - 0.155f, 0.235f, 0.80f);
    glColor4f(1.0f, 0.18f, 0.18f, 0.85f);
    drawSphere(0.016f, 6, 5);
    glPopMatrix();
    glEnable(GL_LIGHTING);
    // red / green running lamps near the top corners
    for (int s = -1; s <= 1; s += 2) {
        glDisable(GL_LIGHTING);
        glPushMatrix();
        glColor3f(0.9f, 0.2f, 0.2f);
        glTranslatef(1.45f, 0.95f, s * 1.10f);
        drawSphere(0.018f, 6, 5);
        glPopMatrix();
        glEnable(GL_LIGHTING);
        glDisable(GL_LIGHTING);
        glPushMatrix();
        glColor3f(0.2f, 0.9f, 0.3f);
        glTranslatef(1.42f, 0.95f, s * 1.10f);
        drawSphere(0.018f, 6, 5);
        glPopMatrix();
        glEnable(GL_LIGHTING);
    }

    // ------------------------------------------------------------------
    // 6. CONSOLE BANK in front of the big screen + front operators
    // ------------------------------------------------------------------
    // small on-screen drawing helpers (flat strokes inside a pushed frame)
    auto drawLine3D = [&](float x1, float y1, float z1,
                          float x2, float y2, float z2) {
        glBegin(GL_LINES);
        glVertex3f(x1, y1, z1);
        glVertex3f(x2, y2, z2);
        glEnd();
    };
    auto drawRing3D = [&](float cx, float cy, float cz, float r, int segs) {
        float tp = (2.0f * (float)PI) / segs;
        glBegin(GL_LINE_LOOP);
        for (int k = 0; k < segs; k++) {
            float a = k * tp;
            glVertex3f(cx, cy + sin(a) * r, cz + cos(a) * r);
        }
        glEnd();
    };
    auto drawRingXY3D = [&](float cx, float cy, float cz, float r, int segs) {
        float tp = (2.0f * (float)PI) / segs;
        glBegin(GL_LINE_LOOP);
        for (int k = 0; k < segs; k++) {
            float a = k * tp;
            glVertex3f(cx + sin(a) * r, cy + cos(a) * r, cz);
        }
        glEnd();
    };
    auto scText = [&](float ly, float lz, const char* t, void* f) {
        drawText3D(0.011f, ly, lz, t, f);
    };
    // each operator PC gets a different submarine control station.
    // NOTE: bitmap fonts stay pixel-sized in 3D so they blow up on these
    // tiny monitors - the PC screens use pure vector graphics only.
    auto drawCrewScreenUI = [&](int thema) {
        const float ctx = -0.008f;   // just aft of the dark panel = viewer side
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        auto screenQuad = [&](float y1, float z1, float y2, float z2, float xo) {
            glBegin(GL_QUADS);
            glVertex3f(ctx + xo, y1, z1);
            glVertex3f(ctx + xo, y1, z2);
            glVertex3f(ctx + xo, y2, z2);
            glVertex3f(ctx + xo, y2, z1);
            glEnd();
        };
        // faint blue backlight so the monitor glows instead of pitch black
        glColor4f(0.03f, 0.11f, 0.20f, 1.0f);
        screenQuad(-0.12f, -0.115f, 0.12f, 0.115f, 0.001f);
        if (thema == 0) {                       // SONAR / radar scope
            float cy0 = -0.015f, cz0 = 0.0f;
            glColor3f(0.12f, 0.85f, 0.95f);
            drawRing3D(ctx, cy0, cz0, 0.085f, 24);
            drawRing3D(ctx, cy0, cz0, 0.055f, 24);
            drawLine3D(ctx, cy0 - 0.085f, cz0, ctx, cy0 + 0.085f, cz0);
            drawLine3D(ctx, cy0, cz0 - 0.085f, ctx, cy0, cz0 + 0.085f);
            // rotating sweep with a fading tail
            float sa = introTimer * 0.0014f;
            for (int i = 0; i < 26; i++) {
                float a = sa + i * 0.022f;
                glColor4f(0.10f, 0.70f, 0.90f, 0.30f - i * 0.009f);
                drawLine3D(ctx, cy0 + sin(a) * 0.055f, cz0 + cos(a) * 0.055f,
                               ctx, cy0 + sin(a) * 0.085f, cz0 + cos(a) * 0.085f);
            }
            // echoing sonar contacts
            glColor3f(0.95f, 0.40f, 0.16f);
            for (int b = 0; b < 3; b++) {
                float ba = b * 2.1f + 0.6f;
                float br = 0.048f + noise01(b, 5, 31) * 0.028f;
                float bl = 0.020f + noise01(b, 9, 33) * 0.018f;
                float pulse = 0.5f + 0.5f * sin(introTimer * 0.004f + b * 2.0f);
                float bqy = cy0 + sin(ba) * br, bqz = cz0 + cos(ba) * br;
                float bs = 0.006f + pulse * 0.007f;
                screenQuad(bqy - bs, bqz - bs, bqy + bs, bqz + bs, -0.001f);
                glBegin(GL_LINE_STRIP);
                for (int s2 = 0; s2 <= 10; s2++) {
                    float ph = introTimer * 0.003f + s2 * 0.628f;
                    float rr = br + sin(ph) * bl;
                    glVertex3f(ctx, cy0 + sin(ba) * rr, cz0 + cos(ba) * rr);
                }
                glEnd();
            }
            // filled sweep wedge so the scan reads at any distance
            float sa2 = sa;
            glColor4f(0.10f, 0.55f, 0.70f, 0.40f);
            glBegin(GL_TRIANGLES);
            glVertex3f(ctx - 0.0008f, cy0, cz0);
            glVertex3f(ctx - 0.0008f, cy0 + sin(sa2) * 0.085f, cz0 + cos(sa2) * 0.085f);
            glVertex3f(ctx - 0.0008f, cy0 + sin(sa2 + 0.4f) * 0.085f, cz0 + cos(sa2 + 0.4f) * 0.085f);
            glEnd();
            // bottom status bar
            glColor4f(0.15f, 0.80f, 0.45f, 0.9f);
            screenQuad(-0.115f, -0.115f, -0.095f, 0.02f, -0.001f);
            glColor3f(0.30f, 0.95f, 1.0f);
            // SONAR tag drawn as vector ticks (no bitmap text on tiny screens)
            drawLine3D(ctx, -0.115f, -0.09f, ctx, -0.060f, -0.09f);
            drawLine3D(ctx, -0.115f, -0.10f, ctx, -0.080f, -0.10f);
        } else if (thema == 1) {                // NAV / chart plotter
            glColor3f(0.25f, 0.80f, 0.55f);
            for (int gr = 0; gr <= 4; gr++) {
                float gz = -0.09f + gr * 0.045f;
                drawLine3D(ctx, -0.11f, gz, ctx, 0.11f, gz);
            }
            for (int gc = 0; gc <= 4; gc++) {
                float gy = -0.11f + gc * 0.055f;
                drawLine3D(ctx, gy, -0.09f, ctx, gy, 0.09f);
            }
            // own-ship marker with a heading line to the live yaw
            float sx = -0.02f + sin(introTimer * 0.0005f) * 0.02f;
            float sz = 0.02f + cos(introTimer * 0.0004f) * 0.02f;
            float hd = sub.yaw * 0.0174533f;
            glColor3f(0.20f, 0.95f, 0.45f);
            drawLine3D(ctx, sx, sz, ctx, sx + sin(hd) * 0.05f, sz + cos(hd) * 0.05f);
            drawLine3D(ctx, sx, sz - 0.028f, ctx, sx, sz - 0.012f);
            drawLine3D(ctx, sx, sz, ctx, sx + 0.028f, sz);
            drawLine3D(ctx, sx, sz, ctx, sx - 0.028f, sz);
            // waypoint boxes + filled cores
            glColor3f(0.95f, 0.45f, 0.20f);
            for (int wp = 0; wp < 3; wp++) {
                float wx = -0.05f + wp * 0.05f;
                float wz = -0.045f + (noise01(wp, 3, 41) - 0.5f) * 0.06f;
                screenQuad(wx - 0.008f, wz - 0.008f, wx + 0.008f, wz + 0.008f, -0.001f);
                glBegin(GL_LINE_LOOP);
                glVertex3f(ctx, wx - 0.012f, wz - 0.012f);
                glVertex3f(ctx, wx + 0.012f, wz - 0.012f);
                glVertex3f(ctx, wx + 0.012f, wz + 0.012f);
                glVertex3f(ctx, wx - 0.012f, wz + 0.012f);
                glEnd();
            }
            // filled own-ship marker + bottom status bar
            glColor3f(0.15f, 0.95f, 0.40f);
            screenQuad(sx - 0.011f, sz - 0.011f, sx + 0.011f, sz + 0.011f, -0.001f);
            screenQuad(-0.115f, -0.115f, -0.095f, 0.02f, -0.001f);
            glColor3f(0.25f, 0.95f, 0.85f);
            // NAV tag as vector ticks
            drawLine3D(ctx, -0.112f, -0.10f, ctx, -0.060f, -0.10f);
            glColor3f(0.20f, 0.95f, 0.45f);
            char mb[32];
            (void)mb;
        } else {                                // ENGINE / machinery
            glColor3f(0.30f, 0.95f, 1.0f);
            // ENG tag as vector ticks
            drawLine3D(ctx, -0.115f, -0.10f, ctx, -0.065f, -0.10f);
            for (int gi = 0; gi < 3; gi++) {
                float gx = -0.09f + gi * 0.065f;
                float gv;
                if (gi == 0) gv = 0.35f + 0.55f * fabs(sub.speed) / 3.0f;
                else if (gi == 1) gv = 0.40f + 0.5f * fmod(sin(introTimer * 0.0008f) + 1.0f, 1.0f);
                else gv = 0.25f + 0.65f * noise01((int)(introTimer * 0.05f), 17, 61);
                glColor3f(0.16f, 0.28f, 0.34f);
                drawLine3D(ctx, gx, -0.09f, ctx, gx, 0.09f);
                glColor3f(0.20f, 0.90f, 0.50f);
                drawLine3D(ctx, gx, -0.09f, ctx, gx, -0.09f + gv * 0.18f);
                // filled bar body so gauges read at distance
                glColor4f(0.15f, 0.75f, 0.40f, 0.9f);
                screenQuad(-0.09f, gx - 0.020f, -0.09f + gv * 0.18f, gx + 0.020f, -0.001f);
            }
            glColor3f(0.20f, 0.95f, 0.60f);
            // status tick
            drawLine3D(ctx, 0.000f, 0.10f, ctx, 0.045f, 0.10f);
            drawLine3D(ctx, -0.100f, -0.02f, ctx, -0.070f, -0.02f);
            drawLine3D(ctx, -0.070f, -0.02f, ctx, -0.105f, 0.035f);
            // top status block
            glColor4f(0.20f, 0.85f, 0.55f, 0.9f);
            screenQuad(0.085f, -0.115f, 0.110f, -0.03f, -0.001f);
        }
        glDisable(GL_BLEND);
    };
    auto frontConsole = [&](float compz, float compw, int thema) {
        glPushMatrix();
        glTranslatef(1.22f, -0.40f, compz);
        glColor3f(0.13f, 0.15f, 0.18f);
        glScalef(0.34f, 0.27f, compw);
        drawCube(1.0f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(1.24f, -0.13f, compz);
        glColor3f(0.22f, 0.26f, 0.30f);
        glScalef(0.46f, 0.035f, compw);
        drawCube(1.0f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(1.42f, -0.05f, compz);
        glRotatef(-12, 0, 0, 1);
        glColor3f(0.10f, 0.11f, 0.13f);
        glScalef(0.035f, 0.26f, compw * 0.80f);
        drawCube(1.0f);
        glPopMatrix();
        glDisable(GL_LIGHTING);
        glPushMatrix();
        glTranslatef(1.435f, -0.045f, compz);
        glRotatef(-12, 0, 0, 1);
        glColor3f(0.02f, 0.03f, 0.05f);
        glScalef(0.012f, 0.24f, compw * 0.72f);
        drawCube(1.0f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(1.3985f, -0.045f, compz);
        glRotatef(-12, 0, 0, 1);
        drawCrewScreenUI(thema);
        glPopMatrix();
        glEnable(GL_LIGHTING);
        for (int k = 0; k < 3; k++) {
            glDisable(GL_LIGHTING);
            glPushMatrix();
            if (k == 0) glColor3f(0.8f, 0.2f, 0.2f);
            else if (k == 1) glColor3f(0.9f, 0.8f, 0.2f);
            else glColor3f(0.2f, 0.9f, 0.3f);
            glTranslatef(1.20f - k * 0.14f, -0.10f, compz);
            drawSphere(0.012f, 6, 5);
            glPopMatrix();
            glEnable(GL_LIGHTING);
        }
    };
    frontConsole(-0.52f, 0.34f, 0);   // sonar station
    frontConsole( 0.00f, 0.34f, 1);   // navigation station
    frontConsole( 0.52f, 0.34f, 2);   // engine station

    auto chair = [&](float cx, float cz, float yaw) {
        glPushMatrix();
        glTranslatef(cx, -0.52f, cz);
        glRotatef(yaw, 0, 1, 0);
        for (int k = 0; k < 5; k++) {
            float a = (float)k * 1.256637f;
            glPushMatrix();
            glColor3f(0.10f, 0.11f, 0.12f);
            glTranslatef(cos(a) * 0.16f, 0.015f, sin(a) * 0.16f);
            drawSphere(0.025f, 6, 5);
            glPopMatrix();
            glPushMatrix();
            glColor3f(0.13f, 0.14f, 0.15f);
            glTranslatef(cos(a) * 0.08f, 0.03f, sin(a) * 0.08f);
            glRotatef(k * 72.0f, 0, 1, 0);
            glScalef(0.05f, 0.02f, 0.16f);
            drawCube(1.0f);
            glPopMatrix();
        }
        glPushMatrix();
        glColor3f(0.12f, 0.13f, 0.15f);
        glTranslatef(0.0f, 0.10f, 0.0f);
        glRotatef(-90, 1, 0, 0);
        drawCylinder(0.025f, 0.10f, 8);
        glPopMatrix();
        glPushMatrix();
        glColor3f(0.16f, 0.20f, 0.24f);
        glTranslatef(0.0f, 0.14f, 0.0f);
        glScalef(0.28f, 0.04f, 0.26f);
        drawCube(1.0f);
        glPopMatrix();
        glPushMatrix();
        glColor3f(0.16f, 0.20f, 0.24f);
        glTranslatef(0.0f, 0.26f, -0.10f);
        glScalef(0.26f, 0.24f, 0.035f);
        drawCube(1.0f);
        glPopMatrix();
        glPopMatrix();
    };

    // crew figure: recognizable human (head, torso, 2 arms, 2 legs, uniform)
    auto crew = [&](float fx, float fy, float fz, float yaw, bool seated, int v) {
        float coatR, coatG, coatB, pantR, pantG, pantB;
        float skinR, skinG, skinB, hairR, hairG, hairB;
        if (v == 0) {  // enlisted: teal shirt, dark pants, light skin, brown hair
            coatR = 0.16f; coatG = 0.30f; coatB = 0.34f;
            pantR = 0.11f; pantG = 0.12f; pantB = 0.14f;
            skinR = 0.82f; skinG = 0.72f; skinB = 0.62f;
            hairR = 0.18f; hairG = 0.14f; hairB = 0.10f;
        } else {      // officer: navy jacket, tan skin, black hair
            coatR = 0.10f; coatG = 0.14f; coatB = 0.24f;
            pantR = 0.06f; pantG = 0.08f; pantB = 0.12f;
            skinR = 0.72f; skinG = 0.58f; skinB = 0.45f;
            hairR = 0.05f; hairG = 0.05f; hairB = 0.06f;
        }
        float hip = seated ? 0.02f : 0.10f;
        float shY = seated ? 0.33f : 0.46f;
        glPushMatrix();
        glTranslatef(fx, fy, fz);
        glRotatef(yaw, 0, 1, 0);
        // legs
        for (int s = -1; s <= 1; s += 2) {
            glPushMatrix();
            glColor3f(pantR, pantG, pantB);
            if (seated) {
                glPushMatrix();
                glTranslatef(s * 0.065f, hip - 0.015f, 0.16f);
                glScalef(0.085f, 0.075f, 0.34f);
                drawCube(1.0f);
                glPopMatrix();
                glPushMatrix();
                glTranslatef(s * 0.065f, hip - 0.24f, 0.30f);
                glScalef(0.075f, 0.50f, 0.075f);
                drawCube(1.0f);
                glPopMatrix();
            } else {
                glPushMatrix();
                glTranslatef(s * 0.065f, -0.20f, 0.0f);
                glScalef(0.075f, 0.60f, 0.085f);
                drawCube(1.0f);
                glPopMatrix();
            }
            glPushMatrix();
            glColor3f(0.08f, 0.08f, 0.09f);
            glTranslatef(s * 0.065f, -0.515f, seated ? 0.30f : 0.02f);
            glScalef(0.085f, 0.055f, 0.12f);
            drawCube(1.0f);
            glPopMatrix();
            glPopMatrix();
        }
        // torso + belt
        glPushMatrix();
        glColor3f(coatR, coatG, coatB);
        glTranslatef(0.0f, seated ? 0.17f : 0.27f, 0.0f);
        glScalef(0.28f, seated ? 0.32f : 0.38f, 0.18f);
        drawCube(1.0f);
        glPopMatrix();
        glPushMatrix();
        glColor3f(0.06f, 0.07f, 0.08f);
        glTranslatef(0.0f, hip, 0.0f);
        glScalef(0.275f, 0.03f, 0.17f);
        drawCube(1.0f);
        glPopMatrix();
        // arms + hands
        for (int s = -1; s <= 1; s += 2) {
            glPushMatrix();
            glColor3f(coatR, coatG, coatB);
            glTranslatef(s * 0.165f, shY - 0.16f, 0.015f);
            glScalef(0.062f, 0.32f, 0.062f);
            drawCube(1.0f);
            glPopMatrix();
            glPushMatrix();
            glColor3f(skinR, skinG, skinB);
            glTranslatef(s * 0.165f, shY - 0.33f, 0.035f);
            drawSphere(0.032f, 8, 6);
            glPopMatrix();
        }
        // neck, head, hair, collar, face hint
        glPushMatrix();
        glColor3f(skinR, skinG, skinB);
        glTranslatef(0.0f, shY + 0.03f, 0.0f);
        glScalef(0.05f, 0.05f, 0.05f);
        drawCube(1.0f);
        glPopMatrix();
        glPushMatrix();
        glColor3f(skinR, skinG, skinB);
        glTranslatef(0.0f, shY + 0.10f, 0.0f);
        drawSphere(0.105f, 12, 10);
        glPopMatrix();
        glPushMatrix();
        glColor3f(hairR, hairG, hairB);
        glTranslatef(0.0f, shY + 0.125f, -0.015f);
        glScalef(0.105f, 0.055f, 0.105f);
        drawSphere(1.0f, 10, 6);
        glPopMatrix();
        glPushMatrix();
        glColor3f(0.92f, 0.92f, 0.94f);
        glTranslatef(0.0f, shY + 0.04f, 0.085f);
        glScalef(0.11f, 0.025f, 0.03f);
        drawCube(1.0f);
        glPopMatrix();
        if (v == 1) {
            glPushMatrix();
            glColor3f(0.03f, 0.04f, 0.06f);
            glTranslatef(0.0f, shY + 0.135f, 0.085f);
            glScalef(0.14f, 0.015f, 0.07f);
            drawCube(1.0f);
            glPopMatrix();
        }
        glPopMatrix();
    };

    chair(0.97f, -0.52f, 90.0f);
    crew(0.96f, -0.52f, -0.52f, 90.0f, true, 0);
    chair(0.97f,  0.52f, 90.0f);
    crew(0.96f, -0.52f,  0.52f, 90.0f, true, 0);

    // ------------------------------------------------------------------
    // 7. SIDE CONSOLES + seated side operators
    // ------------------------------------------------------------------
    auto scSideText = [&](float lx, float ly, const char* t, void* f) {
        drawText3D(lx, ly, 0.002f, t, f);
    };
    auto drawCrewSideUI = [&](int thema) {
        // vector-only instruments: bitmap text would render metres tall here
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        auto sideQuad = [&](float x1, float y1, float x2, float y2, float zo) {
            glBegin(GL_QUADS);
            glVertex3f(x1, y1, zo);
            glVertex3f(x2, y1, zo);
            glVertex3f(x2, y2, zo);
            glVertex3f(x1, y2, zo);
            glEnd();
        };
        // faint blue backlight so the monitor glows instead of pitch black
        glColor4f(0.03f, 0.11f, 0.20f, 1.0f);
        sideQuad(-0.28f, -0.125f, 0.28f, 0.125f, -0.0005f);
        if (thema == 0) {                       // DEPTH + SPEED instruments
            for (int g = 0; g < 2; g++) {
                float gx = -0.115f + g * 0.23f;
                float gy = 0.015f;
                glColor3f(0.30f, 0.80f, 0.95f);
                for (int tk = 0; tk <= 10; tk++) {
                    float a = -PI + tk * 0.1f * PI;
                    drawLine3D(gx + sin(a) * 0.070f, gy + cos(a) * 0.070f, 0.0f,
                               gx + sin(a) * 0.082f, gy + cos(a) * 0.082f, 0.0f);
                }
                drawRingXY3D(gx, gy, 0.0f, 0.085f, 24);
            }
            float dF = fmod(fabs(sub.depth), 40.0f) / 40.0f;
            if (dF > 1.0f) dF = 1.0f;
            float sF = fabs(sub.speed) / 4.0f;
            if (sF > 1.0f) sF = 1.0f;
            float dA = -PI + dF * PI;
            float sA = -PI + sF * PI;
            glColor3f(0.95f, 0.35f, 0.20f);
            drawLine3D(-0.115f, 0.015f, 0.0f, -0.115f + sin(dA) * 0.075f, 0.015f + cos(dA) * 0.075f, 0.0f);
            glColor3f(0.20f, 0.95f, 0.55f);
            drawLine3D(0.115f, 0.015f, 0.0f, 0.115f + sin(sA) * 0.075f, 0.015f + cos(sA) * 0.075f, 0.0f);
            // filled dial hubs so gauges read at distance
            glColor3f(0.90f, 0.35f, 0.20f);
            sideQuad(-0.125f, 0.005f, -0.105f, 0.025f, 0.0008f);
            glColor3f(0.20f, 0.90f, 0.50f);
            sideQuad(0.105f, 0.005f, 0.125f, 0.025f, 0.0008f);
            glColor3f(0.30f, 0.90f, 1.0f);
            // DEPTH / SPEED tags as vector ticks under each dial
            drawLine3D(-0.160f, -0.055f, 0.0f, -0.070f, -0.055f, 0.0f);
            drawLine3D(0.070f, -0.055f, 0.0f, 0.160f, -0.055f, 0.0f);
            // digital readout bars under the needles
            float dBars = dF * 5.0f;
            float sBars = sF * 5.0f;
            glColor3f(0.75f, 0.85f, 0.90f);
            for (int dbi = 0; dbi < 5; dbi++) {
                if ((float)dbi < dBars)
                    sideQuad(-0.175f + dbi * 0.022f, -0.095f,
                             -0.158f + dbi * 0.022f, -0.075f, 0.0008f);
                if ((float)dbi < sBars)
                    sideQuad(0.055f + dbi * 0.022f, -0.095f,
                             0.072f + dbi * 0.022f, -0.075f, 0.0008f);
            }
        } else {                                // SENSOR / SYSTEMS status
            glColor3f(0.30f, 0.90f, 1.0f);
            // SYSTEMS tag as vector ticks
            drawLine3D(-0.28f, 0.115f, 0.0f, -0.16f, 0.115f, 0.0f);
            // 4 status rows as vector ticks (length shimmers = OK flicker)
            glColor3f(0.45f, 0.85f, 0.95f);
            for (int r2 = 0; r2 < 4; r2++) {
                float ry = 0.065f - r2 * 0.042f;
                float rl = 0.10f + noise01(r2, (int)(introTimer * 0.02f), 73) * 0.06f;
                sideQuad(-0.28f, ry - 0.006f, -0.28f + rl, ry + 0.006f, 0.0008f);
            }
            // animated core-temp bars (filled so they read at distance)
            glColor3f(0.20f, 0.95f, 0.50f);
            for (int tg = 0; tg < 8; tg++) {
                float tv = 0.20f + 0.65f * noise01(tg, (int)(introTimer * 0.02f), 71);
                sideQuad(-0.28f + tg * 0.045f - 0.012f, -0.09f,
                         -0.28f + tg * 0.045f + 0.012f, -0.09f + tv * 0.06f, 0.0008f);
            }
            glColor3f(0.35f, 0.70f, 0.85f);
            drawLine3D(-0.28f, -0.115f, 0.0f, -0.14f, -0.115f, 0.0f);
        }
        glDisable(GL_BLEND);
    };
    auto sideConsole = [&](float cx, float cz, float dir, int thema) {
        for (int k = 0; k < 2; k++) {
            glPushMatrix();
            glTranslatef(cx - 0.30f + k * 0.60f, -0.36f, cz + dir * 0.02f);
            glColor3f(0.14f, 0.16f, 0.19f);
            glScalef(0.12f, 0.36f, 0.30f);
            drawCube(1.0f);
            glPopMatrix();
        }
        glPushMatrix();
        glTranslatef(cx, -0.14f, cz + dir * 0.06f);
        glColor3f(0.24f, 0.28f, 0.32f);
        glScalef(0.72f, 0.03f, 0.32f);
        drawCube(1.0f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(cx, 0.06f, cz + dir * 0.24f);
        glColor3f(0.09f, 0.10f, 0.12f);
        glScalef(0.60f, 0.28f, 0.04f);
        drawCube(1.0f);
        glPopMatrix();
        glDisable(GL_LIGHTING);
        glPushMatrix();
        glTranslatef(cx, 0.07f, cz + dir * 0.245f);
        glColor3f(0.02f, 0.03f, 0.05f);
        glScalef(0.56f, 0.25f, 0.015f);
        drawCube(1.0f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(cx, 0.07f, cz + dir * 0.22f - dir * 0.003f);
        if (dir > 0.0f) glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
        drawCrewSideUI(thema);
        glPopMatrix();
        glEnable(GL_LIGHTING);
        glPushMatrix();
        glTranslatef(cx, -0.30f, cz + dir * 0.12f);
        glColor3f(0.08f, 0.09f, 0.10f);
        glScalef(0.56f, 0.02f, 0.10f);
        drawCube(1.0f);
        glPopMatrix();
        glPushMatrix();
        glColor3f(0.75f, 0.70f, 0.45f);
        glTranslatef(cx - 0.28f, -0.10f, cz + dir * 0.02f);
        glRotatef(-90, 1, 0, 0);
        drawCylinder(0.028f, 0.06f, 8);
        glPopMatrix();
    };
    sideConsole(-0.05f, -1.02f, -1.0f, 0);   // depth / speed station (port)
    sideConsole(-0.05f,  1.02f,  1.0f, 1);   // sensor / systems station (starboard)
    chair(-0.05f, -0.76f, 180.0f);
    crew(-0.05f, -0.52f, -0.76f, 180.0f, true, 0);
    chair(-0.05f,  0.76f, 0.0f);
    crew(-0.05f, -0.52f,  0.76f, 0.0f, true, 0);

    // ------------------------------------------------------------------
    // 8. AFT BULKHEAD - round hatch, cabinets, nav display, standing officer
    // ------------------------------------------------------------------
    glPushMatrix();
    glTranslatef(CR_XA, 0.18f, 0.0f);
    glColor3f(0.34f, 0.37f, 0.41f);
    glScalef(0.08f, 1.42f, 2.46f);
    drawCube(1.0f);
    glPopMatrix();

    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix();
        glTranslatef(CR_XA + 0.12f, 0.18f, s * 0.74f);
        glColor3f(0.14f, 0.18f, 0.24f);
        glScalef(0.22f, 1.15f, 0.30f);
        drawCube(1.0f);
        glPopMatrix();
        for (int d2 = 0; d2 < 2; d2++) {
            glPushMatrix();
            glTranslatef(CR_XA + 0.14f, 0.20f, s * 0.74f + (d2 ? 0.09f : -0.09f));
            glColor3f(0.10f, 0.12f, 0.16f);
            glScalef(0.03f, 0.90f, 0.01f);
            drawCube(1.0f);
            glPopMatrix();
        }
    }

    glDisable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(CR_XA + 0.02f, 0.60f, -0.55f);
    glColor3f(0.02f, 0.04f, 0.06f);
    glScalef(0.015f, 0.16f, 0.24f);
    drawCube(1.0f);
    glPopMatrix();
    // navigation plan view on the aft panel, facing into the room
    glPushMatrix();
    glTranslatef(CR_XA + 0.045f, 0.60f, -0.55f);
    // opaque backlight so it reads as a lit screen against the bulkhead
    glColor3f(0.03f, 0.11f, 0.20f);
    glBegin(GL_QUADS);
    glVertex3f(-0.001f, -0.075f, -0.115f);
    glVertex3f(-0.001f, -0.075f, 0.115f);
    glVertex3f(-0.001f, 0.075f, 0.115f);
    glVertex3f(-0.001f, 0.075f, -0.115f);
    glEnd();
    glColor3f(0.30f, 0.90f, 0.60f);
    for (int g = 0; g <= 3; g++) {
        float gy = -0.07f + g * 0.046f;
        drawLine3D(0.0f, gy, -0.11f, 0.0f, gy, 0.11f);
    }
    for (int g = 0; g <= 3; g++) {
        float gz = -0.11f + g * 0.073f;
        drawLine3D(0.0f, -0.07f, gz, 0.0f, 0.07f, gz);
    }
    float hd = sub.yaw * 0.0174533f;
    drawLine3D(0.0f, 0.0f, 0.0f, 0.0f, sin(hd) * 0.06f, cos(hd) * 0.06f);
    drawLine3D(0.0f, 0.0f, -0.015f, 0.0f, 0.0f, -0.035f);
    glColor3f(0.90f, 0.45f, 0.25f);
    for (int wp = 0; wp < 2; wp++) {
        float wx = -0.04f + wp * 0.07f;
        float wz = -0.05f + (noise01(wp, 2, 51) - 0.5f) * 0.06f;
        for (int e2 = 0; e2 < 4; e2++) {
            float ax = (e2 == 0 || e2 == 3) ? -0.012f : 0.012f;
            float az = (e2 < 2) ? -0.012f : 0.012f;
            float bx = (e2 == 0 || e2 == 3) ? 0.012f : -0.012f;
            float bz = (e2 < 2) ? 0.012f : -0.012f;
            drawLine3D(0.0f, wx + ax, wz + az, 0.0f, wx + bx, wz + bz);
        }
    }
    glColor3f(0.40f, 0.90f, 1.0f);
    drawText3D(CR_XA + 0.055f, 0.70f, -0.60f, "NAV PLAN", GLUT_BITMAP_HELVETICA_10);
    glPopMatrix();
    glEnable(GL_LIGHTING);

    glPushMatrix();
    glTranslatef(-1.44f, -0.46f, 0.98f);
    glColor3f(0.65f, 0.12f, 0.13f);
    glScalef(0.07f, 0.14f, 0.07f);
    drawCube(1.0f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-1.42f, -0.47f, -1.00f);
    glColor3f(0.35f, 0.30f, 0.20f);
    glScalef(0.16f, 0.10f, 0.14f);
    drawCube(1.0f);
    glPopMatrix();

    // round dogged hatch
    glPushMatrix();
    glTranslatef(CR_XA + 0.02f, 0.12f, 0.0f);
    glRotatef(90, 0, 1, 0);
    glColor3f(0.22f, 0.24f, 0.27f);
    drawCylinder(0.34f, 0.05f, 26);
    glColor3f(0.34f, 0.36f, 0.39f);
    drawDisk(0.0f, 0.34f, 26);
    glColor3f(0.16f, 0.17f, 0.19f);
    drawDisk(0.12f, 0.30f, 24);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(CR_XA + 0.07f, 0.12f, 0.0f);
    for (int h = 0; h < 6; h++) {
        glPushMatrix();
        glRotatef((float)h * 60.0f, 1, 0, 0);
        glTranslatef(0.0f, 0.16f, 0.0f);
        glColor3f(0.22f, 0.24f, 0.26f);
        glScalef(0.04f, 0.30f, 0.03f);
        drawCube(1.0f);
        glPopMatrix();
    }
    glColor3f(0.30f, 0.32f, 0.35f);
    drawSphere(0.045f, 8, 6);
    glPopMatrix();
    for (int dg = 0; dg < 6; dg++) {
        float a = (float)dg * 1.0471975f;
        glPushMatrix();
        glTranslatef(CR_XA - 0.005f, 0.12f + sin(a) * 0.30f, cos(a) * 0.30f);
        glRotatef((float)dg * 60.0f, 1, 0, 0);
        glColor3f(0.12f, 0.13f, 0.15f);
        glScalef(0.03f, 0.09f, 0.03f);
        drawCube(1.0f);
        glPopMatrix();
    }

    // standing officer near the hatch
    crew(-1.25f, -0.52f, -0.45f, 90.0f, false, 1);

    glDisable(GL_COLOR_MATERIAL);
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
    glVertex2f(60, windowHeight - 282);
    glVertex2f(250, windowHeight - 282);
    glVertex2f(250, windowHeight - 414);
    glVertex2f(60, windowHeight - 414);
    glEnd();

    // Border
    glColor4f(0.0f, 0.8f, 0.8f, 0.8f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(60, windowHeight - 282);
    glVertex2f(250, windowHeight - 282);
    glVertex2f(250, windowHeight - 414);
    glVertex2f(60, windowHeight - 414);
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

    sprintf(buf, "MISSION: %s", missionShort[currentMission]);
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
    if (cameraMode == CAM_CONTROL_SCREEN) {
        drawText(windowWidth - 304, 149, "W/A/S/D: Walk", GLUT_BITMAP_HELVETICA_12);
        drawText(windowWidth - 304, 134, "Mouse drag: Look around", GLUT_BITMAP_HELVETICA_12);
        drawText(windowWidth - 304, 119, "C: Camera  V: External", GLUT_BITMAP_HELVETICA_12);
        drawText(windowWidth - 304, 104, "ESC: Menu", GLUT_BITMAP_HELVETICA_12);
    } else {
        drawText(windowWidth - 304, 149, "W/S: Fwd/Bwd  A/D: Turn", GLUT_BITMAP_HELVETICA_12);
        drawText(windowWidth - 304, 134, "R/F: Up/Down  Q/E: Roll", GLUT_BITMAP_HELVETICA_12);
        drawText(windowWidth - 304, 119, "L: Lights  C: Camera", GLUT_BITMAP_HELVETICA_12);
        drawText(windowWidth - 304, 104, "Space: Stop  ESC: Menu/Exit", GLUT_BITMAP_HELVETICA_12);
        drawText(windowWidth - 304, 89,  "Mouse: Orbit (Ext view)", GLUT_BITMAP_HELVETICA_12);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawViewportBorder() {
    if (cameraMode == CAM_CONTROL_SCREEN) return; // crew cabin: open view, no frame
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

    glColor4f(0.26f, 0.30f, 0.35f, 0.98f);
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
    glColor3f(0.18f,0.19f,0.21f);
    for(float x=S*0.5f; x<W; x+=28) { glBegin(GL_POINTS); glVertex2f(x, H-T*0.5f); glVertex2f(x, B*0.5f); glEnd(); }
    for(float y=B; y<Cy; y+=22) { glBegin(GL_POINTS); glVertex2f(S*0.5f,y); glVertex2f(W-S*0.5f,y); glEnd(); }

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

    glPointSize(3.0f);
    glLineWidth(1.5f);
    glColor4f(0.42f, 0.47f, 0.52f, 0.90f);
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
        0.45f * diveBlend + 0.18f,
        0.45f * diveBlend + 0.20f,
        0.50f * diveBlend + 0.24f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    // Key sun from FRONT-LEFT-TOP (camera side +Z) so visible hull is lit
    GLfloat sunPos[] = { -30.0f, 28.0f, 35.0f, 0.0f };
    GLfloat sunAmb[] = { 0.35f * diveBlend + 0.15f, 0.33f * diveBlend + 0.15f, 0.30f * diveBlend + 0.16f, 1.0f };
    GLfloat sunDiff[] = { 1.15f * diveBlend + 0.24f, 1.05f * diveBlend + 0.24f, 0.95f * diveBlend + 0.26f, 1.0f };
    GLfloat sunSpec[] = { 0.7f * diveBlend, 0.7f * diveBlend, 0.65f * diveBlend, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, sunPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, sunAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDiff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpec);

    // Submarine hull light (follows sub, strengthens with depth)
    float uw = diveTransition;
    GLfloat subPos[] = { sub.x, sub.y + 0.5f, sub.z, 1.0f };
    GLfloat subAmb[] = { 0.05f + 0.14f * uw, 0.05f + 0.15f * uw, 0.08f + 0.18f * uw, 1.0f };
    GLfloat subDiff[] = { 0.15f + 0.52f * uw, 0.15f + 0.58f * uw, 0.25f + 0.68f * uw, 1.0f };

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
    // Natural deep-sea navy: steep, murky falloff, matches clear colour
    GLfloat fogColor[] = { 0.05f * diveBlend, 0.34f * diveBlend + 0.014f, 0.62f * diveBlend + 0.03f, 1.0f };
    glFogfv(GL_FOG_COLOR, fogColor);
    float fogDensity = 0.022f * diveBlend;
    // Inside the crew control room the big front bow window is the sub's live
    // external-camera feed, so it must stay sharp: lift most of the murk there
    // (the horizon/seabed/rocks still keep a gentle depth haze, but no milky
    // fog wash over the screen). All outside cameras keep the full deep fog.
    if (cameraMode == CAM_CONTROL_SCREEN) fogDensity *= 0.45f;
    glFogf(GL_FOG_DENSITY, fogDensity);
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

    if (!userCameraChoice && (gameState == STATE_INTRO || gameState == STATE_DIVING)) {
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
        float ix = sub.x + cos(subYawRad) * 0.8f;
        float iy = sub.y + 0.3f;
        float iz = sub.z - sin(subYawRad) * 0.8f;
        float fx = sub.x + cos(subYawRad) * 3.0f + cam.freeYaw * 0.02f;
        float fy = sub.y + sin(subPitchRad) * 2.0f + cam.freePitch * 0.015f;
        float fz = sub.z - sin(subYawRad) * 3.0f + cam.freeYaw * 0.015f;
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
        float cosY = cos(subYawRad), sinY = sin(subYawRad);
        float lx = cam.freeX * INT_SX, lz = cam.freeZ * INT_SZ;
        float wx = sub.x + lx * cosY + lz * sinY;
        float wz = sub.z - lx * sinY + lz * cosY;
        float wy = sub.y + INT_EYE * INT_SY + cam.freeY;
        float fyaw = cam.freeYaw * DEG_TO_RAD;
        float fpitch = cam.freePitch * DEG_TO_RAD;
        float fx = wx + cos(fyaw) * cos(fpitch) * 2.0f;
        float fy = wy + sin(fpitch) * 2.0f;
        float fz = wz + sin(fyaw) * cos(fpitch) * 2.0f;
        gluLookAt(wx, wy, wz, fx, fy, fz, 0, 1, 0);
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
// FIRST-PERSON CREW MOVEMENT (crew room / control screen)
// ============================================================
void updateCrewWalk() {
    if (gameState != STATE_PLAYING) return;
    if (cameraMode != CAM_CONTROL_SCREEN) return;

    float yawR = cam.freeYaw * DEG_TO_RAD;
    float mx = 0.0f, mz = 0.0f;
    if (walkKeys['w'] || walkKeys['W']) { mx += cos(yawR); mz += sin(yawR); }
    if (walkKeys['s'] || walkKeys['S']) { mx -= cos(yawR); mz -= sin(yawR); }
    if (walkKeys['a'] || walkKeys['A']) { mx += sin(yawR); mz -= cos(yawR); }
    if (walkKeys['d'] || walkKeys['D']) { mx -= sin(yawR); mz += cos(yawR); }
    if (mx == 0.0f && mz == 0.0f) return;

    float len = sqrt(mx * mx + mz * mz);
    float step = 0.06f; // per frame at ~60fps
    cam.freeX += (mx / len) * step;
    cam.freeZ += (mz / len) * step;

    // Hull walls
    if (cam.freeX < -1.5f) cam.freeX = -1.5f;
    if (cam.freeX > 1.06f) cam.freeX = 1.06f;
    if (cam.freeZ < -1.2f) cam.freeZ = -1.2f;
    if (cam.freeZ > 1.2f) cam.freeZ = 1.2f;

    // Stop just short of the forward dashboard console
    if (cam.freeX > 1.04f) cam.freeX = 1.04f;
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

    const char* menuItems[] = { "START MISSION", "CREW CABIN VIEW", "EXTERNAL 3D VIEW", "CONTROLS", "EXIT" };
    int menuCount = 5;

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
        drawBirds();
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

    if (gameState == STATE_PLAYING || gameState == STATE_DIVING ||
        gameState == STATE_INTRO) {
        bool interiorCam = cameraMode == CAM_INTERIOR || cameraMode == CAM_FRONT_WINDOW ||
            cameraMode == CAM_LEFT_WINDOW || cameraMode == CAM_RIGHT_WINDOW ||
            cameraMode == CAM_CONTROL_SCREEN;
        if (interiorCam) {
            // Draw the underwater world FIRST so it shows through the window glass
            drawFish();
            drawSharks();
            drawRays();
            drawBubbles();
            glDisable(GL_FOG);
            glPushMatrix();
            glTranslatef(sub.x, sub.y, sub.z);
            glRotatef(sub.yaw, 0, 1, 0);
            if (cameraMode == CAM_CONTROL_SCREEN) {
                glScalef(INT_SX, INT_SY, INT_SZ);
                drawCrewCabin();
            } else {
                drawInterior();
            }
            glPopMatrix();
            glEnable(GL_FOG);
        } else {
            drawFullSubmarine();
            drawFish();
            drawSharks();
            drawRays();
            drawBubbles();
        }
    }

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
    walkKeys[(size_t)key] = true;

    if (gameState == STATE_MENU) {
        if (key == 13) { // Enter
            switch (menuSelection) {
            case 0: // START MISSION
                gameState = STATE_PLAYING;
                currentMission = MISSION_DIVE;
                cameraMode = CAM_INTERIOR;
                userCameraChoice = true;
                cam.freeYaw = 0; cam.freePitch = 0;
                missionTimer = 0;
                sub.x = 0; sub.y = -5.0f; sub.z = 0;
                sub.depth = 5.0f;
                break;
            case 1: // CREW CABIN VIEW - first-person walkable middle
                gameState = STATE_PLAYING;
                cameraMode = CAM_CONTROL_SCREEN;
                userCameraChoice = true;
                cam.freeX = 0.0f; cam.freeY = 0.0f; cam.freeZ = 0.0f;
                cam.freeYaw = 0; cam.freePitch = 0;
                missionTimer = 0;
                sub.x = 0; sub.y = -5.0f; sub.z = 0;
                sub.depth = 5.0f;
                break;
            case 2: // EXTERNAL 3D VIEW
                gameState = STATE_PLAYING;
                cameraMode = CAM_EXTERNAL;
                userCameraChoice = true;
                cam.orbitDistance = 14.0f; cam.orbitAngleH = 30.0f; cam.orbitAngleV = 15.0f;
                currentMission = MISSION_DIVE;
                missionTimer = 0;
                sub.x = 0; sub.y = -5.0f; sub.z = 0;
                sub.depth = 5.0f;
                break;
            case 3: // CONTROLS - show info
                break;
            case 4: // EXIT
                exit(0);
                break;
            }
        }
        if (key == 27) exit(0);
        return;
    }

    if (gameState == STATE_INTRO || gameState == STATE_DIVING) {
        if (key == 27) exit(0);
        if (key == 'c' || key == 'C' || key == 'v' || key == 'V' ||
            key == 'i' || key == 'I') {
            userCameraChoice = true;
            if (key == 'c' || key == 'C') {
                cameraMode = (CameraMode)((cameraMode + 1) % CAM_COUNT);
            } else if (key == 'v' || key == 'V') {
                cameraMode = CAM_EXTERNAL;
                if (cam.orbitDistance > 16.0f || cam.orbitDistance < 10.0f) { cam.orbitDistance = 14.0f; cam.orbitAngleH = 30.0f; cam.orbitAngleV = 15.0f; }
            } else {
                cameraMode = CAM_INTERIOR;
            }
        } else if (key == 13) {
            // Skip intro
            gameState = STATE_MENU;
            diveTimer = 6000.0f;
            diveTransition = 1.0f;
            sub.y = -13.0f;
        }
        return;
    }

    if (cameraMode == CAM_CONTROL_SCREEN) {
        // First-person walk: W/A/S/D handled continuously in updateCrewWalk().
        // Submarine controls are disabled here; only camera/UI keys work.
        if (key == 'c' || key == 'C') {
            cameraMode = (CameraMode)((cameraMode + 1) % CAM_COUNT);
            userCameraChoice = true;
        } else if (key == 'v' || key == 'V') {
            cameraMode = CAM_EXTERNAL;
            userCameraChoice = true;
            if (cam.orbitDistance > 16.0f || cam.orbitDistance < 10.0f) { cam.orbitDistance = 14.0f; cam.orbitAngleH = 30.0f; cam.orbitAngleV = 15.0f; }
        } else if (key == 27) {
            gameState = STATE_MENU;
            menuSelection = 0;
            sub.targetSpeed = 0;
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
        userCameraChoice = true;
        if (cameraMode == CAM_CONTROL_SCREEN) {
            cam.freeX = 0; cam.freeY = 0; cam.freeZ = 0;
            cam.freeYaw = 0; cam.freePitch = 0;
        }
        break;
    case 'v': case 'V':
        cameraMode = CAM_EXTERNAL;
        userCameraChoice = true;
        if (cam.orbitDistance > 16.0f || cam.orbitDistance < 10.0f) { cam.orbitDistance = 14.0f; cam.orbitAngleH = 30.0f; cam.orbitAngleV = 15.0f; }
        break;
    case 'i': case 'I':
        cameraMode = CAM_INTERIOR;
        userCameraChoice = true;
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
    walkKeys[(size_t)key] = false;

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
            if (menuSelection < 0) menuSelection = 4;
        }
        if (key == GLUT_KEY_DOWN) {
            menuSelection++;
            if (menuSelection > 4) menuSelection = 0;
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
        } else if (cameraMode == CAM_CONTROL_SCREEN) {
            // Full first-person look: 360 degree yaw, look up/down within reason
            cam.freeYaw += dx * 0.3f;
            cam.freePitch -= dy * 0.25f;
            if (cam.freePitch > 85) cam.freePitch = 85;
            if (cam.freePitch < -85) cam.freePitch = -85;
        } else {
            cam.freeYaw += dx * 0.25f;
            cam.freePitch -= dy * 0.20f;
            if (cam.freePitch > 45) cam.freePitch = 45;
            if (cam.freePitch < -45) cam.freePitch = -45;
            if (cam.freeYaw > 60) cam.freeYaw = 60;
            if (cam.freeYaw < -60) cam.freeYaw = -60;
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
    updateCrewWalk();
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
