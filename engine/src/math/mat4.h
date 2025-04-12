#pragma once

#include "math.h"
#include "vec3.h"
#include "math_types.h"

MINLINE mat4 mat4_zero() {
    return (mat4){
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f
    };
}

MINLINE mat4 mat4_identity() {
    return (mat4){
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

MINLINE mat4 mat4_mul(mat4 a, mat4 b) {
    mat4 result = mat4_identity();

    const f32* m1_ptr = a.data;
    const f32* m2_ptr = b.data;
    f32* dst_ptr = result.data;

    for (i32 i = 0; i < 4; ++i) {
        for (i32 j = 0; j < 4; ++j) {
            *dst_ptr = m1_ptr[0] * m2_ptr[0 + j] +
                       m1_ptr[1] * m2_ptr[4 + j] +
                       m1_ptr[2] * m2_ptr[8 + j] +
                       m1_ptr[3] * m2_ptr[12 + j];
            dst_ptr++;
        }
        m1_ptr += 4;
    }
    return result;
}

MINLINE vec3 mat4_mul_vec3(mat4 m, vec3 v) {
    return (vec3){
        m.data[0] * v.x + m.data[1] * v.y + m.data[2] * v.z + m.data[3],
        m.data[4] * v.x + m.data[5] * v.y + m.data[6] * v.z + m.data[7],
        m.data[8] * v.x + m.data[9] * v.y + m.data[10] * v.z + m.data[11]
    };
}

MINLINE vec4 mat4_mul_vec4(mat4 m, vec4 v) {
    return (vec4){
        m.data[0] * v.x + m.data[1] * v.y + m.data[2] * v.z + m.data[3] * v.w,
        m.data[4] * v.x + m.data[5] * v.y + m.data[6] * v.z + m.data[7] * v.w,
        m.data[8] * v.x + m.data[9] * v.y + m.data[10] * v.z + m.data[11] * v.w,
        m.data[12] * v.x + m.data[13] * v.y + m.data[14] * v.z + m.data[15] * v.w
    };
}

MINLINE mat4 mat4_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    mat4 result = mat4_identity();
    
    f32 lr = 1.0f / (left - right);
    f32 bt = 1.0f / (bottom - top);
    f32 nf = 1.0f / (near - far);

    result.data[0] = -2.0f * lr;
    result.data[5] = -2.0f * bt;
    result.data[10] = 2.0f * nf;

    result.data[12] = (left + right) * lr;
    result.data[13] = (top + bottom) * bt;
    result.data[14] = (far + near) * nf;

    return result;
}

MINLINE mat4 mat4_perspective(f32 fov, f32 aspect, f32 near, f32 far) {
    mat4 result = mat4_zero();

    f32 half_tan_fov = mtan(fov * 0.5f);

    result.data[0] = 1.0f / (half_tan_fov * aspect);
    result.data[5] = 1.0f / half_tan_fov;
    result.data[10] = -((far + near) / (far - near));
    result.data[11] = -1.0f;
    result.data[14] = -((2.0f * far * near) / (far - near));

    return result;
}

MINLINE mat4 mat4_look_at(vec3 eye, vec3 target, vec3 up) {
    mat4 result;

    vec3 z_axis = vec3_normalized(vec3_sub(target, eye));
    vec3 x_axis = vec3_normalized(vec3_cross(z_axis, up));
    vec3 y_axis = vec3_cross(x_axis, z_axis);

    result.data[0] = x_axis.x;
    result.data[1] = y_axis.x;
    result.data[2] = -z_axis.x;
    result.data[3] = 0.0f;
    result.data[4] = x_axis.y;
    result.data[5] = y_axis.y;
    result.data[6] = -z_axis.y;
    result.data[7] = 0.0f;
    result.data[8] = x_axis.z;
    result.data[9] = y_axis.z;
    result.data[10] = -z_axis.z;
    result.data[11] = 0.0f;
    result.data[12] = -vec3_dot(x_axis, eye);
    result.data[13] = -vec3_dot(y_axis, eye);
    result.data[14] = vec3_dot(z_axis, eye);
    result.data[15] = 1.0f;

    return result;
}

MINLINE mat4 mat4_transpose(mat4 matrix) {
    mat4 out_matrix = mat4_identity();
    out_matrix.data[0] = matrix.data[0];
    out_matrix.data[1] = matrix.data[4];
    out_matrix.data[2] = matrix.data[8];
    out_matrix.data[3] = matrix.data[12];
    out_matrix.data[4] = matrix.data[1];
    out_matrix.data[5] = matrix.data[5];
    out_matrix.data[6] = matrix.data[9];
    out_matrix.data[7] = matrix.data[13];
    out_matrix.data[8] = matrix.data[2];
    out_matrix.data[9] = matrix.data[6];
    out_matrix.data[10] = matrix.data[10];
    out_matrix.data[11] = matrix.data[14];
    out_matrix.data[12] = matrix.data[3];
    out_matrix.data[13] = matrix.data[7];
    out_matrix.data[14] = matrix.data[11];
    out_matrix.data[15] = matrix.data[15];
    return out_matrix;
}

MINLINE f32 mat4_determinant(mat4 matrix) {
    const f32* m = matrix.data;

    f32 t0 = m[10] * m[15];
    f32 t1 = m[14] * m[11];
    f32 t2 = m[6] * m[15];
    f32 t3 = m[14] * m[7];
    f32 t4 = m[6] * m[11];
    f32 t5 = m[10] * m[7];
    f32 t6 = m[2] * m[15];
    f32 t7 = m[14] * m[3];
    f32 t8 = m[2] * m[11];
    f32 t9 = m[10] * m[3];
    f32 t10 = m[2] * m[7];
    f32 t11 = m[6] * m[3];

    mat3 temp_mat;
    f32* o = temp_mat.data;

    o[0] = (t0 * m[5] + t3 * m[9] + t4 * m[13]) -
           (t1 * m[5] + t2 * m[9] + t5 * m[13]);
    o[1] = (t1 * m[1] + t6 * m[9] + t9 * m[13]) -
           (t0 * m[1] + t7 * m[9] + t8 * m[13]);
    o[2] = (t2 * m[1] + t7 * m[5] + t10 * m[13]) -
           (t3 * m[1] + t6 * m[5] + t11 * m[13]);
    o[3] = (t5 * m[1] + t8 * m[5] + t11 * m[9]) -
           (t4 * m[1] + t9 * m[5] + t10 * m[9]);

    f32 determinant = 1.0f / (m[0] * o[0] + m[4] * o[1] + m[8] * o[2] + m[12] * o[3]);
    return determinant;
}

MINLINE mat4 mat4_inverse(mat4 matrix) {
    const f32* m = matrix.data;

    f32 t0 = m[10] * m[15];
    f32 t1 = m[14] * m[11];
    f32 t2 = m[6] * m[15];
    f32 t3 = m[14] * m[7];
    f32 t4 = m[6] * m[11];
    f32 t5 = m[10] * m[7];
    f32 t6 = m[2] * m[15];
    f32 t7 = m[14] * m[3];
    f32 t8 = m[2] * m[11];
    f32 t9 = m[10] * m[3];
    f32 t10 = m[2] * m[7];
    f32 t11 = m[6] * m[3];
    f32 t12 = m[8] * m[13];
    f32 t13 = m[12] * m[9];
    f32 t14 = m[4] * m[13];
    f32 t15 = m[12] * m[5];
    f32 t16 = m[4] * m[9];
    f32 t17 = m[8] * m[5];
    f32 t18 = m[0] * m[13];
    f32 t19 = m[12] * m[1];
    f32 t20 = m[0] * m[9];
    f32 t21 = m[8] * m[1];
    f32 t22 = m[0] * m[5];
    f32 t23 = m[4] * m[1];

    mat4 out_matrix;
    f32* o = out_matrix.data;

    o[0] = (t0 * m[5] + t3 * m[9] + t4 * m[13]) -
           (t1 * m[5] + t2 * m[9] + t5 * m[13]);
    o[1] = (t1 * m[1] + t6 * m[9] + t9 * m[13]) -
           (t0 * m[1] + t7 * m[9] + t8 * m[13]);
    o[2] = (t2 * m[1] + t7 * m[5] + t10 * m[13]) -
           (t3 * m[1] + t6 * m[5] + t11 * m[13]);
    o[3] = (t5 * m[1] + t8 * m[5] + t11 * m[9]) -
           (t4 * m[1] + t9 * m[5] + t10 * m[9]);

    f32 d = 1.0f / (m[0] * o[0] + m[4] * o[1] + m[8] * o[2] + m[12] * o[3]);

    o[0] = d * o[0];
    o[1] = d * o[1];
    o[2] = d * o[2];
    o[3] = d * o[3];
    o[4] = d * ((t1 * m[4] + t2 * m[8] + t5 * m[12]) -
                (t0 * m[4] + t3 * m[8] + t4 * m[12]));
    o[5] = d * ((t0 * m[0] + t7 * m[8] + t8 * m[12]) -
                (t1 * m[0] + t6 * m[8] + t9 * m[12]));
    o[6] = d * ((t3 * m[0] + t6 * m[4] + t11 * m[12]) -
                (t2 * m[0] + t7 * m[4] + t10 * m[12]));
    o[7] = d * ((t4 * m[0] + t9 * m[4] + t10 * m[8]) -
                (t5 * m[0] + t8 * m[4] + t11 * m[8]));
    o[8] = d * ((t12 * m[7] + t15 * m[11] + t16 * m[15]) -
                (t13 * m[7] + t14 * m[11] + t17 * m[15]));
    o[9] = d * ((t13 * m[3] + t18 * m[11] + t21 * m[15]) -
                (t12 * m[3] + t19 * m[11] + t20 * m[15]));
    o[10] = d * ((t14 * m[3] + t19 * m[7] + t22 * m[15]) -
                 (t15 * m[3] + t18 * m[7] + t23 * m[15]));
    o[11] = d * ((t17 * m[3] + t20 * m[7] + t23 * m[11]) -
                 (t16 * m[3] + t21 * m[7] + t22 * m[11]));
    o[12] = d * ((t14 * m[10] + t17 * m[14] + t13 * m[6]) -
                 (t16 * m[14] + t12 * m[6] + t15 * m[10]));
    o[13] = d * ((t20 * m[14] + t12 * m[2] + t19 * m[10]) -
                 (t18 * m[10] + t21 * m[14] + t13 * m[2]));
    o[14] = d * ((t18 * m[6] + t23 * m[14] + t15 * m[2]) -
                 (t22 * m[14] + t14 * m[2] + t19 * m[6]));
    o[15] = d * ((t22 * m[10] + t16 * m[2] + t21 * m[6]) -
                 (t20 * m[6] + t23 * m[10] + t17 * m[2]));

    return out_matrix;
}

MINLINE mat4 mat4_translation(vec3 position) {
    mat4 result = mat4_identity();
    result.data[12] = position.x;
    result.data[13] = position.y;
    result.data[14] = position.z;
    return result;
}

MINLINE mat4 mat4_euler_x(f32 angle) {
    f32 c = mcos(angle);
    f32 s = msin(angle);

    mat4 result = mat4_identity();
    result.data[5] = c;
    result.data[6] = s;
    result.data[9] = -s;
    result.data[10] = c;
    return result;
}

MINLINE mat4 mat4_euler_y(f32 angle) {
    f32 c = mcos(angle);
    f32 s = msin(angle);

    mat4 result = mat4_identity();
    result.data[0] = c;
    result.data[2] = -s;
    result.data[8] = s;
    result.data[10] = c;
    return result;
}

MINLINE mat4 mat4_euler_z(f32 angle) {
    f32 c = mcos(angle);
    f32 s = msin(angle);

    mat4 result = mat4_identity();
    result.data[0] = c;
    result.data[1] = s;
    result.data[4] = -s;
    result.data[5] = c;
    return result;
}

MINLINE mat4 mat4_euler_xyz(f32 angle_x, f32 angle_y, f32 angle_z) {
    mat4 rx = mat4_euler_x(angle_x);
    mat4 ry = mat4_euler_y(angle_y);
    mat4 rz = mat4_euler_z(angle_z);

    mat4 result = mat4_mul(rx, ry);
    result = mat4_mul(result, rz);
    return result;
}

MINLINE mat4 mat4_scale(vec3 scale) {
    mat4 result = mat4_identity();
    result.data[0] = scale.x;
    result.data[5] = scale.y;
    result.data[10] = scale.z;
    return result;
}

MINLINE vec3 mat4_forward(mat4 m) {
    vec3 result;
    result.x = -m.data[2];
    result.y = -m.data[6];
    result.z = -m.data[10];
    vec3_normalize(&result);
    return result;
}

MINLINE vec3 mat4_backward(mat4 m) {
    vec3 result;
    result.x = m.data[2];
    result.y = m.data[6];
    result.z = m.data[10];
    vec3_normalize(&result);
    return result;
}

MINLINE vec3 mat4_right(mat4 m) {
    vec3 result;
    result.x = m.data[0];
    result.y = m.data[4];
    result.z = m.data[8];
    vec3_normalize(&result);
    return result;
}

MINLINE vec3 mat4_left(mat4 m) {
    vec3 result;
    result.x = -m.data[0];
    result.y = -m.data[4];
    result.z = -m.data[8];
    vec3_normalize(&result);
    return result;
}

MINLINE vec3 mat4_up(mat4 m) {
    vec3 result;
    result.x = m.data[1];
    result.y = m.data[5];
    result.z = m.data[9];
    vec3_normalize(&result);
    return result;
}

MINLINE vec3 mat4_down(mat4 m) {
    vec3 result;
    result.x = -m.data[1];
    result.y = -m.data[5];
    result.z = -m.data[9];
    vec3_normalize(&result);
    return result;
}