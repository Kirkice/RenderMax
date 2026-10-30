/******************************************************************************
 * The MIT License (MIT)
 * Copyright (c) 2026 RenderMax contributors
 ******************************************************************************/
#pragma once

#include "maths/matrix.h"

// The homogeneous W row is constant for an orthographic/affine transform.
inline bool ScenePerspective(const float *m)
{
  for(int i = 0; i < 16; i++)
    if(!std::isfinite(m[i])) return false;
  return m[3] * m[3] + m[7] * m[7] + m[11] * m[11] > 1.0e-8f;
}

inline bool SceneMatrixMatches(const float *a, const float *b)
{
  for(int i = 0; i < 16; i++)
    if(fabsf(a[i] - b[i]) > 0.00001f * RDCMAX(1.0f, fabsf(b[i]))) return false;
  return true;
}

inline Matrix4f SceneCameraMatrix(const float *values, const FloatVector &translation,
                                 const FloatVector &rotation)
{
  Matrix4f original;
  for(int i = 0; i < 16; i++) original[i] = values[i];
  if(translation.x == 0 && translation.y == 0 && translation.z == 0 &&
     rotation.x == 0 && rotation.y == 0 && rotation.z == 0) return original;
  Matrix4f inv = original.Inverse();
  Vec3f eye(0, 0, 0);
  if(fabsf(inv[11]) > 1.0e-8f)
    eye = Vec3f(inv[8] / inv[11], inv[9] / inv[11], inv[10] / inv[11]);
  Vec3f right(values[0], values[4], values[8]);
  Vec3f up(values[1], values[5], values[9]);
  right.Normalise(); up.Normalise();
  Vec3f back = right.Cross(up); back.Normalise(); up = back.Cross(right);
  Matrix4f basis = Matrix4f::Identity();
  basis[0] = right.x; basis[1] = right.y; basis[2] = right.z;
  basis[4] = up.x; basis[5] = up.y; basis[6] = up.z;
  basis[8] = back.x; basis[9] = back.y; basis[10] = back.z;
  Vec3f shift = right * translation.x + up * translation.y + back * translation.z;
  return original.Mul(Matrix4f::Translation(eye).Mul(basis)
      .Mul(Matrix4f::RotationX(-rotation.x).Mul(Matrix4f::RotationY(-rotation.y)))
      .Mul(basis.Transpose()).Mul(Matrix4f::Translation(
          Vec3f(-eye.x - shift.x, -eye.y - shift.y, -eye.z - shift.z))));
}
