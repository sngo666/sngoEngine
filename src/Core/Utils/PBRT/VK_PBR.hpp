#ifndef __VK_PBR_H
#define __VK_PBR_H

#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <utility>

namespace SngoEngine::Core::PBR
{
//===========================================================================================================================
// MaterialData
//===========================================================================================================================

struct MaterialData
{
  struct
  {
    float roughness;
    float metallic;
    float r, g, b;
  } data{};

  std::string name;

  MaterialData() = default;
  MaterialData(std::string n, glm::vec3 c, float r, float m) : name(std::move(n))
  {
    data.roughness = r;
    data.metallic = m;
    data.r = c.r;
    data.g = c.g;
    data.b = c.b;
  };
  MaterialData(std::string& n, glm::vec3 c, float r, float m) : name(n)
  {
    data.roughness = r;
    data.metallic = m;
    data.r = c.r;
    data.g = c.g;
    data.b = c.b;
  };
};

//===========================================================================================================================
// MaterialDictionary
//===========================================================================================================================

struct MaterialDictionary
{
  std::vector<MaterialData> dictionary{};

  explicit MaterialDictionary()
  {
    // default materials
    dictionary.emplace_back("Gold", glm::vec3(1.0f, 0.765557f, 0.336057f), 0.1f, 1.0f);
    dictionary.emplace_back("Copper", glm::vec3(0.955008f, 0.637427f, 0.538163f), 0.1f, 1.0f);
    dictionary.emplace_back("Chromium", glm::vec3(0.549585f, 0.556114f, 0.554256f), 0.1f, 1.0f);
    dictionary.emplace_back("Nickel", glm::vec3(0.659777f, 0.608679f, 0.525649f), 0.1f, 1.0f);
    dictionary.emplace_back("Titanium", glm::vec3(0.541931f, 0.496791f, 0.449419f), 0.1f, 1.0f);
    dictionary.emplace_back("Cobalt", glm::vec3(0.662124f, 0.654864f, 0.633732f), 0.1f, 1.0f);
    dictionary.emplace_back("Platinum", glm::vec3(0.672411f, 0.637331f, 0.585456f), 0.1f, 1.0f);

    // Testing materials
    dictionary.emplace_back("White", glm::vec3(1.0f), 0.1f, 1.0f);
    dictionary.emplace_back("Red", glm::vec3(1.0f, 0.0f, 0.0f), 0.1f, 1.0f);
    dictionary.emplace_back("Blue", glm::vec3(0.0f, 0.0f, 1.0f), 0.1f, 1.0f);
    dictionary.emplace_back("Black", glm::vec3(0.0f), 0.1f, 1.0f);
  }

  MaterialData operator[](size_t n)
  {
    assert(n < dictionary.size());
    return dictionary[n];
  }

  template <typename... Args>
  MaterialData add(Args... args)
  {
    dictionary.emplace_back(args...);
  }
};

}  // namespace SngoEngine::Core::PBR
#endif
