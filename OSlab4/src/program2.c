#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "common.h"

int main() {
    printf("=== Program 2: Runtime linking ===\n\n");
    
    void *lib_integral = dlopen("./build/libmath_integral.so", RTLD_LAZY);
    void *lib_geometry = dlopen("./build/libmath_geometry.so", RTLD_LAZY);
    
    if (!lib_integral || !lib_geometry) {
        fprintf(stderr, "Error loading libraries\n");
        return 1;
    }
    
    float (*SinIntegralFunc)(float, float, float, IntegrationMethod);
    float (*SquareFunc)(float, float, ShapeType);
    
    SinIntegralFunc = dlsym(lib_integral, "SinIntegral");
    SquareFunc = dlsym(lib_geometry, "Square");
    
    if (!SinIntegralFunc || !SquareFunc) {
        fprintf(stderr, "Error getting functions\n");
        dlclose(lib_integral);
        dlclose(lib_geometry);
        return 1;
    }
    
    printf("Dynamic loading test:\n");
    float integral = SinIntegralFunc(0.0f, 3.14159f, 0.001f, METHOD_TRAPEZOID);
    float area = SquareFunc(4.0f, 3.0f, SHAPE_TRIANGLE);
    
    printf("  Integral sin(x) [0, pi]: %.4f\n", integral);
    printf("  Triangle area (4x3): %.2f\n", area);
    
    dlclose(lib_integral);
    dlclose(lib_geometry);
    
    printf("\nSuccess!\n");
    return 0;
}
