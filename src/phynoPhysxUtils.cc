#include "phynoPhysxUtils.h"
#include "Poco/Logger.h"

using Poco::Logger;

void dump_vect(PxVec3 v)
{
    Poco::Logger *logger = &Logger::get("PhynoMainLogger");
    logger->information("vect : %f   %f    %f", v.x, v.y, v.z);
}

char json2vec3(yyjson_val *jv, PxVec3 *v)
{
    double x, y, z = 0;

    if (yyjson_val *jx = yyjson_obj_get(jv, "x"))
    {
        x = yyjson_get_real(jx);
    }

    if (yyjson_val *jy = yyjson_obj_get(jv, "y"))
    {
        y = yyjson_get_real(jy);
    }

    if (yyjson_val *jz = yyjson_obj_get(jv, "z"))
    {
        z = yyjson_get_real(jz);
    }

    // Decoded all values, update input vector
    v->x = x;
    v->y = y;
    v->z = z;

    return (0);
}

char json2Quat(yyjson_val *jv, PxQuat *q)
{

    double x, y, z = 0;
    double w = 0;

    if (yyjson_val *jx = yyjson_obj_get(jv, "x"))
    {
        x = yyjson_get_real(jx);
    }

    if (yyjson_val *jy = yyjson_obj_get(jv, "y"))
    {
        y = yyjson_get_real(jy);
    }

    if (yyjson_val *jz = yyjson_obj_get(jv, "z"))
    {
        z = yyjson_get_real(jz);
    }

    if (yyjson_val *jw = yyjson_obj_get(jv, "w"))
    {
        w = yyjson_get_real(jw);
    }

    // Decoded all values, update input vector
    q->x = x;
    q->y = y;
    q->z = z;
    q->w = w;

    return (0);
}

char json2Transform(yyjson_val *jt, PxTransform *t)
{

    PxVec3 p;
    PxQuat q;

    if (yyjson_val *jp = yyjson_obj_get(jt, "p"))
    {
        if (json2vec3(jp, &p))
        {
            Poco::Logger *logger = &Logger::get("PhynoMainLogger");
            logger->warning("MQTT_PROCESSOR: Failled to decode vec3 in transform");
            return (1);
        }
        else
        {
            t->p = p;
        }
    }

    if (yyjson_val *jq = yyjson_obj_get(jt, "q"))
    {
        if (json2Quat(jq, &q))
        {
            Poco::Logger *logger = &Logger::get("PhynoMainLogger");
            logger->warning("MQTT_PROCESSOR: Failled to decode vec3 in transform");
            return (1);
        }
        else
            t->q = q;
    }

    return (0);
}

char json2SphereGeometry(yyjson_val *js, PxSphereGeometry *s)
{
    double radius;

    if (yyjson_val *jrad = yyjson_obj_get(js, "radius"))
    {
        radius = yyjson_get_real(jrad);
    }

    s->radius = radius;

    return (0);
}

char json2CapsuleGeometry(yyjson_val *jcap, PxCapsuleGeometry *s)
{
    double radius;
    double halfHeight;

    if (yyjson_val *jrad = yyjson_obj_get(jcap, "radius"))
    {
        radius = yyjson_get_real(jrad);
    }

    s->radius = radius;

    if (yyjson_val *jh = yyjson_obj_get(jcap, "halfHeight"))
    {
        halfHeight = yyjson_get_real(jh);
    }

    s->halfHeight = halfHeight;

    return (0);
}

char json2BoxGeometry(yyjson_val *jBox, PxBoxGeometry *s)
{
    PxVec3 halfExtends;
    if (yyjson_val *jb = yyjson_obj_get(jBox, "halfHeight"))
    {
        json2vec3(jb, &halfExtends);
        s->halfExtents = halfExtends;
        return(0);
    }
    return(1);
}

