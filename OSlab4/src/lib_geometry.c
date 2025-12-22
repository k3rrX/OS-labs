#include "../include/common.h"

float Square(float A, float B, ShapeType shape) {
    if (A <= 0.0f || B <= 0.0f) return 0.0f;
    
    switch (shape) {
        case SHAPE_RECTANGLE:
            return A * B;
        case SHAPE_TRIANGLE:
            return (A * B) / 2.0f;
        default:
            return 0.0f;
    }
}
