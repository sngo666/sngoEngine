#ifndef __SNGO_TAGGEDPOINTER_H
#define __SNGO_TAGGEDPOINTER_H

#include <cstdint>
#include <type_traits>

#include "CHECK.hpp"
#include "Utils.hpp"

namespace SngoEngine::Core::PBRT::TaggedPointer
{

template <typename... Ts>
class TaggedPointer
{
 public:
  // TaggedPointer Public Types
  using Types = Utils::TypePack<Ts...>;

  // TaggedPointer Public Methods
  TaggedPointer() = default;
  template <typename T>
  TaggedPointer(T* ptr)
  {
    auto iptr = reinterpret_cast<uint64_t>(ptr);
    assert((iptr & ptrMask) == iptr);
    constexpr unsigned int type = TypeIndex<T>();
    bits = iptr | ((uint64_t)type << tagShift);
  }

  explicit TaggedPointer(std::nullptr_t np) {}

  TaggedPointer(const TaggedPointer& t)
  {
    bits = t.bits;
  }

  TaggedPointer& operator=(const TaggedPointer& t)
  {
    assert(&t == this);

    bits = t.bits;
    return *this;
  }

  template <typename T>
  static constexpr unsigned int TypeIndex()
  {
    using Tp = typename std::remove_cv_t<T>;
    if constexpr (std::is_same_v<Tp, std::nullptr_t>)
      return 0;
    else
      return 1 + Utils::IndexOf<Tp, Types>::count;
  }

  [[nodiscard]] unsigned int Tag() const
  {
    return ((bits & tagMask) >> tagShift);
  }
  template <typename T>
  [[nodiscard]] bool Is() const
  {
    return Tag() == TypeIndex<T>();
  }

  static constexpr unsigned int MaxTag()
  {
    return sizeof...(Ts);
  }

  static constexpr unsigned int NumTags()
  {
    return MaxTag() + 1;
  }

  explicit operator bool() const
  {
    return (bits & ptrMask) != 0;
  }

  bool operator<(const TaggedPointer& tp) const
  {
    return bits < tp.bits;
  }

  template <typename T>
  T* Cast()
  {
    DCHECK(Is<T>());
    return reinterpret_cast<T*>(ptr());
  }

  template <typename T>
  const T* Cast() const
  {
    DCHECK(Is<T>());
    return reinterpret_cast<const T*>(ptr());
  }

  template <typename T>
  T* CastOrNullptr()
  {
    if (Is<T>())
      return reinterpret_cast<T*>(ptr());
    else
      return nullptr;
  }

  template <typename T>
  const T* CastOrNullptr() const
  {
    if (Is<T>())
      return reinterpret_cast<const T*>(ptr());
    else
      return nullptr;
  }

  bool operator==(const TaggedPointer& tp) const
  {
    return bits == tp.bits;
  }

  bool operator!=(const TaggedPointer& tp) const
  {
    return bits != tp.bits;
  }

  void* ptr()
  {
    return (void*)(bits & ptrMask);
  }

  [[nodiscard]] const void* ptr() const
  {
    return (const void*)(bits & ptrMask);
  }

  template <typename F>
  decltype(auto) Dispatch(F&& func)
  {
    using R = typename Utils::ReturnType<F, Ts...>::type;
    return Utils::Dispatch<F, R, Ts...>(func, ptr(), Tag() - 1);
  }

  template <typename F>
  decltype(auto) Dispatch(F&& func) const
  {
    using R = typename Utils::ReturnTypeConst<F, Ts...>::type;
    return Utils::Dispatch<F, R, Ts...>(func, ptr(), Tag() - 1);
  }

 private:
  static_assert(sizeof(uintptr_t) <= sizeof(uint64_t), "Expected pointer size to be <= 64 bits");
  // TaggedPointer Private Members
  static constexpr int tagShift = 57;
  static constexpr int tagBits = 64 - tagShift;
  static constexpr uint64_t tagMask = ((1ull << tagBits) - 1) << tagShift;
  static constexpr uint64_t ptrMask = ~tagMask;
  uint64_t bits = 0;
};
}  // namespace SngoEngine::Core::PBRT::TaggedPointer

#endif