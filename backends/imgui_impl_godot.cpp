#include "imgui_impl_godot.h"
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

using namespace godot;

struct ImGui_ImplGodot_Data
{
    CharString ClipboardTextData;
    uint64_t Time = 0;
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

static void ImGui_ImplGodot_SetClipboardText(ImGuiContext*, const char* text)
{
    ImGui_ImplGodot_Data* data = ImGui_ImplGodot_GetBackendData();

    data->ClipboardTextData = CharString(text);
}

static ImGuiViewport* ImGui_ImplGodot_GetViewportForWindowID()
{
    return ImGui::GetMainViewport();
}

//https://github.com/pkdawson/imgui-godot/blob/master/gdext/include/imgui-godot.h
ImGuiKey ImGui_ImplGodot_KeyEventToImGuiKey(Key key)
{
    switch (key)
    {
    case Key::KEY_ESCAPE:
        return ImGuiKey_Escape;
    case Key::KEY_TAB:
        return ImGuiKey_Tab;
    case Key::KEY_BACKSPACE:
        return ImGuiKey_Backspace;
    case Key::KEY_ENTER:
        return ImGuiKey_Enter;
    case Key::KEY_KP_ENTER:
        return ImGuiKey_KeypadEnter;
    case Key::KEY_INSERT:
        return ImGuiKey_Insert;
    case Key::KEY_DELETE:
        return ImGuiKey_Delete;
    case Key::KEY_PAUSE:
        return ImGuiKey_Pause;
    case Key::KEY_PRINT:
        return ImGuiKey_PrintScreen;
    case Key::KEY_HOME:
        return ImGuiKey_Home;
    case Key::KEY_END:
        return ImGuiKey_End;
    case Key::KEY_LEFT:
        return ImGuiKey_LeftArrow;
    case Key::KEY_UP:
        return ImGuiKey_UpArrow;
    case Key::KEY_RIGHT:
        return ImGuiKey_RightArrow;
    case Key::KEY_DOWN:
        return ImGuiKey_DownArrow;
    case Key::KEY_PAGEUP:
        return ImGuiKey_PageUp;
    case Key::KEY_PAGEDOWN:
        return ImGuiKey_PageDown;
    case Key::KEY_SHIFT:
        return ImGuiKey_LeftShift;
    case Key::KEY_CTRL:
        return ImGuiKey_LeftCtrl;
    case Key::KEY_META:
        return ImGuiKey_LeftSuper;
    case Key::KEY_ALT:
        return ImGuiKey_LeftAlt;
    case Key::KEY_CAPSLOCK:
        return ImGuiKey_CapsLock;
    case Key::KEY_NUMLOCK:
        return ImGuiKey_NumLock;
    case Key::KEY_SCROLLLOCK:
        return ImGuiKey_ScrollLock;
    case Key::KEY_F1:
        return ImGuiKey_F1;
    case Key::KEY_F2:
        return ImGuiKey_F2;
    case Key::KEY_F3:
        return ImGuiKey_F3;
    case Key::KEY_F4:
        return ImGuiKey_F4;
    case Key::KEY_F5:
        return ImGuiKey_F5;
    case Key::KEY_F6:
        return ImGuiKey_F6;
    case Key::KEY_F7:
        return ImGuiKey_F7;
    case Key::KEY_F8:
        return ImGuiKey_F8;
    case Key::KEY_F9:
        return ImGuiKey_F9;
    case Key::KEY_F10:
        return ImGuiKey_F10;
    case Key::KEY_F11:
        return ImGuiKey_F11;
    case Key::KEY_F12:
        return ImGuiKey_F12;
    case Key::KEY_KP_MULTIPLY:
        return ImGuiKey_KeypadMultiply;
    case Key::KEY_KP_DIVIDE:
        return ImGuiKey_KeypadDivide;
    case Key::KEY_KP_SUBTRACT:
        return ImGuiKey_KeypadSubtract;
    case Key::KEY_KP_PERIOD:
        return ImGuiKey_KeypadDecimal;
    case Key::KEY_KP_ADD:
        return ImGuiKey_KeypadAdd;
    case Key::KEY_KP_0:
        return ImGuiKey_Keypad0;
    case Key::KEY_KP_1:
        return ImGuiKey_Keypad1;
    case Key::KEY_KP_2:
        return ImGuiKey_Keypad2;
    case Key::KEY_KP_3:
        return ImGuiKey_Keypad3;
    case Key::KEY_KP_4:
        return ImGuiKey_Keypad4;
    case Key::KEY_KP_5:
        return ImGuiKey_Keypad5;
    case Key::KEY_KP_6:
        return ImGuiKey_Keypad6;
    case Key::KEY_KP_7:
        return ImGuiKey_Keypad7;
    case Key::KEY_KP_8:
        return ImGuiKey_Keypad8;
    case Key::KEY_KP_9:
        return ImGuiKey_Keypad9;
    case Key::KEY_MENU:
        return ImGuiKey_Menu;
    case Key::KEY_SPACE:
        return ImGuiKey_Space;
    case Key::KEY_APOSTROPHE:
        return ImGuiKey_Apostrophe;
    case Key::KEY_COMMA:
        return ImGuiKey_Comma;
    case Key::KEY_MINUS:
        return ImGuiKey_Minus;
    case Key::KEY_PERIOD:
        return ImGuiKey_Period;
    case Key::KEY_SLASH:
        return ImGuiKey_Slash;
    case Key::KEY_0:
        return ImGuiKey_0;
    case Key::KEY_1:
        return ImGuiKey_1;
    case Key::KEY_2:
        return ImGuiKey_2;
    case Key::KEY_3:
        return ImGuiKey_3;
    case Key::KEY_4:
        return ImGuiKey_4;
    case Key::KEY_5:
        return ImGuiKey_5;
    case Key::KEY_6:
        return ImGuiKey_6;
    case Key::KEY_7:
        return ImGuiKey_7;
    case Key::KEY_8:
        return ImGuiKey_8;
    case Key::KEY_9:
        return ImGuiKey_9;
    case Key::KEY_SEMICOLON:
        return ImGuiKey_Semicolon;
    case Key::KEY_EQUAL:
        return ImGuiKey_Equal;
    case Key::KEY_A:
        return ImGuiKey_A;
    case Key::KEY_B:
        return ImGuiKey_B;
    case Key::KEY_C:
        return ImGuiKey_C;
    case Key::KEY_D:
        return ImGuiKey_D;
    case Key::KEY_E:
        return ImGuiKey_E;
    case Key::KEY_F:
        return ImGuiKey_F;
    case Key::KEY_G:
        return ImGuiKey_G;
    case Key::KEY_H:
        return ImGuiKey_H;
    case Key::KEY_I:
        return ImGuiKey_I;
    case Key::KEY_J:
        return ImGuiKey_J;
    case Key::KEY_K:
        return ImGuiKey_K;
    case Key::KEY_L:
        return ImGuiKey_L;
    case Key::KEY_M:
        return ImGuiKey_M;
    case Key::KEY_N:
        return ImGuiKey_N;
    case Key::KEY_O:
        return ImGuiKey_O;
    case Key::KEY_P:
        return ImGuiKey_P;
    case Key::KEY_Q:
        return ImGuiKey_Q;
    case Key::KEY_R:
        return ImGuiKey_R;
    case Key::KEY_S:
        return ImGuiKey_S;
    case Key::KEY_T:
        return ImGuiKey_T;
    case Key::KEY_U:
        return ImGuiKey_U;
    case Key::KEY_V:
        return ImGuiKey_V;
    case Key::KEY_W:
        return ImGuiKey_W;
    case Key::KEY_X:
        return ImGuiKey_X;
    case Key::KEY_Y:
        return ImGuiKey_Y;
    case Key::KEY_Z:
        return ImGuiKey_Z;
    case Key::KEY_BRACKETLEFT:
        return ImGuiKey_LeftBracket;
    case Key::KEY_BACKSLASH:
        return ImGuiKey_Backslash;
    case Key::KEY_BRACKETRIGHT:
        return ImGuiKey_RightBracket;
    case Key::KEY_QUOTELEFT:
        return ImGuiKey_GraveAccent;
    default:
        return ImGuiKey_None;
    };
}

ImGuiKey ImGui_ImplGodot_JoyButtonToImGuiKey(JoyButton btn)
{
    switch (btn)
    {
    case JoyButton::JOY_BUTTON_A:
        return ImGuiKey_GamepadFaceDown;
    case JoyButton::JOY_BUTTON_B:
        return ImGuiKey_GamepadFaceRight;
    case JoyButton::JOY_BUTTON_X:
        return ImGuiKey_GamepadFaceLeft;
    case JoyButton::JOY_BUTTON_Y:
        return ImGuiKey_GamepadFaceUp;
    case JoyButton::JOY_BUTTON_BACK:
        return ImGuiKey_GamepadBack;
    case JoyButton::JOY_BUTTON_START:
        return ImGuiKey_GamepadStart;
    case JoyButton::JOY_BUTTON_LEFT_STICK:
        return ImGuiKey_GamepadL3;
    case JoyButton::JOY_BUTTON_RIGHT_STICK:
        return ImGuiKey_GamepadR3;
    case JoyButton::JOY_BUTTON_LEFT_SHOULDER:
        return ImGuiKey_GamepadL1;
    case JoyButton::JOY_BUTTON_RIGHT_SHOULDER:
        return ImGuiKey_GamepadR1;
    case JoyButton::JOY_BUTTON_DPAD_UP:
        return ImGuiKey_GamepadDpadUp;
    case JoyButton::JOY_BUTTON_DPAD_DOWN:
        return ImGuiKey_GamepadDpadDown;
    case JoyButton::JOY_BUTTON_DPAD_LEFT:
        return ImGuiKey_GamepadDpadLeft;
    case JoyButton::JOY_BUTTON_DPAD_RIGHT:
        return ImGuiKey_GamepadDpadRight;
    default:
        return ImGuiKey_None;
    };
}

static void ImGui_ImplGodot_UpdateKeyModifiers(const Ref<InputEventKey>& evt)
{
    ImGuiIO& io = ImGui::GetIO();

    Key kc = evt->get_keycode();

    if(kc == Key::KEY_CTRL)
        io.AddKeyEvent(ImGuiMod_Ctrl, evt->is_pressed());
    else if(kc == Key::KEY_SHIFT)
        io.AddKeyEvent(ImGuiMod_Shift, evt->is_pressed());
    else if(kc == Key::KEY_ALT)
        io.AddKeyEvent(ImGuiMod_Alt, evt->is_pressed());
    else if(kc == Key::KEY_META)
        io.AddKeyEvent(ImGuiMod_Super, evt->is_pressed());
}

bool ImGui_ImplGodot_ProcessEvent(const Ref<InputEvent>& event)
{
    ImGui_ImplGodot_Data* bd = ImGui_ImplGodot_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplGodot_Init()?");

    ImGuiIO& io = ImGui::GetIO();

    //ignore mice events because godot has no way to get the global coordinate
    if(Ref<InputEventMouseMotion> mouse_motion = event; mouse_motion.is_valid())
    {
        return true;
    }
    else if(Ref<InputEventMouseButton> mouse_button = event; mouse_button.is_valid())
    {
        MouseButton mb = mouse_button->get_button_index();

        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);

        if(mb == MOUSE_BUTTON_WHEEL_UP)
            io.AddMouseWheelEvent(0.f, mouse_button->get_factor());
        else if(mb == MOUSE_BUTTON_WHEEL_DOWN)
            io.AddMouseWheelEvent(0.f, -mouse_button->get_factor());
        else if(mb == MOUSE_BUTTON_WHEEL_LEFT)
            io.AddMouseWheelEvent(mouse_button->get_factor(), 0.f);
        else if(mb == MOUSE_BUTTON_WHEEL_RIGHT)
            io.AddMouseWheelEvent(-mouse_button->get_factor(), 0.f);

        else if(mb == MOUSE_BUTTON_LEFT)
            io.AddMouseButtonEvent(0, mouse_button->is_pressed());
        else if(mb == MOUSE_BUTTON_RIGHT)
            io.AddMouseButtonEvent(1, mouse_button->is_pressed());
        else if(mb == MOUSE_BUTTON_MIDDLE)
            io.AddMouseButtonEvent(2, mouse_button->is_pressed());
        else if(mb == MOUSE_BUTTON_XBUTTON1)
            io.AddMouseButtonEvent(3, mouse_button->is_pressed());
        else if(mb == MOUSE_BUTTON_XBUTTON2)
            io.AddMouseButtonEvent(4, mouse_button->is_pressed());

        return true;
    }
    else if(Ref<InputEventKey> input_key; input_key.is_valid())
    {
        ImGui_ImplGodot_UpdateKeyModifiers(input_key);

        ImGuiKey key = ImGui_ImplGodot_KeyEventToImGuiKey(input_key->get_keycode());
        bool pressed = input_key->is_pressed();
        uint32_t unicode = input_key->get_unicode();

        if(key != ImGuiKey_None)
            io.AddKeyEvent(key, pressed);

        if(pressed && unicode != 0)
            io.AddInputCharacterUTF16(unicode);

        return true;
    }

    return false;
}

bool ImGui_ImplGodot_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");

    ImGui_ImplGodot_Data* bd = IM_NEW(ImGui_ImplGodot_Data)();

    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = "godot";

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Platform_SetClipboardTextFn = ImGui_ImplGodot_SetClipboardText;
    platform_io.Platform_GetClipboardTextFn = ImGui_ImplGodot_GetClipboardText;

    bd->Time = Time::get_singleton()->get_ticks_usec();
    return true;
}

void ImGui_ImplGodot_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGodot_Data* bd = ImGui_ImplGodot_GetBackendData();

    uint64_t current_time = Time::get_singleton()->get_ticks_usec();

    //add 1ms
    if(current_time <= bd->Time)
        current_time = bd->Time + 1000;

    io.DeltaTime = bd->Time > 0 ? (float)((double)(current_time - bd->Time) / (1000. * 1000.)) : 1.f/60.f;
    bd->Time = current_time;
}

struct Godot_ViewportData
{
    Window* window = nullptr;
};

static void Godot_CreateWindow(ImGuiViewport* vp)
{
    Godot_ViewportData* vd = IM_NEW(Godot_ViewportData);
    vp->PlatformUserData = vd;

    {
        Godot_ViewportData* mainvd = (Godot_ViewportData*)ImGui::GetMainViewport()->PlatformUserData;
        Window* mainWindow = mainvd->window;
        if (mainWindow->is_embedding_subwindows())
        {
            if (ProjectSettings::get_singleton()->get("display/window/subwindows/embed_subwindows"))
            {
                UtilityFunctions::push_warning(
                    "ImGui Viewports: 'display/window/subwindows/embed_subwindows' needs to be disabled");
            }
            mainWindow->set_embedding_subwindows(false);
        }
    }

    const Rect2i winRect = Rect2i(vp->Pos, vp->Size);

    ImGuiWindow* igwin = memnew(ImGuiWindow);
    igwin->init(vp);
    vd->window = Object::cast_to<Window>(igwin);
    vd->window->set_flag(Window::FLAG_BORDERLESS, true);
    vd->window->set_position(winRect.position);
    vd->window->set_size(winRect.size);
    vd->window->set_transparent_background(true);
    vd->window->set_flag(Window::FLAG_TRANSPARENT, true);
    vd->window->set_flag(Window::FLAG_ALWAYS_ON_TOP, vp->Flags & ImGuiViewportFlags_TopMost);
    vd->window->set_flag(Window::FLAG_NO_FOCUS, vp->Flags & ImGuiViewportFlags_NoFocusOnClick);

    Node* root = Object::cast_to<Node>(Engine::get_singleton()->get_singleton("ImGuiController"));
    root->add_child(vd->window);

    // need to do this after add_child
    vd->window->set_flag(Window::FLAG_TRANSPARENT, true);

    // it's our window, so just draw directly to the root viewport
    const RID vprid = vd->window->get_viewport_rid();
    vp->RendererUserData = (void*)vprid.get_id();

    int32_t windowID = vd->window->get_window_id();
    vp->PlatformHandle = (void*)(int64_t)windowID;
    DisplayServer::get_singleton()->window_set_input_event_callback(
        Callable(ImGuiController::Instance(), "window_input_callback"),
        windowID);

    RenderingServer* RS = RenderingServer::get_singleton();
    ImGui::Godot::GetContext()->renderer->InitViewport(vprid);
    RS->viewport_set_transparent_background(vprid, true);
}

static void Godot_DestroyWindow(ImGuiViewport* vp)
{
    Godot_ViewportData* vd = (Godot_ViewportData*)vp->PlatformUserData;
    if (vd)
    {
        if (vd->window && vp != ImGui::GetMainViewport())
        {
            vd->window->get_parent()->remove_child(vd->window);
            memdelete(vd->window);
        }
        IM_DELETE(vd);
        vp->PlatformUserData = nullptr;
        vp->RendererUserData = nullptr;
    }
}

static void Godot_ShowWindow(ImGuiViewport* vp)
{
    Godot_ViewportData* vd = (Godot_ViewportData*)vp->PlatformUserData;
    vd->window->show();
}

static void Godot_SetWindowPos(ImGuiViewport* vp, ImVec2 pos)
{
    Godot_ViewportData* vd = (Godot_ViewportData*)vp->PlatformUserData;
    vd->window->set_position(pos);
}

ImVec2 Godot_GetWindowPos(ImGuiViewport* vp)
{
    Godot_ViewportData* vd = (Godot_ViewportData*)vp->PlatformUserData;
    return vd->window->get_position();
}

void Godot_SetWindowSize(ImGuiViewport* vp, ImVec2 size)
{
    Godot_ViewportData* vd = (Godot_ViewportData*)vp->PlatformUserData;
    vd->window->set_size(size);
}

ImVec2 Godot_GetWindowSize(ImGuiViewport* vp)
{
    Godot_ViewportData* vd = (Godot_ViewportData*)vp->PlatformUserData;
    return vd->window->get_size();
}

void Godot_SetWindowFocus(ImGuiViewport* vp)
{
    Godot_ViewportData* vd = (Godot_ViewportData*)vp->PlatformUserData;
    vd->window->grab_focus();
}

bool Godot_GetWindowFocus(ImGuiViewport* vp)
{
    Godot_ViewportData* vd = (Godot_ViewportData*)vp->PlatformUserData;
    return vd->window->has_focus();
}

bool Godot_GetWindowMinimized(ImGuiViewport* vp)
{
    Godot_ViewportData* vd = (Godot_ViewportData*)vp->PlatformUserData;
    return vd->window->get_mode() & Window::MODE_MINIMIZED;
}

void Godot_SetWindowTitle(ImGuiViewport* vp, const char* str)
{
    Godot_ViewportData* vd = (Godot_ViewportData*)vp->PlatformUserData;
    vd->window->set_title(String(str));
}

void ImGui_ImplGodot_InitPlatformInterface()
{
    auto& pio = ImGui::GetPlatformIO();
    pio.Platform_CreateWindow = Godot_CreateWindow;
    pio.Platform_DestroyWindow = Godot_DestroyWindow;
    pio.Platform_ShowWindow = Godot_ShowWindow;
    pio.Platform_SetWindowPos = Godot_SetWindowPos;
    pio.Platform_GetWindowPos = Godot_GetWindowPos;
    pio.Platform_SetWindowSize = Godot_SetWindowSize;
    pio.Platform_GetWindowSize = Godot_GetWindowSize;
    pio.Platform_SetWindowFocus = Godot_SetWindowFocus;
    pio.Platform_GetWindowFocus = Godot_GetWindowFocus;
    pio.Platform_GetWindowMinimized = Godot_GetWindowMinimized;
    pio.Platform_SetWindowTitle = Godot_SetWindowTitle;
}

void update_monitors()
{
    auto& pio = ImGui::GetPlatformIO();
    const DisplayServer* DS = DisplayServer::get_singleton();
    const int screenCount = DS->get_screen_count();

    pio.Monitors.resize(0);
    for (int i = 0; i < screenCount; ++i)
    {
        ImGuiPlatformMonitor monitor;
        monitor.MainPos = DS->screen_get_position(i);
        monitor.MainSize = DS->screen_get_size(i);
        monitor.DpiScale = DS->screen_get_scale(i);

        const Rect2i rect = DS->screen_get_usable_rect(i);
        monitor.WorkPos = rect.position;
        monitor.WorkSize = rect.size;

        pio.Monitors.push_back(monitor);
    }

    // fix headless
    if (pio.Monitors.size() == 0)
    {
        ImGuiPlatformMonitor monitor;
        monitor.MainPos = {0, 0};
        monitor.MainSize = {640, 480};
        monitor.DpiScale = 1.0f;

        monitor.WorkPos = {0, 0};
        monitor.WorkSize = {640, 480};

        pio.Monitors.push_back(monitor);
    }
}
