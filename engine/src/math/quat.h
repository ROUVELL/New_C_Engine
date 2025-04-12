#pragma once

#include "math.h"
#include "mat4.h"
#include "math_types.h"

MINLINE quat quat_identity() {
    return (quat){0.0f, 0.0f, 0.0f, 1.0f};
}

MINLINE f32 quat_normal(quat q) {
    return msqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
}

MINLINE quat quat_normalize(quat q) {
    f32 normal = quat_normal(q);
    return (quat){
        q.x / normal,
        q.y / normal,
        q.z / normal,
        q.w / normal
    };
}

MINLINE quat quat_conjugate(quat q) {
    return (quat){-q.x, -q.y, -q.z, q.w};
}

MINLINE quat quat_inverse(quat q) {
    return quat_normalize(quat_conjugate(q));
}

MINLINE quat quat_mul(quat q_0, quat q_1) {
    quat out_quaternion;

    out_quaternion.x = q_0.x * q_1.w + q_0.y * q_1.z - q_0.z * q_1.y + q_0.w * q_1.x;
    out_quaternion.y = -q_0.x * q_1.z + q_0.y * q_1.w + q_0.z * q_1.x + q_0.w * q_1.y;
    out_quaternion.z = q_0.x * q_1.y - q_0.y * q_1.x + q_0.z * q_1.w + q_0.w * q_1.z;
    out_quaternion.w = -q_0.x * q_1.x - q_0.y * q_1.y - q_0.z * q_1.z + q_0.w * q_1.w;

    return out_quaternion;
}

MINLINE f32 quat_dot(quat q_0, quat q_1) {
    return q_0.x * q_1.x + q_0.y * q_1.y + q_0.z * q_1.z + q_0.w * q_1.w;
}

MINLINE mat4 quat_to_mat4(quat q) {
    mat4 result = mat4_identity();

    // https://stackoverflow.com/questions/1556260/convert-quaternion-rotation-to-rotation-matrix

    quat n = quat_normalize(q);

    result.data[0] = 1.0f - 2.0f * n.y * n.y - 2.0f * n.z * n.z;
    result.data[1] = 2.0f * n.x * n.y - 2.0f * n.z * n.w;
    result.data[2] = 2.0f * n.x * n.z + 2.0f * n.y * n.w;

    result.data[4] = 2.0f * n.x * n.y + 2.0f * n.z * n.w;
    result.data[5] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.z * n.z;
    result.data[6] = 2.0f * n.y * n.z - 2.0f * n.x * n.w;

    result.data[8] = 2.0f * n.x * n.z - 2.0f * n.y * n.w;
    result.data[9] = 2.0f * n.y * n.z + 2.0f * n.x * n.w;
    result.data[10] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.y * n.y;

    return result;
}

MINLINE mat4 quat_to_rotation_matrix(quat q, vec3 center) {
    mat4 out_matrix;

    f32* o = out_matrix.data;
    o[0] = (q.x * q.x) - (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
    o[1] = 2.0f * ((q.x * q.y) + (q.z * q.w));
    o[2] = 2.0f * ((q.x * q.z) - (q.y * q.w));
    o[3] = center.x - center.x * o[0] - center.y * o[1] - center.z * o[2];

    o[4] = 2.0f * ((q.x * q.y) - (q.z * q.w));
    o[5] = -(q.x * q.x) + (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
    o[6] = 2.0f * ((q.y * q.z) + (q.x * q.w));
    o[7] = center.y - center.x * o[4] - center.y * o[5] - center.z * o[6];

    o[8] = 2.0f * ((q.x * q.z) + (q.y * q.w));
    o[9] = 2.0f * ((q.y * q.z) - (q.x * q.w));
    o[10] = -(q.x * q.x) - (q.y * q.y) + (q.z * q.z) + (q.w * q.w);
    o[11] = center.z - center.x * o[8] - center.y * o[9] - center.z * o[10];

    o[12] = 0.0f;
    o[13] = 0.0f;
    o[14] = 0.0f;
    o[15] = 1.0f;
    return out_matrix;
}

MINLINE quat quat_from_axis_angle(vec3 axis, f32 angle) {
    quat result;

    f32 half_angle = 0.5f * angle;
    f32 s = msin(half_angle);

    result.x = s * axis.x;
    result.y = s * axis.y;
    result.z = s * axis.z;
    result.w = mcos(half_angle);

    return result;
}

MINLINE quat quat_slerp(quat q_0, quat q_1, f32 percentage) {
    quat out_quaternion;
    // Source: https://en.wikipedia.org/wiki/Slerp
    // Only unit quaternions are valid rotations.
    // Normalize to avoid undefined behavior.
    quat v0 = quat_normalize(q_0);
    quat v1 = quat_normalize(q_1);

    // Compute the cosine of the angle between the two vectors.
    f32 dot = quat_dot(v0, v1);

    // If the dot product is negative, slerp won't take
    // the shorter path. Note that v1 and -v1 are equivalent when
    // the negation is applied to all four components. Fix by
    // reversing one quaternion.
    if (dot < 0.0f) {
        v1.x = -v1.x;
        v1.y = -v1.y;
        v1.z = -v1.z;
        v1.w = -v1.w;
        dot = -dot;
    }

    const f32 DOT_THRESHOLD = 0.9995f;
    if (dot > DOT_THRESHOLD) {
        // If the inputs are too close for comfort, linearly interpolate
        // and normalize the result.
        out_quaternion = (quat){v0.x + ((v1.x - v0.x) * percentage),
                                v0.y + ((v1.y - v0.y) * percentage),
                                v0.z + ((v1.z - v0.z) * percentage),
                                v0.w + ((v1.w - v0.w) * percentage)};

        return quat_normalize(out_quaternion);
    }

    // Since dot is in range [0, DOT_THRESHOLD], acos is safe
    f32 theta_0 = macos(dot);         // theta_0 = angle between input vectors
    f32 theta = theta_0 * percentage; // theta = angle between v0 and result
    f32 sin_theta = msin(theta);      // compute this value only once
    f32 sin_theta_0 = msin(theta_0);  // compute this value only once

    f32 s0 = mcos(theta) - dot * sin_theta / sin_theta_0; // == sin(theta_0 - theta) / sin(theta_0)
    f32 s1 = sin_theta / sin_theta_0;

    return (quat){
        (v0.x * s0) + (v1.x * s1),
        (v0.y * s0) + (v1.y * s1),
        (v0.z * s0) + (v1.z * s1),
        (v0.w * s0) + (v1.w * s1)
    };
}