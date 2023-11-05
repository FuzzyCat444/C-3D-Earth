#include <math.h>
#include <stdlib.h>
#include <stdint.h>

#include "cgif.h"

#include "earth_data.h"

#define PI 3.141592653589793
#define PI_OVER_TWO 1.570796326794896
#define TWO_PI 6.283185307179586
#define DEG_TO_RAD 0.01745329251994329

typedef struct Vec3 {
    double x, y, z;
} Vec3;

// Sum of two vectors.
Vec3 vsum(Vec3 a, Vec3 b) {
    return (Vec3) { a.x + b.x, a.y + b.y, a.z + b.z };
}

// Difference of two vectors.
Vec3 vdiff(Vec3 a, Vec3 b) {
    return (Vec3) { a.x - b.x, a.y - b.y, a.z - b.z };
}

// Scale a vector.
Vec3 vscl(Vec3 a, double s) {
    return (Vec3) { s * a.x, s * a.y, s * a.z };
}

// Cartesian dot product of two vectors.
// https://en.wikipedia.org/wiki/Dot_product
double vdot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Get magnitude squared (length squared) of a vector.
// The dot product of a vector with itself is equivalent to the
// pythagorean theorem.
// https://en.wikipedia.org/wiki/Dot_product
double vmag2(Vec3 a) {
    return vdot(a, a);
}

// Rotate on XY plane.
// https://en.wikipedia.org/wiki/Rotation_matrix
Vec3 vrotxy(Vec3 v, double c, double s) {
    return (Vec3) { v.x * c - v.y * s, v.x * s + v.y * c, v.z };
}

// Rotate on YZ plane.
// https://en.wikipedia.org/wiki/Rotation_matrix
Vec3 vrotyz(Vec3 v, double c, double s) {
    return (Vec3) { v.x, v.y * c - v.z * s, v.y * s + v.z * c };
}

// Rotate on ZX plane.
// https://en.wikipedia.org/wiki/Rotation_matrix
Vec3 vrotzx(Vec3 v, double c, double s) {
    return (Vec3) { v.z * s + v.x * c, v.y, v.z * c - v.x * s };
}

// Find intersection between ray and sphere.
// https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection
Vec3 raySphere(Vec3 o, Vec3 u, Vec3 c, double r) {
    // Normalize u (make it have length 1). This allows us to use the simpler
    // Wikipedia formula at the line "Note that in the specific case where u 
    // is a unit vector, we can simplify this further"
    u = vscl(u, 1.0 / sqrt(vmag2(u)));
    
    // Initialize values according to wikipedia article.
    Vec3 oc = vdiff(o, c);
    double oc2 = vmag2(oc);
    double udotoc = vdot(u, oc);
    double udotoc2 = udotoc * udotoc;
    double r2 = r * r;
    
    // On wikipedia, this variable is the upside down triangle symbol.
    // We can just distribute the (-) and get rid of the parentheses.
    double del = udotoc2 - oc2 + r2;
    // del < 0.0 means no solutions
    if (del < 0.0)
        return (Vec3) { INFINITY, INFINITY, INFINITY };
    // We only care about positive d, so the closer value is obtained by
    // subtracting sqrt(del). A smaller d means a closer intersection.
    double d = -udotoc - sqrt(del);
    // d < 0.0 means that the ray hit the sphere "in reverse."
    if (d < 0.0)
        return (Vec3) { INFINITY, INFINITY, INFINITY };
    
    // Return origin point + direction * distance.
    return vsum(o, vscl(u, d));
}

// Get normal given a point on a sphere.
Vec3 sphereNormal(Vec3 c, double r, Vec3 p) {
    return vscl(vdiff(p, c), 1.0 / r);
}

// X (U) texture coordinate given normal of sphere.
// https://en.wikipedia.org/wiki/UV_mapping
int texCoordX(Vec3 n, int width) {
    double arctangent = atan2(n.x, n.z);
    if (arctangent < 0.0) arctangent += TWO_PI;
    
    int x = (int) (arctangent * width / TWO_PI);
    if (x < 0) x = 0;
    else if (x >= width) x = width - 1;
    
    return x;
}

// Y (V) texture coordinate given normal of sphere.
// https://en.wikipedia.org/wiki/UV_mapping
int texCoordY(Vec3 n, int height) {
    double arcsine = asin(-n.y);
    arcsine += PI_OVER_TWO;
    
    int y = (int) (arcsine * height / PI);
    if (y < 0) y = 0;
    else if (y >= height) y = height - 1;
    
    return y;
}

// Render the earth.
// time = seconds elapsed.
// totalTime = length of whole animation in seconds.
void traceGlobe(uint8_t* screen, int width, int height, 
    double time, double totalTime) {
    
    // Field of view.
    const double fov = 60.0;
    // Tangent of half of fov (slope of frustum).
    // tanFov2x and tanFov2y are 1/2 of the dimensions of the "near plane"
    // at z = 1. We will use them to construct the view's rays.
    double tanFov2x = tan(fov / 2.0 * DEG_TO_RAD);
    double tanFov2y = tanFov2x * height / width;
    double pixelSize = 2.0 * tanFov2x / width;
    
    // Light source direction.
    Vec3 light = { 1.0, 0.0, -1.0 };
    light = vscl(light, 1.0 / sqrt(vmag2(light)));
    
    // Center and radius of globe for raySphere() function.
    Vec3 c = { 0.0, 0.0, 0.0 };
    double r = 1.0;
    
    // Compute sine and cosine of earth's spin and axis tilt.
    // We will complete one full rotation (2*pi).
    double rot = -TWO_PI * time / totalTime;
    double cRot = cos(rot);
    double sRot = sin(rot);
    double tilt = 23.4 * DEG_TO_RAD;
    double cTilt = cos(tilt);
    double sTilt = sin(tilt);
    
    int i = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Origin (view/camera center) in front of globe.
            Vec3 o = { 0.0, 0.0, 2.2 };
            // Create ray direction vector based on near plane z = 1.
            Vec3 u = { 
                -tanFov2x + pixelSize * (x + 0.5), 
                tanFov2y - pixelSize * (y - 0.5),
                -1.0
            };
            // Find point where ray hits sphere and the surface normal.
            Vec3 p = raySphere(o, u, c, r);
            
            // Ray hit the globe.
            if (!isinf(p.x)) {
                Vec3 n = sphereNormal(c, r, p);
            
                // Calculate brightness of point on sphere from light source.
                double bright = -vdot(n, light);
                int brightI = (int) (bright * 6.0);
                if (brightI > 3) brightI = 3;
                else if (brightI < 0) brightI = 0;
                
                // Rotate normals so that texture will be sampled at different
                // locations so it appears the sphere itself is rotating.
                n = vrotxy(n, cTilt, sTilt);
                n = vrotzx(n, cRot, sRot);
                
                // Sample texture value (0 or 1, ocean or land).
                int texX = texCoordX(n, EARTH_DATA_WIDTH);
                int texY = texCoordY(n, EARTH_DATA_HEIGHT);
                int sample = sampleEarthData(texX, texY);
                
                // Select one of four colors for ocean or one of four colors
                // for land.
                screen[i] = 1 + 4 * sample + brightI;
            // Ray did not hit the globe
            } else {
                // Set color to background color (black).
                screen[i] = 0;
            }
            
            i++;
        }
    }
}

int main(int argc, char* argv[]) {
    
    const int width = 500;
    const int height = 500;
    uint8_t* screen = (uint8_t*) 
        malloc(width * height * sizeof(uint8_t));
        
    const int numFrames = 200;
    const uint16_t frameDelay = 3;
    const int numColors = 9;
    uint8_t palette[] = {
        // Background color
        0, 0, 0,
        // Blues
        0, 19, 88,
        0, 24, 132,
        0, 28, 169,
        0, 32, 207,
        // Greens
        0, 82, 9,
        8, 133, 5,
        14, 169, 3,
        21, 210, 0
    };
        
    CGIF_Config gifConfig = {
        .pGlobalPalette = palette,
        .path = "globe.gif",
        .attrFlags = CGIF_ATTR_IS_ANIMATED,
        .genFlags = 0,
        .width = width,
        .height = height,
        .numGlobalPaletteEntries = numColors,
        .numLoops = 0,
        .pWriteFn = NULL,
        .pContext = NULL
    };
    CGIF_FrameConfig frameConfig = {
        .pLocalPalette = NULL,
        .pImageData = screen,
        .attrFlags = 0,
        .genFlags = 0,
        .delay = frameDelay,
        .numLocalPaletteEntries = 0,
        .transIndex = 0
    };
    CGIF* gif = cgif_newgif(&gifConfig);
    
    double time = 0.0;
    // Frame delay is in hundredths of a second.
    double timeIncr = 0.01 * frameDelay;
    double totalTime = timeIncr * numFrames;
    for (int i = 0; i < numFrames; i++) {
        traceGlobe(screen, width, height, time, totalTime);
        cgif_addframe(gif, &frameConfig);
        time = i * timeIncr;
    }
    
    cgif_close(gif);
    free(screen);
    
    return 0;
}