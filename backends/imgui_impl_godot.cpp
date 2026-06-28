#include "imgui_impl_godot.h"
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

struct ImGui_ImplGodot_Data
{
    CharString ClipboardTextData;
};

static ImGui_ImplGodot_Data* ImGui_ImplGodot_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplGodot_Data*)ImGui::GetIO().BackendPlatformUserData : nullptr;
}

static const char* ImGui_ImplGodot_GetClipboardText(ImGuiContext*)
{
    DisplayServer* server = DisplayServer::get_singleton();

    ImGui_ImplGodot_Data* data = ImGui_ImplGodot_GetBackendData();

    if(server->clipboard_has())
        data->ClipboardTextData = server->clipboard_get().utf8();
    else
        data->ClipboardTextData = CharString();

    return data->ClipboardTextData.get_data();
}

static void ImGui_ImplGodot_SetClipboardText(const char* text)
{
    ImGui_ImplGodot_Data* data = ImGui_ImplGodot_GetBackendData();

    data->ClipboardTextData = CharString(text);
}
