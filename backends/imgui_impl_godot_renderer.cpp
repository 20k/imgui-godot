#include "imgui_impl_godot_renderer.h"

#include <array>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/rd_attachment_format.hpp>
#include <godot_cpp/classes/rd_pipeline_color_blend_state.hpp>
#include <godot_cpp/classes/rd_pipeline_color_blend_state_attachment.hpp>
#include <godot_cpp/classes/rd_pipeline_depth_stencil_state.hpp>
#include <godot_cpp/classes/rd_pipeline_multisample_state.hpp>
#include <godot_cpp/classes/rd_pipeline_rasterization_state.hpp>
#include <godot_cpp/classes/rd_sampler_state.hpp>
#include <godot_cpp/classes/rd_shader_file.hpp>
#include <godot_cpp/classes/rd_shader_spirv.hpp>
#include <godot_cpp/classes/rd_uniform.hpp>
#include <godot_cpp/classes/rd_vertex_attribute.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "imgui.h"
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>

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

struct ImGui_ImplGodotRenderer_Data
{
    RID shader;
    int64_t vtxFormat = 0;
    RID pipeline;
    RID sampler;
    TypedArray<RID> srcBuffers;
    TypedArray<RDUniform> uniforms;
    godot::PackedColorArray clearColors;

    std::map<RID, RID> framebuffers;
    std::map<ImTextureID, RID> uniformSets;
    std::set<ImTextureID> usedTextures;

    godot::PackedInt64Array vtxOffsets;

    RID idxBuffer;
    int idxBufferSize = 0; // size in indices
    RID vtxBuffer;
    int vtxBufferSize = 0; // size in vertices

    ImGui_ImplGodotRenderer_Data()
    {
        clearColors.push_back(godot::Color(0, 0, 0, 0));
        srcBuffers.resize(3);
        vtxOffsets.resize(3);
    }
};

struct ImGui_ImplGodotRenderer_Data* ImGui_ImplGodotRenderer_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplGodotRenderer_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
};

RID make_rid(int64_t id)
{
    static_assert(sizeof(id) == sizeof(RID));

    RID rv;
    memcpy(rv._native_ptr(), &id, sizeof(id));
    return rv;
}

RID get_framebuffer(RID vprid)
{
    if (!vprid.is_valid())
        return RID();

    ImGui_ImplGodotRenderer_Data* bd = ImGui_ImplGodotRenderer_GetBackendData();

    const RenderingServer* RS = RenderingServer::get_singleton();
    RenderingDevice* RD = RS->get_rendering_device();
    auto it = bd->framebuffers.find(vprid);
    if (it != bd->framebuffers.end())
    {
        RID fb = it->second;
        if (RD->framebuffer_is_valid(fb))
            return fb;
    }

    const RID vptex = RS->texture_get_rd_texture(RS->viewport_get_texture(vprid));
    godot::TypedArray<godot::RID> arr;
    arr.push_back(vptex);
    RID fb = RD->framebuffer_create(arr);
    bd->framebuffers[vprid] = fb;
    return fb;
}


void setup_buffers(ImDrawData* drawData)
{
    RenderingDevice* RD = RenderingServer::get_singleton()->get_rendering_device();

    int globalIdxOffset = 0;
    int globalVtxOffset = 0;

    const int idxBufSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);
    godot::PackedByteArray idxBuf;
    idxBuf.resize(idxBufSize);

    const int vertBufSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
    godot::PackedByteArray vertBuf;
    vertBuf.resize(vertBufSize);

    ImGui_ImplGodotRenderer_Data* bd = ImGui_ImplGodotRenderer_GetBackendData();

    for(int i = 0; i < drawData->CmdListsCount; i++)
    {
        ImDrawList* cmdList = drawData->CmdLists[i];

        std::copy(cmdList->VtxBuffer.begin(),
                  cmdList->VtxBuffer.end(),
                  reinterpret_cast<ImDrawVert*>(vertBuf.ptrw() + globalVtxOffset));
        globalVtxOffset += cmdList->VtxBuffer.size_in_bytes();

        std::copy(cmdList->IdxBuffer.begin(),
                  cmdList->IdxBuffer.end(),
                  reinterpret_cast<ImDrawIdx*>(idxBuf.ptrw() + globalIdxOffset));
        globalIdxOffset += cmdList->IdxBuffer.size_in_bytes();

        // create a uniform set for each texture
        for(int cmdi = 0; cmdi < cmdList->CmdBuffer.Size; ++cmdi)
        {
            const ImDrawCmd& drawCmd = cmdList->CmdBuffer[cmdi];
            const ImTextureID texid = drawCmd.GetTexID();
            if(!texid)
                continue;
            const RID texrid = make_rid(texid);
            if(!RD->texture_is_valid(texrid))
                continue;

            bd->usedTextures.insert(texid);
            if(!bd->uniformSets.contains(texid))
            {
                Ref<RDUniform> uniform;
                uniform.instantiate();
                uniform->set_binding(0);
                uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE);
                uniform->add_id(bd->sampler);
                uniform->add_id(texrid);

                bd->uniforms[0] = uniform;
                bd->uniformSets[texid] = RD->uniform_set_create(bd->uniforms, bd->shader, 0);
            }
        }
    }

    RD->buffer_update(bd->idxBuffer, 0, idxBuf.size(), idxBuf);
    RD->buffer_update(bd->vtxBuffer, 0, vertBuf.size(), vertBuf);
}

