#ifndef PHYSXAPI_MATH_H
#define PHYSXAPI_MATH_H

#include <foundation/PxQuat.h>

#define PI 3.14159265358979323846264338327950288419716939937510F

inline float Deg2Rad(const float deg) {
    return deg / 360.0F * 2.0F * PI;
}

#endif // PHYSXAPI_MATH_H
