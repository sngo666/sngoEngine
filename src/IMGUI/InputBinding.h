#ifndef __SNGO_INPUT_H
#define __SNGO_INPUT_H

#include <functional>
#include <utility>

#include "src/IMGUI/include/imgui.h"
#include "type_traits"

using namespace std::placeholders;

namespace SngoEngine::Imgui
{

enum KeyBoard_Event
{
  PRESSED = 0x00000001,
  RELEASED = 0x00000002,
  DOWN = 0x00000004,
  UP = 0x00000008
};

template <typename Fn, typename... Args>
  requires std::is_invocable_v<Fn, Args...>
constexpr void Make_keyInputEvent(ImGuiKey _key, KeyBoard_Event _event, Fn _fn, Args... args)
{
  switch (_event)
    {
        case PRESSED: {
          if (ImGui::IsKeyPressed(_key))
            _fn(args...);
          break;
        }
        case RELEASED: {
          if (ImGui::IsKeyReleased(_key))
            _fn(args...);
          break;
        }
        case DOWN: {
          if (ImGui::IsKeyDown(_key))
            _fn(args...);
          break;
        }
        default: {
          _fn(args...);
        }
        break;
    }
}

}  // namespace SngoEngine::Imgui
#endif