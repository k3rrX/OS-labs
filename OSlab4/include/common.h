#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

// Методы интегрирования
typedef enum {
    METHOD_RECTANGLE,
    METHOD_TRAPEZOID
} IntegrationMethod;

// Типы фигур
typedef enum {
    SHAPE_RECTANGLE,
    SHAPE_TRIANGLE
} ShapeType;

// Прототипы функций
float SinIntegral(float A, float B, float e, IntegrationMethod method);
float Square(float A, float B, ShapeType shape);

#ifdef __cplusplus
}
#endif

#endif
