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
  // The captured D3D shaders consume this as a row-major VP matrix.  Do not
  // invert it to guess an eye point: a VP matrix is not invertible in the
  // affine sense and that produced a matrix which clipped the whole scene.
  // Apply a camera-space delta directly and preserve the captured projection.
  Matrix4f delta = Matrix4f::RotationY(-rotation.y).Mul(Matrix4f::RotationX(-rotation.x))
                       .Mul(Matrix4f::Translation(
                           Vec3f(-translation.x, -translation.y, -translation.z)));
  Matrix4f changed = original.Mul(delta);
  for(int i = 0; i < 16; i++)
    if(!std::isfinite(changed[i])) return original;
  return changed;
}
