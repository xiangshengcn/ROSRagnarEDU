#include "ragnar_kinematics/ragnar_kinematics.h"
#include "ragnar_kinematics/ragnar_kinematic_defs.h"

#include <cmath>
#include <iostream>

#define PIOVER180 0.01745329251994329576923690768489F

const static float pi12 = std::acos(0.0);
const static float rad2deg = 180.0 / acosf(-1.0);

const static float theta_max = 0.0f;
const static float theta_min = 0.0f;



// cartesian to actuator - inverse kinematics
bool ragnar_kinematics::inverse_kinematics(const float cartesian_mm[],
                                           float actuator_mm[])
{
  float Q[9];
  int k;
  static const signed char iv0[3] = {0, 0, 1};

  float SOL[8];
  int i;
  float leg[8], sLeg[8], cLeg[8];

  float dv0[3];
  float b_Q[3];
  float b_leg[3];
  float CA[3];
  int i0;
  float C[3];
  float I;
  float J;
  float K;

  float theta;

  // limit theta to platform restrictions
  if (cartesian_mm[3] > theta_max)
    theta = theta_max;
  else if (cartesian_mm[3] < theta_min)
    theta = theta_min;
  else
    theta = cartesian_mm[3];

  Q[0] = cosf(theta);
  Q[3] = -sinf(theta);
  Q[6] = 0.0;
  Q[1] = sinf(theta);
  Q[4] = cosf(theta);
  Q[7] = 0.0;
  for (k = 0; k < 3; k++)
  {
    Q[2 + 3 * k] = iv0[k];
  }

  for (i = 0; i < 4; i++)
  {
    for (k = 0; k < 8; k++)
    {
      leg[k] = ragnar_params[i + (k << 2)];
      sLeg[k] = ragnar_sine_cache[i + (k << 2)];
      cLeg[k] = ragnar_cosine_cache[i + (k << 2)];
    }

    dv0[0] = cLeg[7];
    dv0[1] = sLeg[7];
    dv0[2] = 0.0;

    b_leg[0] = leg[0];
    b_leg[1] = leg[1];
    b_leg[2] = 0.0;
    for (k = 0; k < 3; k++)
    {
      b_Q[k] = 0.0;
      for (i0 = 0; i0 < 3; i0++)
      {
        b_Q[k] += Q[k + 3 * i0] * dv0[i0];
      }

      C[k] = b_Q[k] * leg[6] + cartesian_mm[k];
      CA[k] = C[k] - b_leg[k];
    }

    I = 2.0 * leg[4] * ((C[0] - leg[0]) * sLeg[2] - (C[1] - leg[1]) * cLeg[2]);
    J = -2.0 * leg[4] * (((C[0] - leg[0]) * cLeg[2] * cLeg[3] +
                          (C[1] - leg[1]) * sLeg[2] * cLeg[3]) -
                         C[2] * sLeg[3]);

    K = 0.0;
    for (k = 0; k < 3; k++)
    {
      K += CA[k] * CA[k];
    }

    K = (K + leg[4] * leg[4]) - leg[5] * leg[5];

    SOL[i << 1] = 2.0 * atan((-I + sqrt((I * I + J * J) - K * K)) / (K - J));
    SOL[1 + (i << 1)] =
        2.0 * atan((-I - sqrt((I * I + J * J) - K * K)) / (K - J));
  }

  actuator_mm[0] = (SOL[1] - pi12);
  actuator_mm[1] = (SOL[2] - pi12);
  actuator_mm[2] = (pi12 + SOL[5]);
  actuator_mm[3] = (pi12 + SOL[6]);

  for (int i = 0; i < 4; ++i)
  {
    if (std::isnan(actuator_mm[i])) return false;
  }

  return true;
}

bool ragnar_kinematics::forward_kinematics(const float actuator_mm[],
                                           float cartesian_mm[])
{
  float joints[4] = {(actuator_mm[0] + M_PI_2), (actuator_mm[1] + M_PI_2),
                     (actuator_mm[2] - M_PI_2), (actuator_mm[3] - M_PI_2)};

  float u1, v1, w1;
  float u2, v2, w2;
  float u3, v3, w3;
  float s1x, s1y, s1z;
  float s2x, s2y, s2z;
  float s11, s22;
  float D[4];
  float g[2];
  int r1, r2;
  float xy[2];
  float sxx, sxy, sxz;

  u1 = (-ragnar_params[0] + ragnar_params[24] * ragnar_cosine_cache[28]) +
       ragnar_params[16] *
           (ragnar_sine_cache[8] * sinf(joints[0]) -
            ragnar_cosine_cache[8] * ragnar_cosine_cache[12] * cosf(joints[0]));
  v1 = -((ragnar_params[4] - ragnar_params[24] * ragnar_sine_cache[28]) +
         ragnar_params[16] * (ragnar_cosine_cache[8] * sinf(joints[0]) +
                              ragnar_cosine_cache[12] * ragnar_sine_cache[8] *
                                  cosf(joints[0])));
  w1 = ragnar_params[16] * ragnar_sine_cache[12] * cosf(joints[0]);

  u2 = (-ragnar_params[1] + ragnar_params[25] * ragnar_cosine_cache[29]) +
       ragnar_params[17] *
           (ragnar_sine_cache[9] * sinf(joints[1]) -
            ragnar_cosine_cache[9] * ragnar_cosine_cache[13] * cosf(joints[1]));
  v2 = -((ragnar_params[5] - ragnar_params[25] * ragnar_sine_cache[29]) +
         ragnar_params[17] * (ragnar_cosine_cache[9] * sinf(joints[1]) +
                              ragnar_cosine_cache[13] * ragnar_sine_cache[9] *
                                  cosf(joints[1])));
  w2 = ragnar_params[17] * ragnar_sine_cache[13] * cosf(joints[1]);

  u3 = (-ragnar_params[2] + ragnar_params[26] * ragnar_cosine_cache[30]) +
       ragnar_params[18] * (ragnar_sine_cache[10] * sinf(joints[2]) -
                            ragnar_cosine_cache[10] * ragnar_cosine_cache[14] *
                                cosf(joints[2]));
  v3 = -((ragnar_params[6] - ragnar_params[26] * ragnar_sine_cache[30]) +
         ragnar_params[18] * (ragnar_cosine_cache[10] * sinf(joints[2]) +
                              ragnar_cosine_cache[14] * ragnar_sine_cache[10] *
                                  cosf(joints[2])));
  w3 = ragnar_params[18] * ragnar_sine_cache[14] * cosf(joints[2]);

  s1x = 2.0 * u1 - 2.0 * u2;
  s1y = 2.0 * v1 - 2.0 * v2;
  s1z = 2.0 * w1 - 2.0 * w2;
  s11 = ((((pow(u1, 2.0) - powf(u2, 2.0)) + powf(v1, 2.0)) - powf(v2, 2.0)) +
         powf(w1, 2.0)) -
        powf(w2, 2.0);
  s2x = 2.0 * u1 - 2.0 * u3;
  s2y = 2.0 * v1 - 2.0 * v3;
  s2z = 2.0 * w1 - 2.0 * w3;
  s22 = ((((powf(u1, 2.0) - powf(u3, 2.0)) + powf(v1, 2.0)) - powf(v3, 2.0)) +
         powf(w1, 2.0)) -
        powf(w3, 2.0);

  if ((joints[0] == 1.5707963267948966) && (joints[1] == 1.5707963267948966) &&
      (joints[2] == -1.5707963267948966))
  {
    D[0] = s1x;
    D[2] = s1y;
    D[1] = s2x;
    D[3] = s2y;
    g[0] = -s11;
    g[1] = -s22;

    if (fabs(D[1]) > fabs(D[0]))
    {
      r1 = 1;
      r2 = 0;
    }
    else
    {
      r1 = 0;
      r2 = 1;
    }

    v2 = D[r2] / D[r1];
    u2 = D[2 + r2] - v2 * D[2 + r1];
    xy[1] = (g[r2] - g[r1] * v2) / u2;
    xy[0] = (g[r1] - xy[1] * D[2 + r1]) / D[r1];

    for (r1 = 0; r1 < 2; r1++)
    {
      cartesian_mm[r1] = xy[r1];
    }

    cartesian_mm[2] =
        -sqrtf((powf(ragnar_params[20], 2.0) - powf(xy[0] + u1, 2.0)) -
               powf(xy[1] + v1, 2.0));
  }
  else
  {
    sxx =
        (powf(s1x * s2y - s2x * s1y, 2.0) / powf(s1y * s2z - s2y * s1z, 2.0) +
         powf(s1x * s2z - s2x * s1z, 2.0) / powf(s1y * s2z - s2y * s1z, 2.0)) +
        1.0;
    sxy = (((2.0 * u1 -
             2.0 * v1 * (s1x * s2z - s2x * s1z) / (s1y * s2z - s2y * s1z)) +
            2.0 * w1 * (s1x * s2y - s2x * s1y) / (s1y * s2z - s2y * s1z)) +
           2.0 * (s11 * s2y - s22 * s1y) * (s1x * s2y - s2x * s1y) /
               powf(s1y * s2z - s2y * s1z, 2.0)) +
          2.0 * (s11 * s2z - s22 * s1z) * (s1x * s2z - s2x * s1z) /
              powf(s1y * s2z - s2y * s1z, 2.0);
    sxz = (powf(u1, 2.0) - powf(ragnar_params[20], 2.0) + powf(v1, 2.0) +
           powf(w1, 2.0)) +
          powf(s11 * s2y - s22 * s1y, 2.0) / powf(s1y * s2z - s2y * s1z, 2.0) +
          powf(s11 * s2z - s22 * s1z, 2.0) / powf(s1y * s2z - s2y * s1z, 2.0) -
          2.0 * v1 * (s11 * s2z - s22 * s1z) / (s1y * s2z - s2y * s1z) +
          2.0 * w1 * (s11 * s2y - s22 * s1y) / (s1y * s2z - s2y * s1z);
    cartesian_mm[0] =
        (-sxy + sqrtf(powf(sxy, 2.0) - 4.0 * sxx * sxz)) / (2.0 * sxx);
    cartesian_mm[1] =
        -(((s11 * s2z - s22 * s1z) + s1x * s2z * cartesian_mm[0]) -
          s2x * s1z * cartesian_mm[0]) /
        (s1y * s2z - s2y * s1z);
    cartesian_mm[2] = (((s11 * s2y - s22 * s1y) + s1x * s2y * cartesian_mm[0]) -
                       s2x * s1y * cartesian_mm[0]) /
                      (s1y * s2z - s2y * s1z);
    if (cartesian_mm[2] > 0)
    {
      cartesian_mm[0] =
          (-sxy - sqrtf(powf(sxy, 2.0) - 4.0 * sxx * sxz)) / (2.0 * sxx);
      cartesian_mm[1] =
          -(((s11 * s2z - s22 * s1z) + s1x * s2z * cartesian_mm[0]) -
            s2x * s1z * cartesian_mm[0]) /
          (s1y * s2z - s2y * s1z);
      cartesian_mm[2] =
          (((s11 * s2y - s22 * s1y) + s1x * s2y * cartesian_mm[0]) -
           s2x * s1y * cartesian_mm[0]) /
          (s1y * s2z - s2y * s1z);
    }
  }

  for (int i = 0; i < 4; ++i)
  {
    if (std::isnan(cartesian_mm[i])) return false;
  }
  return true;
}

void calcSingleArmPoints(const Eigen::Vector3f& P,
                         float theta,
                         float ax,
                         float ay,
                         float alpha,
                         float beta,
                         float b,
                         float L,
                         float d,
                         float r,
                         float gama,
                         Eigen::Vector3f& A,
                         Eigen::Vector3f& B,
                         Eigen::Vector3f& C)
{
  using namespace Eigen;
  using std::cos;
  using std::sin;

  // Position vector of Ai in xyz
  A = Vector3f(ax, ay, 0.0f);

  Matrix3f rz, ry, rzz;

  // Position vector of Bi in xyz
  rz << cos(alpha), -sin(alpha),  0,
        sin(alpha), cos(alpha),   0,
        0,          0,            1;

  ry << cos(beta),  0,            sin(beta),
        0,          1,            0,
        -sin(beta), 0,            cos(beta);

  rzz <<  cos(theta), -sin(theta),  0,
          sin(theta), cos(theta),   0,
          0,          0,            1;

  B = rz * ry * rzz * Vector3f(b, 0, 0) + A;

  // axis of rotation
//  Vector3f e = rz * ry * Vector3f(0, 0, 1);
  // TODO: B1

  // Position vectors Ci in xyz
  C = Vector3f(cos(gama), sin(gama), 0) * r + P;
}

// intermediate points
bool ragnar_kinematics::calcIntermediatePoints(const float actuator_mm[],
                                               const float cartesian_mm[],
                                               ragnar_kinematics::IntermediatePoints& pts)
{
  using namespace Eigen;
  using std::cos;
  using std::sin;

  Vector3f P (cartesian_mm[0], cartesian_mm[1], cartesian_mm[2]);

  Vector3f A,B,C;
  // arm 0
  calcSingleArmPoints(P,
      actuator_mm[0] + M_PI_2,
      RAGNAR_JOINT1_BASE_X,
      RAGNAR_JOINT1_BASE_Y,
      RAGNAR_JOINT1_BASE_PAN,
      RAGNAR_JOINT1_BASE_TILT,
      RAGNAR_JOINT1_PRIMARY_ARM,
      RAGNAR_JOINT1_SECONDARY_ARM,
      0.02f,
      RAGNAR_JOINT1_BRIDGED_DISTANCE,
      RAGNAR_JOINT1_FIXED_BRIDGE_ANGLE,
      A,
      B,
      C
  );
  pts.A.col(0) = A;
  pts.B.col(0) = B;
  pts.C.col(0) = C;

  // arm 1
  calcSingleArmPoints(P,
      actuator_mm[1] + M_PI_2,
      RAGNAR_JOINT2_BASE_X,
      RAGNAR_JOINT2_BASE_Y,
      RAGNAR_JOINT2_BASE_PAN,
      RAGNAR_JOINT2_BASE_TILT,
      RAGNAR_JOINT2_PRIMARY_ARM,
      RAGNAR_JOINT2_SECONDARY_ARM,
      0.02f,
      RAGNAR_JOINT2_BRIDGED_DISTANCE,
      RAGNAR_JOINT2_FIXED_BRIDGE_ANGLE,
      A,
      B,
      C
  );
  pts.A.col(1) = A;
  pts.B.col(1) = B;
  pts.C.col(1) = C;

  // arm 2
  calcSingleArmPoints(P,
      actuator_mm[2] - M_PI_2,
      RAGNAR_JOINT3_BASE_X,
      RAGNAR_JOINT3_BASE_Y,
      RAGNAR_JOINT3_BASE_PAN,
      RAGNAR_JOINT3_BASE_TILT,
      RAGNAR_JOINT3_PRIMARY_ARM,
      RAGNAR_JOINT3_SECONDARY_ARM,
      0.02f,
      RAGNAR_JOINT3_BRIDGED_DISTANCE,
      RAGNAR_JOINT3_FIXED_BRIDGE_ANGLE,
      A,
      B,
      C
  );
  pts.A.col(2) = A;
  pts.B.col(2) = B;
  pts.C.col(2) = C;

  // arm 3
  calcSingleArmPoints(P,
      actuator_mm[3] - M_PI_2,
      RAGNAR_JOINT4_BASE_X,
      RAGNAR_JOINT4_BASE_Y,
      RAGNAR_JOINT4_BASE_PAN,
      RAGNAR_JOINT4_BASE_TILT,
      RAGNAR_JOINT4_PRIMARY_ARM,
      RAGNAR_JOINT4_SECONDARY_ARM,
      0.02f,
      RAGNAR_JOINT4_BRIDGED_DISTANCE,
      RAGNAR_JOINT4_FIXED_BRIDGE_ANGLE,
      A,
      B,
      C
  );
  pts.A.col(3) = A;
  pts.B.col(3) = B;
  pts.C.col(3) = C;

  return true;
}

