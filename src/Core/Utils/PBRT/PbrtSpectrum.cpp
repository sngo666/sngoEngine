#include "PbrtSpectrum.hpp"

#include <cassert>
#include <cstdint>
#include <glm/detail/qualifier.hpp>
#include <glm/matrix.hpp>
#include <stdexcept>
#include <utility>
#include <vector>

#include "fmt/core.h"
#include "src/Core/Utils/CHECK.hpp"
#include "src/Core/Utils/ColorSpace/ColorSpace.hpp"
#include "src/Core/Utils/ColorSpace/RGBUtils.hpp"
#include "src/Core/Utils/Math.hpp"
#include "src/Core/Utils/Utils.hpp"

//===========================================================================================================================
// SngoEngine::Core::PBRT::Spectrum
//===========================================================================================================================

inline float SngoEngine::Core::PBRT::Spectrum::SampleVisibleWavelengths(float u)
{
  return 538 - 138.888889f * std::atanh(0.85691062f - 1.82750197f * u);
}

inline float SngoEngine::Core::PBRT::Spectrum::VisibleWavelengthsPDF(float lambda)
{
  if (lambda < 360 || lambda > 830)
    return 0;
  return 0.0039398042f / Utils::Math::Sqr(std::cosh(0.0072f * (lambda - 538)));
}

inline float SngoEngine::Core::PBRT::Spectrum::InnerProduct(const Spectrum& f, const Spectrum& g)
{
  float integral = 0;
  float lambda = Lambda_min;
  while (lambda <= Lambda_max)
    {
      lambda += 1;
      integral += f(lambda) * g(lambda);
    }
  return integral;
}

inline float SngoEngine::Core::PBRT::Spectrum::Blackbody(float lambda, float T)
{
  if (T <= 0)
    return 0;
  const float c = 299792458.f;
  const float h = 6.62606957e-34f;
  const float kb = 1.3806488e-23f;
  // Return emitted radiance for blackbody at wavelength _lambda_
  float l = lambda * 1e-9f;
  float Le = (2 * h * c * c)
             / (Utils::Math::Pow<5>(l) * (Utils::Math::Fast_Exp((h * c) / (l * kb * T)) - 1));
  assert(!std::isnan(Le));
  return Le;
}

SngoEngine::Core::PBRT::Spectrum::PiecewiseLinearSpectrum*
SngoEngine::Core::PBRT::Spectrum::FromInterleaved(std::span<const float> samples, bool normalize)
{
  assert(0 == samples.size() % 2);
  uint32_t n = samples.size() / 2;
  std::vector<float> lambda, v;

  // Extend samples to cover range of visible wavelengths if needed.
  if (samples[0] > Lambda_min)
    {
      lambda.push_back(Lambda_min - 1);
      v.push_back(samples[1]);
    }
  for (size_t i = 0; i < n; ++i)
    {
      lambda.push_back(samples[2 * i]);
      v.push_back(samples[2 * i + 1]);
      if (i > 0)
        assert(lambda.back() >= lambda[lambda.size() - 2]);
    }
  if (lambda.back() < Lambda_max)
    {
      lambda.push_back(Lambda_max + 1);
      v.push_back(v.back());
    }

  auto* spec = ALLOC.new_object<PiecewiseLinearSpectrum>(lambda, v);

  if (normalize)
    // Normalize to have luminance of 1.
    spec->Scale(CIE_Y_integral / InnerProduct(spec, &Spectra::Y()));

  return spec;
}

std::optional<SngoEngine::Core::PBRT::Spectrum::Spectrum>
SngoEngine::Core::PBRT::Spectrum::Read_Spectrum(const std::string& fn)
{
  std::vector<float> vals = Utils::Read_FloatFile(fn);
  if (vals.empty())
    {
      throw std::runtime_error("Uunable to read spectrum file." + fn);
      return {};
    }
  else
    {
      if (vals.size() % 2 != 0)
        {
          fmt::println("%s: extra value found in spectrum file.", fn);
          return {};
        }
      std::vector<float> lambda, v;
      for (size_t i = 0; i < vals.size() / 2; ++i)
        {
          if (i > 0 && vals[2 * i] <= lambda.back())
            {
              fmt::println(
                  "%s: spectrum file invalid: at %d'th entry, "
                  "wavelengths aren't "
                  "increasing: %f >= %f.",
                  fn,
                  int(i),
                  lambda.back(),
                  vals[2 * i]);
              return {};
            }
          lambda.push_back(vals[2 * i]);
          v.push_back(vals[2 * i + 1]);
        }
      return Spectrum{ALLOC.new_object<PiecewiseLinearSpectrum>(lambda, v)};
    }
}

SngoEngine::Core::PBRT::Spectrum::SampledSpectrum SngoEngine::Core::PBRT::Spectrum::SafeDiv(
    SampledSpectrum a,
    SampledSpectrum b)
{
  SampledSpectrum r;
  for (int i = 0; i < NSpectrumSamples; ++i)
    r[i] = (b[i] != 0) ? a[i] / b[i] : 0.;
  return r;
}

SngoEngine::Core::RGBUtils::XYZ SngoEngine::Core::PBRT::Spectrum::SpectrumToXYZ(Spectrum s)
{
  return RGBUtils::XYZ(InnerProduct(&Spectra::X(), s),
                       InnerProduct(&Spectra::Y(), s),
                       InnerProduct(&Spectra::Z(), s))
         / CIE_Y_integral;
}

SngoEngine::Core::PBRT::Spectrum::Spectrum SngoEngine::Core::PBRT::Spectrum::GetNamedSpectrum(
    const std::string& name)
{
  auto iter = Spectra::namedSpectra.find(name);
  if (iter != Spectra::namedSpectra.end())
    return iter->second;
  return Spectrum{nullptr};
}

//===========================================================================================================================
// SampledSpectrum
//===========================================================================================================================

SngoEngine::Core::RGBUtils::XYZ SngoEngine::Core::PBRT::Spectrum::SampledSpectrum::ToXYZ(
    const SampledWavelengths& lambda) const
{
  // Sample the $X$, $Y$, and $Z$ matching curves at _lambda_
  SampledSpectrum X = Spectra::X().Sample(lambda);
  SampledSpectrum Y = Spectra::Y().Sample(lambda);
  SampledSpectrum Z = Spectra::Z().Sample(lambda);

  // Evaluate estimator to compute $(x,y,z)$ coefficients
  SampledSpectrum pdf = lambda.PDF();
  return RGBUtils::XYZ{SafeDiv(X * *this, pdf).Average(),
                       SafeDiv(Y * *this, pdf).Average(),
                       SafeDiv(Z * *this, pdf).Average()}
         / CIE_Y_integral;
}

SngoEngine::Core::RGBUtils::RGB SngoEngine::Core::PBRT::Spectrum::SampledSpectrum::ToRGB(
    const SampledWavelengths& lambda,
    const RGBColorSpace& cs) const
{
  RGBUtils::XYZ xyz = ToXYZ(lambda);
  return cs.ToRGB(xyz);
}

//===========================================================================================================================
// SampledWavelengths
//===========================================================================================================================

SngoEngine::Core::PBRT::Spectrum::SampledWavelengths
SngoEngine::Core::PBRT::Spectrum::SampledWavelengths::SampleVisible(float u)
{
  SampledWavelengths swl{};
  for (int i = 0; i < NSpectrumSamples; ++i)
    {
      // Compute _up_ for $i$th wavelength sample
      float up = u + float(i) / NSpectrumSamples;
      if (up > 1)
        up -= 1;

      swl.lambda[i] = SampleVisibleWavelengths(up);
      swl.pdf[i] = VisibleWavelengthsPDF(swl.lambda[i]);
    }
  return swl;
}

//===========================================================================================================================
// BlackbodySpectrum
//===========================================================================================================================

SngoEngine::Core::PBRT::Spectrum::BlackbodySpectrum::BlackbodySpectrum(float T) : T(T)
{
  // Compute blackbody normalization constant for given temperature
  float lambdaMax = 2.8977721e-3f / T;
  normalizationFactor = 1 / Blackbody(lambdaMax * 1e9f, T);
}

float SngoEngine::Core::PBRT::Spectrum::BlackbodySpectrum::operator()(float lambda) const
{
  return Blackbody(lambda, T) * normalizationFactor;
}

[[nodiscard]] SngoEngine::Core::PBRT::Spectrum::SampledSpectrum
SngoEngine::Core::PBRT::Spectrum::BlackbodySpectrum::Sample(const SampledWavelengths& lambda) const
{
  SampledSpectrum s;
  for (int i = 0; i < NSpectrumSamples; ++i)
    s[i] = Blackbody(lambda[i], T) * normalizationFactor;
  return s;
}

//===========================================================================================================================
// DenselySampledSpectrum
//===========================================================================================================================

SngoEngine::Core::PBRT::Spectrum::DenselySampledSpectrum::DenselySampledSpectrum(int lambda_min,
                                                                                 int lambda_max)
    : lambda_min(lambda_min), lambda_max(lambda_max)
{
}

SngoEngine::Core::PBRT::Spectrum::DenselySampledSpectrum::DenselySampledSpectrum(
    const Spectrum& spec,
    int lambda_min,
    int lambda_max)
    : lambda_min(lambda_min), lambda_max(lambda_max), values(lambda_max - lambda_min + 1, 0.f)
{
  if (spec)
    for (int lambda = lambda_min; lambda <= lambda_max; ++lambda)
      values[lambda - lambda_min] = spec(static_cast<float>(lambda));
}

[[nodiscard]] SngoEngine::Core::PBRT::Spectrum::SampledSpectrum
SngoEngine::Core::PBRT::Spectrum::DenselySampledSpectrum::Sample(
    const SampledWavelengths& lambda) const
{
  SampledSpectrum s;
  for (int i = 0; i < NSpectrumSamples; ++i)
    {
      int offset = std::lround(lambda[i]) - lambda_min;
      if (offset < 0 || offset >= values.size())
        s[i] = 0;
      else
        s[i] = values[offset];
    }
  return s;
}

namespace std
{
template <>
struct hash<SngoEngine::Core::PBRT::Spectrum::DenselySampledSpectrum>
{
  size_t operator()(const SngoEngine::Core::PBRT::Spectrum::DenselySampledSpectrum& s) const
  {
    return SngoEngine::Core::Utils::Math::HashBuffer(s.values.data(), s.values.size());
  }
};
}  // namespace std

//===========================================================================================================================
// PiecewiseLinearSpectrum
//===========================================================================================================================

SngoEngine::Core::PBRT::Spectrum::PiecewiseLinearSpectrum::PiecewiseLinearSpectrum(
    std::span<const float> l,
    std::span<const float> v)
    : lambdas(l.begin(), l.end()), values(v.begin(), v.end())
{
  assert(lambdas.size() == values.size());
  for (size_t i = 0; i < lambdas.size() - 1; ++i)
    assert(lambdas[i] <= lambdas[i + 1]);
}

[[nodiscard]] SngoEngine::Core::PBRT::Spectrum::SampledSpectrum
SngoEngine::Core::PBRT::Spectrum::PiecewiseLinearSpectrum::Sample(
    const SampledWavelengths& lambda) const
{
  SampledSpectrum s;
  for (int i = 0; i < NSpectrumSamples; ++i)
    s[i] = (*this)(lambda[i]);
  return s;
}

float SngoEngine::Core::PBRT::Spectrum::PiecewiseLinearSpectrum::operator()(float lambda) const
{
  // Handle _PiecewiseLinearSpectrum_ corner cases
  if (lambdas.empty() || lambda < lambdas.front() || lambda > lambdas.back())
    return 0;

  // Find offset to largest _lambdas_ below _lambda_ and interpolate
  auto o = Utils::Math::FindInterval(lambdas.size(), [&](int i) { return lambdas[i] <= lambda; });
  assert(lambda >= lambdas[o] && lambda <= lambdas[o + 1]);

  float t = (lambda - lambdas[o]) / (lambdas[o + 1] - lambdas[o]);
  return Utils::Math::Lerp(t, values[o], values[o + 1]);
}

//===========================================================================================================================
// ConstantSpectrum
//===========================================================================================================================

// all in .hpp

//===========================================================================================================================
// RGBAlbedoSpectrum
//===========================================================================================================================

SngoEngine::Core::PBRT::Spectrum::RGBAlbedoSpectrum::RGBAlbedoSpectrum(const RGBColorSpace& cs,
                                                                       RGBUtils::RGB rgb)
{
  assert(std::max({rgb.r, rgb.g, rgb.b}) <= 1);
  assert(std::min({rgb.r, rgb.g, rgb.b}) >= 0);
  rsp = cs.ToRGBCoeffs(rgb);
}

//===========================================================================================================================
// Spectrum
//===========================================================================================================================

inline float SngoEngine::Core::PBRT::Spectrum::Spectrum::operator()(float lambda) const
{
  auto op = [&](auto ptr) { return (*ptr)(lambda); };
  return Dispatch(op);
}

inline SngoEngine::Core::PBRT::Spectrum::SampledSpectrum
SngoEngine::Core::PBRT::Spectrum::Spectrum::Sample(const SampledWavelengths& lambda) const
{
  auto samp = [&](auto ptr) { return ptr->Sample(lambda); };
  return Dispatch(samp);
}

inline float SngoEngine::Core::PBRT::Spectrum::Spectrum::MaxValue() const
{
  auto max = [&](auto ptr) { return ptr->MaxValue(); };
  return Dispatch(max);
}

//===========================================================================================================================
// RGBToSpectrumTable
//===========================================================================================================================

SngoEngine::Core::RGBUtils::RGBSigmoidPolynomial
SngoEngine::Core::PBRT::Spectrum::RGBToSpectrumTable::operator()(RGBUtils::RGB rgb) const
{
  assert(rgb[0] >= 0.f && rgb[1] >= 0.f && rgb[2] >= 0.f && rgb[0] <= 1.f && rgb[1] <= 1.f
         && rgb[2] <= 1.f);

  // Handle uniform _rgb_ values
  if (rgb[0] == rgb[1] && rgb[1] == rgb[2])
    return {0, 0, (rgb[0] - .5f) / std::sqrt(rgb[0] * (1 - rgb[0]))};

  // Find maximum component and compute remapped component values
  int maxc = (rgb[0] > rgb[1]) ? ((rgb[0] > rgb[2]) ? 0 : 2) : ((rgb[1] > rgb[2]) ? 1 : 2);
  float z = rgb[maxc];
  float x = rgb[(maxc + 1) % 3] * (res - 1) / z;
  float y = rgb[(maxc + 2) % 3] * (res - 1) / z;

  // Compute integer indices and offsets for coefficient interpolation
  int xi = std::min((int)x, res - 2), yi = std::min((int)y, res - 2),
      zi = Utils::Math::FindInterval(res, [&](int i) { return zNodes[i] < z; });
  float dx = x - xi, dy = y - yi, dz = (z - zNodes[zi]) / (zNodes[zi + 1] - zNodes[zi]);

  // Trilinearly interpolate sigmoid polynomial coefficients _c_
  std::array<float, 3> c{};
  for (int i = 0; i < 3; ++i)
    {
      // Define _co_ lambda for looking up sigmoid polynomial coefficients
      auto co = [&](int dx, int dy, int dz) {
        return (*coeffs)[maxc][zi + dz][yi + dy][xi + dx][i];
      };

      c[i] = Utils::Math::Lerp(dz,
                               Utils::Math::Lerp(dy,
                                                 Utils::Math::Lerp(dx, co(0, 0, 0), co(1, 0, 0)),
                                                 Utils::Math::Lerp(dx, co(0, 1, 0), co(1, 1, 0))),
                               Utils::Math::Lerp(dy,
                                                 Utils::Math::Lerp(dx, co(0, 0, 1), co(1, 0, 1)),
                                                 Utils::Math::Lerp(dx, co(0, 1, 1), co(1, 1, 1))));
    }

  return {c[0], c[1], c[2]};
}

//===========================================================================================================================
// RGBColorSpace
//===========================================================================================================================

SngoEngine::Core::PBRT::Spectrum::RGBColorSpace::RGBColorSpace(
    Utils::Math::Point2f r,
    Utils::Math::Point2f g,
    Utils::Math::Point2f b,
    const Spectrum& illuminant,
    const RGBToSpectrumTable* rgbToSpectrumTable)
    : r(r), g(g), b(b), illuminant(illuminant), rgbToSpectrumTable(rgbToSpectrumTable)
{
  // Compute whitepoint primaries and XYZ coordinates
  RGBUtils::XYZ W = SpectrumToXYZ(illuminant);
  w = W.xy();
  RGBUtils::XYZ R = RGBUtils::XYZ::FromxyY(r), G = RGBUtils::XYZ::FromxyY(g),
                B = RGBUtils::XYZ::FromxyY(b);

  // Initialize XYZ color space conversion matrices
  glm::mat<3, 3, float> rgb(R.x, G.x, B.x, R.y, G.y, B.y, R.z, G.z, B.z);
  RGBUtils::XYZ C{glm::inverse(rgb) * W};
  XYZFromRGB = rgb * Utils::Math::Diag<3>({C[0], C[1], C[2]});
  RGBFromXYZ = glm::inverse(XYZFromRGB);
}

std::shared_ptr<SngoEngine::Core::PBRT::Spectrum::RGBColorSpace>
SngoEngine::Core::PBRT::Spectrum::RGBColorSpace::GetNamed(const std::string& n)
{
  return namedColorSpace[n];
}

std::shared_ptr<SngoEngine::Core::PBRT::Spectrum::RGBColorSpace>
SngoEngine::Core::PBRT::Spectrum::RGBColorSpace::Lookup(Utils::Math::Point2f r,
                                                        Utils::Math::Point2f g,
                                                        Utils::Math::Point2f b,
                                                        Utils::Math::Point2f w)
{
  auto closeEnough = [](const Utils::Math::Point2f& a, const Utils::Math::Point2f& b) {
    return ((a.x == b.x || std::abs((a.x - b.x) / b.x) < 1e-3)
            && (a.y == b.y || std::abs((a.y - b.y) / b.y) < 1e-3));
  };

  for (auto& _pair : namedColorSpace.table)
    {
      auto cs{_pair.second};
      if (closeEnough(r, cs->r) && closeEnough(g, cs->g) && closeEnough(b, cs->b)
          && closeEnough(w, cs->w))
        return cs;
    }
  return nullptr;
}

SngoEngine::Core::RGBUtils::RGBSigmoidPolynomial
SngoEngine::Core::PBRT::Spectrum::RGBColorSpace::ToRGBCoeffs(RGBUtils::RGB rgb) const
{
  assert(rgb.r >= 0 && rgb.g >= 0 && rgb.b >= 0);
  return (*rgbToSpectrumTable)(ClampZero(rgb));
}
