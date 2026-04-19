#ifndef PHYSXAPI_PHYSXMATH_H
#define PHYSXAPI_PHYSXMATH_H

#include <foundation/PxQuat.h>

#define PHYSXAPI_PI 3.14159265358979323846264338327950288419716939937510F

inline float PhysxApiDeg2Rad(const float deg)
{
    return deg / 360.0F * 2.0F * PHYSXAPI_PI;
}

#endif // PHYSXAPI_PHYSXMATH_H

