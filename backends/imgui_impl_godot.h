#pragma once

#ifndef IMGUI_DISABLE
#include "imgui.h"

bool ImGui_ImplGodot_Init();
void ImGui_ImplGodot_Shutdown();
void ImGui_ImplGodot_NewFrame();
bool ImGui_ImplGodot_ProcessEvent();

#endif
