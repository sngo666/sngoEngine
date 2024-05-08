#ifndef __SNGO_MATH_H
#define __SNGO_MATH_H

#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/vector_float2.hpp>
#include <initializer_list>
#include <limits>
#include <span>
#include <vector>

namespace SngoEngine::Core::Utils::Math
{
using Point2d = glm::vec<2, double>;
using Point2f = glm::vec<2, float>;
using Point2i = glm::vec<2, int>;

using Vector2d = glm::vec<2, double>;
using Vector2f = glm::vec<2, float>;
using Vector2i = glm::vec<2, int>;

inline int Exponent(float v);
inline float Fast_Exp(float x);

//===========================================================================================================================
// FMA
// EvaluatePolynomial(t, a, b ,c) = a + bt + ct^2
//===========================================================================================================================

inline float FMA(float a, float b, float c)
{
  return std::fma(a, b, c);
}

template <typename F, typename C>
inline constexpr float EvaluatePolynomial(F t, C c)
{
  return c;
}

template <typename F, typename C, typename... Args>
inline constexpr float EvaluatePolynomial(F t, C c, Args... cRemaining)
{
  return FMA(t, EvaluatePolynomial(t, cRemaining...), c);
}

//===========================================================================================================================
// Sqr & POW
//===========================================================================================================================

template <typename T>
inline constexpr T Sqr(T v)
{
  return v * v;
}

template <int n>
inline constexpr float Pow(float v)
{
  if constexpr (n < 0)
    return 1 / Pow<(-n)>(v);
  float n2 = Pow<n / 2>(v);
  return n2 * n2 * Pow<n & 1>(v);
}

template <>
inline constexpr float Pow<1>(float v)
{
  return v;
}
template <>
inline constexpr float Pow<0>(float v)
{
  return 1;
}

template <int n>
inline constexpr double Pow(double v)
{
  if constexpr (n < 0)
    return 1 / Pow<(-n)>(v);
  double n2 = Pow<n / 2>(v);
  return n2 * n2 * Pow<n & 1>(v);
}

template <>
inline constexpr double Pow<1>(double v)
{
  return v;
}

template <>
inline constexpr double Pow<0>(double v)
{
  return 1;
}

//===========================================================================================================================
// Lerp
//===========================================================================================================================

// Math Inline Functions
inline float Lerp(float x, float a, float b)
{
  return (1 - x) * a + x * b;
}

//===========================================================================================================================
// Exponent
//===========================================================================================================================

inline int Exponent(double d)
{
  return static_cast<int>((std::bit_cast<uint64_t>(d) >> 52) - 1023);
}

inline int Exponent(float v)
{
  return static_cast<int>((std::bit_cast<uint32_t>(v) >> 23) - 127);
}

//===========================================================================================================================
// Fast_Exp
//===========================================================================================================================

const auto FLOAT_INFINITY = std::numeric_limits<float>::infinity();

inline float Fast_Exp(float x)
{
  // Compute $x'$ such that $\roman{e}^x = 2^{x'}$
  float xp = x * 1.442695041f;

  // Find integer and fractional components of $x'$
  float fxp = std::floor(xp), f = xp - fxp;
  int i = (int)fxp;

  // Evaluate polynomial approximation of $2^f$
  float twoToF = EvaluatePolynomial(f, 1.f, 0.695556856f, 0.226173572f, 0.0781455737f);

  // Scale $2^f$ by $2^i$ and return final result
  int exponent = Exponent(twoToF) + i;
  if (exponent < -126)
    return 0;
  if (exponent > 127)
    return FLOAT_INFINITY;
  auto bits = std::bit_cast<uint32_t>(twoToF);
  bits &= 0b10000000011111111111111111111111u;
  bits |= (exponent + 127) << 23;
  return std::bit_cast<float>(bits);
}

template <typename T, typename U, typename V>
inline constexpr T Clamp(T val, U low, V high)
{
  if (val < low)
    return T(low);
  else if (val > high)
    return T(high);
  else
    return val;
}

//===========================================================================================================================
// DifferenceOfProducts & SumOfProducts
//===========================================================================================================================

template <typename Ta, typename Tb, typename Tc, typename Td>
inline auto DifferenceOfProducts(Ta a, Tb b, Tc c, Td d)
{
  auto cd = c * d;
  auto differenceOfProducts = FMA(a, b, -cd);
  auto error = FMA(-c, d, cd);
  return differenceOfProducts + error;
}

template <typename Ta, typename Tb, typename Tc, typename Td>
inline auto SumOfProducts(Ta a, Tb b, Tc c, Td d)
{
  auto cd = c * d;
  auto sumOfProducts = FMA(a, b, cd);
  auto error = FMA(c, d, -cd);
  return sumOfProducts + error;
}

//===========================================================================================================================
// Quadratic
//===========================================================================================================================

inline bool Quadratic(float a, float b, float c, float* t0, float* t1)
{
  // Handle case of $a=0$ for quadratic solution
  if (a == 0)
    {
      if (b == 0)
        return false;
      *t0 = *t1 = -c / b;
      return true;
    }

  // Find quadratic discriminant
  float discrim = DifferenceOfProducts(b, b, 4 * a, c);
  if (discrim < 0)
    return false;
  float rootDiscrim = std::sqrt(discrim);

  // Compute quadratic _t_ values
  float q = -0.5f * (b + std::copysign(rootDiscrim, b));
  *t0 = q / a;
  *t1 = c / q;
  if (*t0 > *t1)
    std::swap(*t0, *t1);

  return true;
}

//===========================================================================================================================
// InvertBilinear
//===========================================================================================================================

inline Point2f InvertBilinear(Point2f p, std::span<const Point2f> vert)
{
  // The below assumes a quad (vs uv parametric layout) in v....
  Point2f a = vert[0], b = vert[1], c = vert[3], d = vert[2];
  Vector2f e = b - a, f = d - a, g = (a - b) + (c - d), h = p - a;

  auto cross2d = [](Vector2f a, Vector2f b) { return DifferenceOfProducts(a.x, b.y, a.y, b.x); };

  float k2 = cross2d(g, f);
  float k1 = cross2d(e, f) + cross2d(h, g);
  float k0 = cross2d(h, e);

  // if edges are parallel, this is a linear equation
  if (std::abs(k2) < 0.001f)
    {
      if (std::abs(e.x * k1 - g.x * k0) < 1e-5f)
        return {(h.y * k1 + f.y * k0) / (e.y * k1 - g.y * k0), -k0 / k1};
      else
        return {(h.x * k1 + f.x * k0) / (e.x * k1 - g.x * k0), -k0 / k1};
    }

  float v0, v1;
  if (!Quadratic(k2, k1, k0, &v0, &v1))
    return {0, 0};

  float u = (h.x - f.x * v0) / (e.x + g.x * v0);
  if (u < 0 || u > 1 || v0 < 0 || v0 > 1)
    return {(h.x - f.x * v1) / (e.x + g.x * v1), v1};
  return {u, v0};
}

//===========================================================================================================================
// Matrix
//===========================================================================================================================

template <typename Tresult, int N, typename T>
inline Tresult Mul(const glm::mat<N, N, float>& m, const T& v)
{
  Tresult result;
  for (int i = 0; i < N; ++i)
    {
      result[i] = 0;
      for (int j = 0; j < N; ++j)
        result[i] += m[i][j] * v[j];
    }
  return result;
}

template <size_t N, typename T>
inline decltype(auto) Diag(std::initializer_list<T> ts)
{
  std::vector<T> ls{ts};
  const size_t dim{std::min(ts.size(), N)};
  glm::mat<N, N, T> m;
  for (int i = 0; i < dim; i++)
    {
      m[i][i] = ls[i];
    }
  return m;
}

//===========================================================================================================================
// Hash
//===========================================================================================================================

inline uint64_t MurmurHash64A(const unsigned char* key, size_t len, uint64_t seed)
{
  const uint64_t m = 0xc6a4a7935bd1e995ull;
  const int r = 47;

  uint64_t h = seed ^ (len * m);

  const unsigned char* end = key + 8 * (len / 8);

  while (key != end)
    {
      uint64_t k;
      std::memcpy(&k, key, sizeof(uint64_t));
      key += 8;

      k *= m;
      k ^= k >> r;
      k *= m;

      h ^= k;
      h *= m;
    }

  switch (len & 7)
    {
      case 7:
        h ^= uint64_t(key[6]) << 48;
      case 6:
        h ^= uint64_t(key[5]) << 40;
      case 5:
        h ^= uint64_t(key[4]) << 32;
      case 4:
        h ^= uint64_t(key[3]) << 24;
      case 3:
        h ^= uint64_t(key[2]) << 16;
      case 2:
        h ^= uint64_t(key[1]) << 8;
      case 1:
        h ^= uint64_t(key[0]);
        h *= m;
    };

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
}

template <typename T>
inline uint64_t HashBuffer(const T* ptr, size_t size, uint64_t seed = 0)
{
  return MurmurHash64A((const unsigned char*)ptr, size, seed);
}

//===========================================================================================================================
// Hash
//===========================================================================================================================

template <typename Predicate>
inline size_t FindInterval(size_t sz, const Predicate& pred)
{
  using ssize_t = std::make_signed_t<size_t>;
  ssize_t size = (ssize_t)sz - 2, first = 1;
  while (size > 0)
    {
      // Evaluate predicate at midpoint and update _first_ and _size_
      size_t half = (size_t)size >> 1, middle = first + half;
      bool predResult = pred(middle);
      first = predResult ? middle + 1 : first;
      size = predResult ? size - (half + 1) : half;
    }
  return (size_t)Clamp((ssize_t)first - 1, 0, sz - 2);
}

}  // namespace SngoEngine::Core::Utils::Math
#endif