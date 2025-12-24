#include <stdio.h>
#include <stdlib.h>
#include "common.h"

int main() {
    printf("=== Program 1: Compile-time linking ===\n\n");
    
    printf("Integral tests:\n");
    float result1 = SinIntegral(0.0f, 3.14159f, 0.001f, METHOD_RECTANGLE);
    printf("  Rectangle method (0, pi): %.6f\n", result1);
    
    float result2 = SinIntegral(0.0f, 3.14159f, 0.001f, METHOD_TRAPEZOID);
    printf("  Trapezoid method (0, pi): %.6f\n", result2);
    
    printf("\nGeometry tests:\n");
    float area1 = Square(5.0f, 3.0f, SHAPE_RECTANGLE);
    printf("  Rectangle (5x3): %.2f\n", area1);
    
    float area2 = Square(5.0f, 3.0f, SHAPE_TRIANGLE);
    printf("  Triangle (5x3): %.2f\n", area2);
    
    return 0;
}
