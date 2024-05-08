#ifndef __PBRT_SPECTRUM_H
#define __PBRT_SPECTRUM_H

#include <algorithm>
#include <cassert>
#include <cmath>
#include <glm/ext/vector_float3.hpp>
#include <map>
#include <memory>
#include <memory_resource>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "SpectrumData.hpp"
#include "src/Core/Utils/ColorSpace/RGBUtils.hpp"
#include "src/Core/Utils/Math.hpp"
#include "src/Core/Utils/TaggedPointer.hpp"

namespace SngoEngine::Core::PBRT::Spectrum
{
using Allocator = std::pmr::polymorphic_allocator<std::byte>;
static Allocator ALLOC{};

static constexpr float CIE_Y_integral = 106.856895;
constexpr float Lambda_min = 360, Lambda_max = 830;
static constexpr int NSpectrumSamples = 4;

inline float SampleVisibleWavelengths(float u);
inline float VisibleWavelengthsPDF(float lambda);
inline float Blackbody(float lambda, float T);

struct SampledWavelengths;
struct BlackbodySpectrum;
struct PiecewiseLinearSpectrum;
struct SampledSpectrum;
struct Spectrum;
struct RGBColorSpace;

static std::optional<Spectrum> Read_Spectrum(const std::string& fn);
PiecewiseLinearSpectrum* FromInterleaved(std::span<const float> samples, bool normalize);
inline float InnerProduct(const Spectrum& f, const Spectrum& g);
inline SampledSpectrum SafeDiv(SampledSpectrum a, SampledSpectrum b);
RGBUtils::XYZ SpectrumToXYZ(Spectrum s);
Spectrum GetNamedSpectrum(const std::string& name);

struct SampledSpectrum
{
 public:
  // SampledSpectrum Public Methods
  SampledSpectrum operator+(const SampledSpectrum& s) const
  {
    SampledSpectrum ret = *this;
    return ret += s;
  }

  SampledSpectrum& operator-=(const SampledSpectrum& s)
  {
    for (int i = 0; i < NSpectrumSamples; ++i)
      values[i] -= s.values[i];
    return *this;
  }

  SampledSpectrum operator-(const SampledSpectrum& s) const
  {
    SampledSpectrum ret = *this;
    return ret -= s;
  }

  friend SampledSpectrum operator-(float a, const SampledSpectrum& s)
  {
    assert(!std::isnan(a));
    SampledSpectrum ret;
    for (int i = 0; i < NSpectrumSamples; ++i)
      ret.values[i] = a - s.values[i];
    return ret;
  }

  SampledSpectrum& operator*=(const SampledSpectrum& s)
  {
    for (int i = 0; i < NSpectrumSamples; ++i)
      values[i] *= s.values[i];
    return *this;
  }

  SampledSpectrum operator*(const SampledSpectrum& s) const
  {
    SampledSpectrum ret = *this;
    return ret *= s;
  }

  SampledSpectrum operator*(float a) const
  {
    assert(!std::isnan(a));

    SampledSpectrum ret = *this;
    for (int i = 0; i < NSpectrumSamples; ++i)
      ret.values[i] *= a;
    return ret;
  }

  SampledSpectrum& operator*=(float a)
  {
    assert(!std::isnan(a));

    for (int i = 0; i < NSpectrumSamples; ++i)
      values[i] *= a;
    return *this;
  }

  friend SampledSpectrum operator*(float a, const SampledSpectrum& s)
  {
    return s * a;
  }

  SampledSpectrum& operator/=(const SampledSpectrum& s)
  {
    for (int i = 0; i < NSpectrumSamples; ++i)
      {
        assert(0 != s.values[i]);
        values[i] /= s.values[i];
      }
    return *this;
  }

  SampledSpectrum operator/(const SampledSpectrum& s) const
  {
    SampledSpectrum ret = *this;
    return ret /= s;
  }

  SampledSpectrum& operator/=(float a)
  {
    assert(a != 0);
    assert(!std::isnan(a));
    for (int i = 0; i < NSpectrumSamples; ++i)
      values[i] /= a;
    return *this;
  }

  SampledSpectrum operator/(float a) const
  {
    SampledSpectrum ret = *this;
    return ret /= a;
  }

  SampledSpectrum operator-() const
  {
    SampledSpectrum ret;
    for (int i = 0; i < NSpectrumSamples; ++i)
      ret.values[i] = -values[i];
    return ret;
  }

  bool operator==(const SampledSpectrum& s) const
  {
    return values == s.values;
  }

  bool operator!=(const SampledSpectrum& s) const
  {
    return values != s.values;
  }

  [[nodiscard]] bool HasNaNs() const
  {
    for (int i = 0; i < NSpectrumSamples; ++i)
      if (std::isnan(values[i]))
        return true;
    return false;
  }

  SampledSpectrum() = default;

  explicit SampledSpectrum(float c)
  {
    values.fill(c);
  }

  explicit SampledSpectrum(std::span<const float> v)
  {
    assert(NSpectrumSamples == v.size());
    for (int i = 0; i < NSpectrumSamples; ++i)
      values[i] = v[i];
  }

  float operator[](int i) const
  {
    assert(i >= 0 && i < NSpectrumSamples);
    return values[i];
  }

  float& operator[](int i)
  {
    assert(i >= 0 && i < NSpectrumSamples);
    return values[i];
  }

  explicit operator bool() const
  {
    for (int i = 0; i < NSpectrumSamples; ++i)
      if (values[i] != 0)
        return true;
    return false;
  }

  SampledSpectrum& operator+=(const SampledSpectrum& s)
  {
    for (int i = 0; i < NSpectrumSamples; ++i)
      values[i] += s.values[i];
    return *this;
  }

  [[nodiscard]] float MinComponentValue() const
  {
    float m = values[0];
    for (int i = 1; i < NSpectrumSamples; ++i)
      m = std::min(m, values[i]);
    return m;
  }

  [[nodiscard]] float MaxComponentValue() const
  {
    float m = values[0];
    for (int i = 1; i < NSpectrumSamples; ++i)
      m = std::max(m, values[i]);
    return m;
  }

  [[nodiscard]] float Average() const
  {
    float sum = values[0];
    for (int i = 1; i < NSpectrumSamples; ++i)
      sum += values[i];
    return sum / NSpectrumSamples;
  }

  [[nodiscard]] RGBUtils::XYZ ToXYZ(const SampledWavelengths& lambda) const;
  [[nodiscard]] RGBUtils::RGB ToRGB(const SampledWavelengths& lambda,
                                    const RGBColorSpace& cs) const;

 private:
  std::array<float, NSpectrumSamples> values{};
};

//===========================================================================================================================
// SampledWavelengths
//===========================================================================================================================

struct SampledWavelengths
{
 public:
  bool operator==(const SampledWavelengths& swl) const
  {
    return lambda == swl.lambda && pdf == swl.pdf;
  }

  bool operator!=(const SampledWavelengths& swl) const
  {
    return lambda != swl.lambda || pdf != swl.pdf;
  }

  static SampledWavelengths SampleUniform(float u,
                                          float lambda_min = Lambda_min,
                                          float lambda_max = Lambda_max)
  {
    SampledWavelengths swl{};
    // Sample first wavelength using _u_
    swl.lambda[0] = Utils::Math::Lerp(u, lambda_min, lambda_max);

    // Initialize _lambda_ for remaining wavelengths
    float delta = (lambda_max - lambda_min) / NSpectrumSamples;
    for (int i = 1; i < NSpectrumSamples; ++i)
      {
        swl.lambda[i] = swl.lambda[i - 1] + delta;
        if (swl.lambda[i] > lambda_max)
          swl.lambda[i] = lambda_min + (swl.lambda[i] - lambda_max);
      }

    // Compute PDF for sampled wavelengths
    for (int i = 0; i < NSpectrumSamples; ++i)
      swl.pdf[i] = 1 / (lambda_max - lambda_min);

    return swl;
  }

  float operator[](int i) const
  {
    return lambda[i];
  }

  float& operator[](int i)
  {
    return lambda[i];
  }

  [[nodiscard]] SampledSpectrum PDF() const
  {
    return SampledSpectrum(pdf);
  }

  void TerminateSecondary()
  {
    if (SecondaryTerminated())
      return;
    // Update wavelength probabilities for termination
    for (int i = 1; i < NSpectrumSamples; ++i)
      pdf[i] = 0;
    pdf[0] /= NSpectrumSamples;
  }

  [[nodiscard]] bool SecondaryTerminated() const
  {
    for (int i = 1; i < NSpectrumSamples; ++i)
      if (pdf[i] != 0)
        return false;
    return true;
  }

  static SampledWavelengths SampleVisible(float u);

 private:
  std::array<float, NSpectrumSamples> lambda;
  std::array<float, NSpectrumSamples> pdf;
};

//===========================================================================================================================
// BlackbodySpectrum
//===========================================================================================================================

struct BlackbodySpectrum
{
 public:
  explicit BlackbodySpectrum(float T);

  float operator()(float lambda) const;

  [[nodiscard]] SampledSpectrum Sample(const SampledWavelengths& lambda) const;

  static float MaxValue()
  {
    return 1.f;
  }

 private:
  // BlackbodySpectrum Private Members
  float T;
  float normalizationFactor;
};

//===========================================================================================================================
// struct DenselySampledSpectrum
//===========================================================================================================================

struct DenselySampledSpectrum
{
 public:
  // DenselySampledSpectrum Public Methods
  explicit DenselySampledSpectrum(int lambda_min = Lambda_min, int lambda_max = Lambda_max);
  explicit DenselySampledSpectrum(const Spectrum& spec,
                                  int lambda_min = Lambda_min,
                                  int lambda_max = Lambda_max);
  DenselySampledSpectrum(const DenselySampledSpectrum& s) = default;

  [[nodiscard]] SampledSpectrum Sample(const SampledWavelengths& lambda) const;

  void Scale(float s)
  {
    for (float& v : values)
      v *= s;
  }

  [[nodiscard]] float MaxValue() const
  {
    return *std::max_element(values.begin(), values.end());
  }

  template <typename F>
  static DenselySampledSpectrum SampleFunction(F func,
                                               int lambda_min = Lambda_min,
                                               int lambda_max = Lambda_max)
  {
    DenselySampledSpectrum s(lambda_min, lambda_max);
    for (int lambda = lambda_min; lambda <= lambda_max; ++lambda)
      s.values[lambda - lambda_min] = func(lambda);
    return s;
  }

  float operator()(float lambda) const
  {
    int offset = std::lround(lambda) - lambda_min;
    if (offset < 0 || offset >= values.size())
      return 0;
    return values[offset];
  }

  bool operator==(const DenselySampledSpectrum& d) const
  {
    if (lambda_min != d.lambda_min || lambda_max != d.lambda_max
        || values.size() != d.values.size())
      return false;
    for (size_t i = 0; i < values.size(); ++i)
      if (values[i] != d.values[i])
        return false;
    return true;
  }

 private:
  friend struct std::hash<DenselySampledSpectrum>;
  int lambda_min, lambda_max;
  std::vector<float> values;
};

//===========================================================================================================================
// PiecewiseLinearSpectrum
//===========================================================================================================================

struct PiecewiseLinearSpectrum
{
 public:
  // PiecewiseLinearSpectrum Public Methods
  PiecewiseLinearSpectrum() = default;

  void Scale(float s)
  {
    for (float& v : values)
      v *= s;
  }

  [[nodiscard]] float MaxValue() const
  {
    return *std::max_element(values.begin(), values.end());
  }

  [[nodiscard]] SampledSpectrum Sample(const SampledWavelengths& lambda) const;

  float operator()(float lambda) const;

  PiecewiseLinearSpectrum(std::span<const float> lambdas, std::span<const float> values);

 private:
  // PiecewiseLinearSpectrum Private Members
  std::vector<float> lambdas{};
  std::vector<float> values{};
};

//===========================================================================================================================
// ConstantSpectrum
//===========================================================================================================================

class ConstantSpectrum
{
 public:
  explicit ConstantSpectrum(float c) : c(c) {}

  float operator()(float lambda) const
  {
    return c;
  }
  // ConstantSpectrum Public Methods

  [[nodiscard]] SampledSpectrum Sample(const SampledWavelengths&) const
  {
    return SampledSpectrum(c);
  }

  [[nodiscard]] float MaxValue() const
  {
    return c;
  }

 private:
  float c;
};

//===========================================================================================================================
// RGBAlbedoSpectrum
//===========================================================================================================================

class RGBAlbedoSpectrum
{
 public:
  // RGBAlbedoSpectrum Public Methods

  float operator()(float lambda) const
  {
    return rsp(lambda);
  }

  [[nodiscard]] float MaxValue() const
  {
    return rsp.MaxValue();
  }

  RGBAlbedoSpectrum(const RGBColorSpace& cs, RGBUtils::RGB rgb);

  [[nodiscard]] PBRT::Spectrum::SampledSpectrum Sample(
      const PBRT::Spectrum::SampledWavelengths& lambda) const
  {
    PBRT::Spectrum::SampledSpectrum s;
    for (int i = 0; i < PBRT::Spectrum::NSpectrumSamples; ++i)
      s[i] = rsp(lambda[i]);
    return s;
  }

 private:
  // RGBAlbedoSpectrum Private Members
  RGBUtils::RGBSigmoidPolynomial rsp{};
};

//===========================================================================================================================
// RGBUnboundedSpectrum
//===========================================================================================================================

class RGBUnboundedSpectrum
{
 public:
  // RGBUnboundedSpectrum Public Methods

  float operator()(float lambda) const
  {
    return scale * rsp(lambda);
  }

  [[nodiscard]] float MaxValue() const
  {
    return scale * rsp.MaxValue();
  }

  RGBUnboundedSpectrum(const RGBColorSpace& cs, RGBUtils::RGB rgb);
  RGBUnboundedSpectrum() : scale(0), rsp(0, 0, 0) {}
  [[nodiscard]] SampledSpectrum Sample(const SampledWavelengths& lambda) const
  {
    SampledSpectrum s;
    for (int i = 0; i < NSpectrumSamples; ++i)
      s[i] = scale * rsp(lambda[i]);
    return s;
  }

 private:
  // RGBUnboundedSpectrum Private Members
  float scale = 1;
  RGBUtils::RGBSigmoidPolynomial rsp;
};

//===========================================================================================================================
// RGBIlluminantSpectrum
//===========================================================================================================================

class RGBIlluminantSpectrum
{
 public:
  // RGBIlluminantSpectrum Public Methods
  RGBIlluminantSpectrum() = default;

  RGBIlluminantSpectrum(const RGBColorSpace& cs, RGBUtils::RGB rgb);

  float operator()(float lambda) const
  {
    if (!illuminant)
      return 0;
    return scale * rsp(lambda) * (*illuminant)(lambda);
  }

  [[nodiscard]] float MaxValue() const
  {
    if (!illuminant)
      return 0;
    return scale * rsp.MaxValue() * illuminant->MaxValue();
  }

  [[nodiscard]] const DenselySampledSpectrum* Illuminant() const
  {
    return illuminant;
  }

  [[nodiscard]] SampledSpectrum Sample(const SampledWavelengths& lambda) const
  {
    if (!illuminant)
      return SampledSpectrum(0);
    SampledSpectrum s;
    for (int i = 0; i < NSpectrumSamples; ++i)
      s[i] = scale * rsp(lambda[i]);
    return s * illuminant->Sample(lambda);
  }

 private:
  // RGBIlluminantSpectrum Private Members
  float scale;
  RGBUtils::RGBSigmoidPolynomial rsp;
  const DenselySampledSpectrum* illuminant;
};

//===========================================================================================================================
// Spectrum
//===========================================================================================================================

struct Spectrum : public TaggedPointer::TaggedPointer<ConstantSpectrum,
                                                      DenselySampledSpectrum,
                                                      PiecewiseLinearSpectrum,
                                                      RGBAlbedoSpectrum,
                                                      RGBUnboundedSpectrum,
                                                      RGBIlluminantSpectrum,
                                                      BlackbodySpectrum>
{
  using TaggedPointer::TaggedPointer;

  float operator()(float lambda) const;
  [[nodiscard]] float MaxValue() const;
  [[nodiscard]] SampledSpectrum Sample(const SampledWavelengths& lambda) const;
};

//===========================================================================================================================
// Spectra
//===========================================================================================================================

namespace Spectra
{
static DenselySampledSpectrum* x;
static DenselySampledSpectrum* y;
static DenselySampledSpectrum* z;

static std::map<std::string, Spectrum> named_Spectra_init() noexcept
{
  PiecewiseLinearSpectrum xpls(CIE_lambda, CIE_X);
  Spectra::x = ALLOC.new_object<DenselySampledSpectrum>(&xpls);

  PiecewiseLinearSpectrum ypls(CIE_lambda, CIE_Y);
  y = ALLOC.new_object<DenselySampledSpectrum>(&ypls);

  PiecewiseLinearSpectrum zpls(CIE_lambda, CIE_Z);
  z = ALLOC.new_object<DenselySampledSpectrum>(&zpls);

  Spectrum illuma{FromInterleaved(CIE_Illum_A, true)};
  Spectrum illumd50{FromInterleaved(CIE_Illum_D5000, true)};
  Spectrum illumacesd60{FromInterleaved(ACES_Illum_D60, true)};
  Spectrum illumd65{FromInterleaved(CIE_Illum_D6500, true)};
  Spectrum illumf1{FromInterleaved(CIE_Illum_F1, true)};
  Spectrum illumf2{FromInterleaved(CIE_Illum_F2, true)};
  Spectrum illumf3{FromInterleaved(CIE_Illum_F3, true)};
  Spectrum illumf4{FromInterleaved(CIE_Illum_F4, true)};
  Spectrum illumf5{FromInterleaved(CIE_Illum_F5, true)};
  Spectrum illumf6{FromInterleaved(CIE_Illum_F6, true)};
  Spectrum illumf7{FromInterleaved(CIE_Illum_F7, true)};
  Spectrum illumf8{FromInterleaved(CIE_Illum_F8, true)};
  Spectrum illumf9{FromInterleaved(CIE_Illum_F9, true)};
  Spectrum illumf10{FromInterleaved(CIE_Illum_F10, true)};
  Spectrum illumf11{FromInterleaved(CIE_Illum_F11, true)};
  Spectrum illumf12{FromInterleaved(CIE_Illum_F12, true)};

  Spectrum ageta{FromInterleaved(Ag_eta, false)};
  Spectrum agk{FromInterleaved(Ag_k, false)};
  Spectrum aleta{FromInterleaved(Al_eta, false)};
  Spectrum alk{FromInterleaved(Al_k, false)};
  Spectrum aueta{FromInterleaved(Au_eta, false)};
  Spectrum auk{FromInterleaved(Au_k, false)};
  Spectrum cueta{FromInterleaved(Cu_eta, false)};
  Spectrum cuk{FromInterleaved(Cu_k, false)};
  Spectrum cuzneta{FromInterleaved(CuZn_eta, false)};
  Spectrum cuznk{FromInterleaved(CuZn_k, false)};
  Spectrum mgoeta{FromInterleaved(MgO_eta, false)};
  Spectrum mgok{FromInterleaved(MgO_k, false)};
  Spectrum tio2eta{FromInterleaved(TiO2_eta, false)};
  Spectrum tio2k{FromInterleaved(TiO2_k, false)};
  Spectrum glassbk7eta{FromInterleaved(GlassBK7_eta, false)};
  Spectrum glassbaf10eta{FromInterleaved(GlassBAF10_eta, false)};
  Spectrum glassfk51aeta{FromInterleaved(GlassFK51A_eta, false)};
  Spectrum glasslasf9eta{FromInterleaved(GlassLASF9_eta, false)};
  Spectrum glasssf5eta{FromInterleaved(GlassSF5_eta, false)};
  Spectrum glasssf10eta{FromInterleaved(GlassSF10_eta, false)};
  Spectrum glasssf11eta{FromInterleaved(GlassSF11_eta, false)};

  return {{"glass-BK7", glassbk7eta},
          {"glass-BAF10", glassbaf10eta},
          {"glass-FK51A", glassfk51aeta},
          {"glass-LASF9", glasslasf9eta},
          {"glass-F5", glasssf5eta},
          {"glass-F10", glasssf10eta},
          {"glass-F11", glasssf11eta},

          {"metal-Ag-eta", ageta},
          {"metal-Ag-k", agk},
          {"metal-Al-eta", aleta},
          {"metal-Al-k", alk},
          {"metal-Au-eta", aueta},
          {"metal-Au-k", auk},
          {"metal-Cu-eta", cueta},
          {"metal-Cu-k", cuk},
          {"metal-CuZn-eta", cuzneta},
          {"metal-CuZn-k", cuznk},
          {"metal-MgO-eta", mgoeta},
          {"metal-MgO-k", mgok},
          {"metal-TiO2-eta", tio2eta},
          {"metal-TiO2-k", tio2k},

          {"stdillum-A", illuma},
          {"stdillum-D50", illumd50},
          {"stdillum-D65", illumd65},
          {"stdillum-F1", illumf1},
          {"stdillum-F2", illumf2},
          {"stdillum-F3", illumf3},
          {"stdillum-F4", illumf4},
          {"stdillum-F5", illumf5},
          {"stdillum-F6", illumf6},
          {"stdillum-F7", illumf7},
          {"stdillum-F8", illumf8},
          {"stdillum-F9", illumf9},
          {"stdillum-F10", illumf10},
          {"stdillum-F11", illumf11},
          {"stdillum-F12", illumf12},

          {"illum-acesD60", illumacesd60},

          {"canon_eos_100d_r", FromInterleaved(canon_eos_100d_r, false)},
          {"canon_eos_100d_g", FromInterleaved(canon_eos_100d_g, false)},
          {"canon_eos_100d_b", FromInterleaved(canon_eos_100d_b, false)},

          {"canon_eos_1dx_mkii_r", FromInterleaved(canon_eos_1dx_mkii_r, false)},
          {"canon_eos_1dx_mkii_g", FromInterleaved(canon_eos_1dx_mkii_g, false)},
          {"canon_eos_1dx_mkii_b", FromInterleaved(canon_eos_1dx_mkii_b, false)},

          {"canon_eos_200d_r", FromInterleaved(canon_eos_200d_r, false)},
          {"canon_eos_200d_g", FromInterleaved(canon_eos_200d_g, false)},
          {"canon_eos_200d_b", FromInterleaved(canon_eos_200d_b, false)},

          {"canon_eos_200d_mkii_r", FromInterleaved(canon_eos_200d_mkii_r, false)},
          {"canon_eos_200d_mkii_g", FromInterleaved(canon_eos_200d_mkii_g, false)},
          {"canon_eos_200d_mkii_b", FromInterleaved(canon_eos_200d_mkii_b, false)},

          {"canon_eos_5d_r", FromInterleaved(canon_eos_5d_r, false)},
          {"canon_eos_5d_g", FromInterleaved(canon_eos_5d_g, false)},
          {"canon_eos_5d_b", FromInterleaved(canon_eos_5d_b, false)},

          {"canon_eos_5d_mkii_r", FromInterleaved(canon_eos_5d_mkii_r, false)},
          {"canon_eos_5d_mkii_g", FromInterleaved(canon_eos_5d_mkii_g, false)},
          {"canon_eos_5d_mkii_b", FromInterleaved(canon_eos_5d_mkii_b, false)},

          {"canon_eos_5d_mkiii_r", FromInterleaved(canon_eos_5d_mkiii_r, false)},
          {"canon_eos_5d_mkiii_g", FromInterleaved(canon_eos_5d_mkiii_g, false)},
          {"canon_eos_5d_mkiii_b", FromInterleaved(canon_eos_5d_mkiii_b, false)},

          {"canon_eos_5d_mkiv_r", FromInterleaved(canon_eos_5d_mkiv_r, false)},
          {"canon_eos_5d_mkiv_g", FromInterleaved(canon_eos_5d_mkiv_g, false)},
          {"canon_eos_5d_mkiv_b", FromInterleaved(canon_eos_5d_mkiv_b, false)},

          {"canon_eos_5ds_r", FromInterleaved(canon_eos_5ds_r, false)},
          {"canon_eos_5ds_g", FromInterleaved(canon_eos_5ds_g, false)},
          {"canon_eos_5ds_b", FromInterleaved(canon_eos_5ds_b, false)},

          {"canon_eos_m_r", FromInterleaved(canon_eos_m_r, false)},
          {"canon_eos_m_g", FromInterleaved(canon_eos_m_g, false)},
          {"canon_eos_m_b", FromInterleaved(canon_eos_m_b, false)},

          {"hasselblad_l1d_20c_r", FromInterleaved(hasselblad_l1d_20c_r, false)},
          {"hasselblad_l1d_20c_g", FromInterleaved(hasselblad_l1d_20c_g, false)},
          {"hasselblad_l1d_20c_b", FromInterleaved(hasselblad_l1d_20c_b, false)},

          {"nikon_d810_r", FromInterleaved(nikon_d810_r, false)},
          {"nikon_d810_g", FromInterleaved(nikon_d810_g, false)},
          {"nikon_d810_b", FromInterleaved(nikon_d810_b, false)},

          {"nikon_d850_r", FromInterleaved(nikon_d850_r, false)},
          {"nikon_d850_g", FromInterleaved(nikon_d850_g, false)},
          {"nikon_d850_b", FromInterleaved(nikon_d850_b, false)},

          {"sony_ilce_6400_r", FromInterleaved(sony_ilce_6400_r, false)},
          {"sony_ilce_6400_g", FromInterleaved(sony_ilce_6400_g, false)},
          {"sony_ilce_6400_b", FromInterleaved(sony_ilce_6400_b, false)},

          {"sony_ilce_7m3_r", FromInterleaved(sony_ilce_7m3_r, false)},
          {"sony_ilce_7m3_g", FromInterleaved(sony_ilce_7m3_g, false)},
          {"sony_ilce_7m3_b", FromInterleaved(sony_ilce_7m3_b, false)},

          {"sony_ilce_7rm3_r", FromInterleaved(sony_ilce_7rm3_r, false)},
          {"sony_ilce_7rm3_g", FromInterleaved(sony_ilce_7rm3_g, false)},
          {"sony_ilce_7rm3_b", FromInterleaved(sony_ilce_7rm3_b, false)},

          {"sony_ilce_9_r", FromInterleaved(sony_ilce_9_r, false)},
          {"sony_ilce_9_g", FromInterleaved(sony_ilce_9_g, false)},
          {"sony_ilce_9_b", FromInterleaved(sony_ilce_9_b, false)}};
}

static std::map<std::string, Spectrum> namedSpectra{named_Spectra_init()};

static inline const DenselySampledSpectrum& X()
{
  return *x;
}

static inline const DenselySampledSpectrum& Y()
{
  return *y;
}

static inline const DenselySampledSpectrum& Z()
{
  return *z;
}

static DenselySampledSpectrum D(float temperature)
{
  // Convert temperature to CCT
  float cct = temperature * 1.4388f / 1.4380f;
  if (cct < 4000)
    {
      // CIE D ill-defined, use blackbody
      BlackbodySpectrum bb{cct};
      DenselySampledSpectrum blackbody =
          DenselySampledSpectrum::SampleFunction([=](float lambda) { return bb(lambda); });

      return blackbody;
    }

  // Convert CCT to xy
  float x;
  if (cct <= 7000)
    x = -4.607f * 1e9f / Utils::Math::Pow<3>(cct) + 2.9678f * 1e6f / Utils::Math::Sqr(cct)
        + 0.09911f * 1e3f / cct + 0.244063f;
  else
    x = -2.0064f * 1e9f / Utils::Math::Pow<3>(cct) + 1.9018f * 1e6f / Utils::Math::Sqr(cct)
        + 0.24748f * 1e3f / cct + 0.23704f;
  float y = -3 * x * x + 2.870f * x - 0.275f;

  // Interpolate D spectrum
  float M = 0.0241f + 0.2562f * x - 0.7341f * y;
  float M1 = (-1.3515f - 1.7703f * x + 5.9114f * y) / M;
  float M2 = (0.0300f - 31.4424f * x + 30.0717f * y) / M;

  std::vector<float> values(nCIES);
  for (int i = 0; i < nCIES; ++i)
    values[i] = (CIE_S0[i] + CIE_S1[i] * M1 + CIE_S2[i] * M2) * 0.01;

  PiecewiseLinearSpectrum dpls(CIE_S_lambda, values);
  DenselySampledSpectrum ret{Spectrum{&dpls}};
  return ret;
}

};  // namespace Spectra

//===========================================================================================================================
// RGBToSpectrumTable
//===========================================================================================================================
static constexpr int res = 64;
using CoefficientArray = float[3][res][res][res][3];

// sRGB
const int sRGBToSpectrumTable_Res{};
const std::array<float, 64> sRGBToSpectrumTable_Scale{};
const CoefficientArray sRGBToSpectrumTable_Data{};

// DCI_P3
const int DCI_P3ToSpectrumTable_Res{};
const std::array<float, 64> DCI_P3ToSpectrumTable_Scale{};
const CoefficientArray DCI_P3ToSpectrumTable_Data{};

// REC2020
const int REC2020ToSpectrumTable_Res{};
const std::array<float, 64> REC2020ToSpectrumTable_Scale{};
const CoefficientArray REC2020ToSpectrumTable_Data{};

// ACES2065_1
const int ACES2065_1ToSpectrumTable_Res{};
const std::array<float, 64> ACES2065_1ToSpectrumTable_Scale{};
const CoefficientArray ACES2065_1ToSpectrumTable_Data{};

// RGBToSpectrumTable Definition
struct RGBToSpectrumTable
{
 public:
  // RGBToSpectrumTable Public Constants

  // RGBToSpectrumTable Public Methods
  RGBToSpectrumTable(const float* zNodes, const CoefficientArray* coeffs) noexcept
      : zNodes(zNodes), coeffs(coeffs)
  {
  }

  RGBUtils::RGBSigmoidPolynomial operator()(RGBUtils::RGB rgb) const;

  static void Init();

  inline static const RGBToSpectrumTable* sRGB =
      ALLOC.new_object<RGBToSpectrumTable>(sRGBToSpectrumTable_Scale.data(),
                                           &sRGBToSpectrumTable_Data);
  inline static const RGBToSpectrumTable* DCI_P3 =
      ALLOC.new_object<RGBToSpectrumTable>(DCI_P3ToSpectrumTable_Scale.data(),
                                           &DCI_P3ToSpectrumTable_Data);
  inline static const RGBToSpectrumTable* Rec2020 =
      ALLOC.new_object<RGBToSpectrumTable>(REC2020ToSpectrumTable_Scale.data(),
                                           &REC2020ToSpectrumTable_Data);
  inline static const RGBToSpectrumTable* ACES2065_1 =
      ALLOC.new_object<RGBToSpectrumTable>(ACES2065_1ToSpectrumTable_Scale.data(),
                                           &ACES2065_1ToSpectrumTable_Data);

 private:
  // RGBToSpectrumTable Private Members
  const float* zNodes;
  const CoefficientArray* coeffs;
};

//===========================================================================================================================
// RGBColorSpace
//===========================================================================================================================

// RGBColorSpace Definition
struct RGBColorSpace
{
 public:
  // RGBColorSpace Public Methods
  RGBColorSpace(Utils::Math::Point2f r,
                Utils::Math::Point2f g,
                Utils::Math::Point2f b,
                const Spectrum& illuminant,
                const RGBToSpectrumTable* rgbToSpectrumTable);

  // RGBColorSpace Public Members
  Utils::Math::Point2f r, g, b, w;
  DenselySampledSpectrum illuminant;
  glm::mat<3, 3, float> XYZFromRGB, RGBFromXYZ;

  struct NamedColorSpace
  {
    static RGBColorSpace* noexcept_RGBColorSpace_constructor(
        Utils::Math::Point2f p1,
        Utils::Math::Point2f p2,
        Utils::Math::Point2f p3,
        SngoEngine::Core::PBRT::Spectrum::Spectrum spec,
        const RGBToSpectrumTable* table) noexcept
    {
      return ALLOC.new_object<RGBColorSpace>(p1, p2, p3, spec, table);
    }

    std::unordered_map<std::string, const std::shared_ptr<RGBColorSpace>> table;

    NamedColorSpace()
    {
      const std::shared_ptr<RGBColorSpace> sRGB{
          std::make_shared<RGBColorSpace>(Utils::Math::Point2f(.64, .33),
                                          Utils::Math::Point2f(.3, .6),
                                          Utils::Math::Point2f(.15, .06),
                                          GetNamedSpectrum("stdillum-D65"),
                                          RGBToSpectrumTable::sRGB)};
      table.emplace("srgb", sRGB);

      const std::shared_ptr<RGBColorSpace> DCI_P3{
          std::make_shared<RGBColorSpace>(Utils::Math::Point2f(.68, .32),
                                          Utils::Math::Point2f(.265, .690),
                                          Utils::Math::Point2f(.15, .06),
                                          GetNamedSpectrum("stdillum-D65"),
                                          RGBToSpectrumTable::DCI_P3)};
      table.emplace("dci-p3", DCI_P3);

      const std::shared_ptr<RGBColorSpace> Rec2020{
          std::make_shared<RGBColorSpace>(Utils::Math::Point2f(.708, .292),
                                          Utils::Math::Point2f(.170, .797),
                                          Utils::Math::Point2f(.131, .046),
                                          GetNamedSpectrum("stdillum-D65"),
                                          RGBToSpectrumTable::Rec2020)};
      table.emplace("rec2020-p3", Rec2020);

      const std::shared_ptr<RGBColorSpace> ACES2065_1{
          std::make_shared<RGBColorSpace>(Utils::Math::Point2f(.7347, .2653),
                                          Utils::Math::Point2f(0., 1.),
                                          Utils::Math::Point2f(.0001, -.077),
                                          GetNamedSpectrum("illum-acesD60"),
                                          RGBToSpectrumTable::ACES2065_1)};
      table.emplace("aces2065-1", ACES2065_1);
    }

    std::shared_ptr<RGBColorSpace> operator[](std::string n) const
    {
      std::string name;
      std::transform(n.begin(), n.end(), std::back_inserter(name), ::tolower);
      if (table.find(n) != table.end())
        return table.find(n)->second;
      else
        return nullptr;
    }

  } inline static namedColorSpace{};

  bool operator==(const RGBColorSpace& cs) const
  {
    return (r == cs.r && g == cs.g && b == cs.b && w == cs.w
            && rgbToSpectrumTable == cs.rgbToSpectrumTable);
  }

  bool operator!=(const RGBColorSpace& cs) const
  {
    return (r != cs.r || g != cs.g || b != cs.b || w != cs.w
            || rgbToSpectrumTable != cs.rgbToSpectrumTable);
  }

  [[nodiscard]] RGBUtils::RGB LuminanceVector() const
  {
    return {XYZFromRGB[1][0], XYZFromRGB[1][1], XYZFromRGB[1][2]};
  }

  [[nodiscard]] RGBUtils::RGB ToRGB(RGBUtils::XYZ xyz) const
  {
    return Utils::Math::Mul<RGBUtils::RGB>(RGBFromXYZ, xyz);
  }

  [[nodiscard]] RGBUtils::XYZ ToXYZ(RGBUtils::RGB rgb) const
  {
    return Utils::Math::Mul<RGBUtils::XYZ>(XYZFromRGB, rgb);
  }

  [[nodiscard]] static std::shared_ptr<RGBColorSpace> GetNamed(const std::string& n);
  static std::shared_ptr<RGBColorSpace> Lookup(Utils::Math::Point2f r,
                                               Utils::Math::Point2f g,
                                               Utils::Math::Point2f b,
                                               Utils::Math::Point2f w);
  [[nodiscard]] RGBUtils::RGBSigmoidPolynomial ToRGBCoeffs(RGBUtils::RGB rgb) const;

 private:
  // RGBColorSpace Private Members
  const RGBToSpectrumTable* rgbToSpectrumTable;
};

}  // namespace SngoEngine::Core::PBRT::Spectrum

#endif
