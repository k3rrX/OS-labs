#include <math.h>
#include "../include/common.h"

float SinIntegral(float A, float B, float e, IntegrationMethod method) {
    if (e <= 0.0f || A >= B) return 0.0f;
    
    float integral = 0.0f;
    float x = A;
    int steps = (int)((B - A) / e);
    
    if (method == METHOD_RECTANGLE) {
        for (int i = 0; i < steps; i++) {
            integral += sinf(x) * e;
            x += e;
        }
    } else if (method == METHOD_TRAPEZOID) {
        float prev_value = sinf(A);
        x = A + e;
        
        for (int i = 1; i <= steps; i++) {
            float curr_value = sinf(x);
            integral += (prev_value + curr_value) * e / 2.0f;
            prev_value = curr_value;
            x += e;
        }
    }
    
    return integral;
}
