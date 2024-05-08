#ifndef __SNGO_RGBUTILS_H
#define __SNGO_RGBUTILS_H

#include <cmath>

#include "src/Core/Utils/Math.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <fmt/core.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace SngoEngine::Core::RGBUtils
{
//===========================================================================================================================
// RGB
//===========================================================================================================================

struct RGB : public glm::vec3
{
  explicit RGB(glm::vec3 v) : vec(v.r, v.g, v.b) {}
  RGB(float _r, float _g, float _b) : vec(_r, _g, _b) {}
  RGB() : vec(0.0f, 0.0f, 0.0f) {}

  friend RGB operator-(float a, RGB s)
  {
    return RGB{a - s.r, a - s.g, a - s.b};
  }

  [[nodiscard]] float Average() const
  {
    return (r + g + b) / 3;
  }
};

inline RGB max(RGB a, RGB b)
{
  return {std::max(a.r, b.r), std::max(a.g, b.g), std::max(a.b, b.b)};
}

inline RGB Lerp(float t, RGB s1, RGB s2)
{
  return RGB{(1 - t) * s1 + t * s2};
}

// RGB Inline Functions
template <typename U, typename V>
inline RGB Clamp(RGB rgb, U min, V max)
{
  return RGB(Utils::Math::Clamp(rgb.r, min, max),
             Utils::Math::Clamp(rgb.g, min, max),
             Utils::Math::Clamp(rgb.b, min, max));
}
inline RGB ClampZero(RGB rgb)
{
  return {std::max<float>(0, rgb.r), std::max<float>(0, rgb.g), std::max<float>(0, rgb.b)};
}

//===========================================================================================================================
// XYZ
//===========================================================================================================================

struct XYZ : public glm::vec3
{
  explicit XYZ(glm::vec3 v) : vec(v.x, v.y, v.z) {}
  XYZ(float _x, float _y, float _z) : vec(_x, _y, _z) {}
  XYZ() : vec(0.0f, 0.0f, 0.0f) {}

  friend XYZ operator-(float a, XYZ s)
  {
    return XYZ{a - s.x, a - s.y, a - s.z};
  }

  XYZ operator/(float a) const
  {
    XYZ ret = *this;
    return XYZ{ret /= a};
  }

  [[nodiscard]] float Average() const
  {
    return (x + y + z) / 3;
  }

  [[nodiscard]] Utils::Math::Point2f xy() const
  {
    return {x / (x + y + z), y / (x + y + z)};
  }

  static XYZ FromxyY(Utils::Math::Point2f xy, float Y = 1)
  {
    if (xy.y == 0)
      return {0, 0, 0};
    return {xy.x * Y / xy.y, Y, (1 - xy.x - xy.y) * Y / xy.y};
  }
};

template <typename U, typename V>
inline XYZ Clamp(const XYZ& xyz, U min, V max)
{
  return XYZ(Utils::Math::Clamp(xyz.x, min, max),
             Utils::Math::Clamp(xyz.y, min, max),
             Utils::Math::Clamp(xyz.z, min, max));
}

inline XYZ ClampZero(const XYZ& xyz)
{
  return {std::max<float>(0, xyz.x), std::max<float>(0, xyz.y), std::max<float>(0, xyz.z)};
}

inline XYZ Lerp(float t, const XYZ& s1, const XYZ& s2)
{
  return XYZ{(1 - t) * s1 + t * s2};
}

//===========================================================================================================================
// RGBSigmoidPolynomial
//===========================================================================================================================

struct RGBSigmoidPolynomial
{
 public:
  // RGBSigmoidPolynomial Public Methods
  RGBSigmoidPolynomial() = default;

  RGBSigmoidPolynomial(float c0, float c1, float c2) : c0(c0), c1(c1), c2(c2) {}

  float operator()(float lambda) const
  {
    return s(Utils::Math::EvaluatePolynomial(lambda, c2, c1, c0));
  }

  [[nodiscard]] float MaxValue() const
  {
    float result = std::max((*this)(360), (*this)(830));
    float lambda = -c1 / (2 * c0);
    if (lambda >= 360 && lambda <= 830)
      result = std::max(result, (*this)(lambda));
    return result;
  }

 private:
  static float s(float x)
  {
    if (std::isinf(x))
      return x > 0 ? 1 : 0;
    return .5f + x / (2 * std::sqrt(1 + Utils::Math::Sqr(x)));
  };

  float c0, c1, c2;
};

}  // namespace SngoEngine::Core::RGBUtils

#endif