#include "imgui_impl_godot_renderer.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/classes/atlas_texture.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/font_file.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/time.hpp>

struct ImGui_ImplRenderer_Data
{

};

struct ImGui_ImplRenderer_Data* ImGuiImplRenderer_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplRenderer_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
};
