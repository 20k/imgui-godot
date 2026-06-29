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
#include <string>

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
#include <godot_cpp/classes/rd_shader_source.hpp>
#include <godot_cpp/classes/rd_shader_spirv.hpp>

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

const char shader[] =
{
    #embed "godot/shader.glsl"
};

bool render_init()
{
    RenderingDevice* rd = RenderingServer::get_singleton()->get_rendering_device();

    if(!rd)
        return false;

    Ref<RDShaderSource> shader_source = memnew(RDShaderSource);
    shader_source->set_language(RenderingDevice::SHADER_LANGUAGE_GLSL);
    shader_source->set_stage_source(RenderingDevice::SHADER_STAGE_COMPUTE, shader);

    Ref<RDShaderSPIRV> shader_spirv = rd->shader_compile_spirv_from_source(shader_source);

    std::string err = shader_spirv->get_stage_compile_error(RenderingDevice::SHADER_STAGE_COMPUTE).utf8().get_data();

    if(err != "")
    {
        printf("Error %s\n", err.c_str());
        return false;
    }

    ImGui_ImplGodotRenderer_Data* bd = ImGui_ImplGodotRenderer_GetBackendData();

    bd->shader = rd->shader_create_from_spirv(shader_spirv);

    if(!bd->shader.is_valid())
        return false;

    TypedArray<RDVertexAttribute> vattrs;

    RDVertexAttribute attr_points;
    attr_points.set_location(0);
    attr_points.set_format(RenderingDevice::DATA_FORMAT_R32G32_SFLOAT);
    attr_points.set_stride(sizeof(ImDrawVert));
    attr_points.set_offset(0);

    RDVertexAttribute attr_uvs;
    attr_uvs.set_location(1);
    attr_uvs.set_format(RenderingDevice::DATA_FORMAT_R32G32_SFLOAT);
    attr_uvs.set_stride(sizeof(ImDrawVert));
    attr_uvs.set_offset(sizeof(float) * 2);

    RDVertexAttribute attr_colors;
    attr_colors.set_location(2);
    attr_colors.set_format(RenderingDevice::DATA_FORMAT_R8G8B8A8_UNORM);
    attr_colors.set_stride(sizeof(ImDrawVert));
    attr_colors.set_offset(sizeof(float) * 4);

    vattrs.append(&attr_points);
    vattrs.append(&attr_uvs);
    vattrs.append(&attr_colors);

    bd->vtxFormat = rd->vertex_format_create(vattrs);

    RDPipelineColorBlendStateAttachment bsa;
    bsa.set_enable_blend(true);
    bsa.set_src_color_blend_factor(RenderingDevice::BLEND_FACTOR_SRC_ALPHA);
    bsa.set_dst_color_blend_factor(RenderingDevice::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    bsa.set_color_blend_op(RenderingDevice::BLEND_OP_ADD);
    bsa.set_src_alpha_blend_factor(RenderingDevice::BLEND_FACTOR_ONE);
    bsa.set_dst_alpha_blend_factor(RenderingDevice::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    bsa.set_alpha_blend_op(RenderingDevice::BLEND_OP_ADD);

    Ref<RDPipelineColorBlendState> blend;
    blend.instantiate();
    blend->set_blend_constant(Color(0.0f, 0.0f, 0.0f, 0.0f));
    blend->get_attachments().append(&bsa);

    Ref<RDPipelineRasterizationState> raster_state;
    raster_state.instantiate();
    raster_state->set_front_face(RenderingDevice::POLYGON_FRONT_FACE_COUNTER_CLOCKWISE);

    Ref<RDAttachmentFormat> af;
    af.instantiate();
    af->set_format(RenderingDevice::DATA_FORMAT_R8G8B8A8_UNORM);
    af->set_samples(RenderingDevice::TEXTURE_SAMPLES_1);
    af->set_usage_flags(RenderingDevice::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT);

    TypedArray<RDAttachmentFormat> afs;
    afs.push_back(af);

    const int64_t fb_format = rd->framebuffer_format_create(afs);

    bd->pipeline = rd->render_pipeline_create(bd->shader,
                                              fb_format,
                                              bd->vtxFormat,
                                              RenderingDevice::RENDER_PRIMITIVE_TRIANGLES,
                                              raster_state,
                                              {},
                                              {},
                                              blend);

    if(!bd->pipeline.is_valid())
        return false;

    RDSamplerState sampler_state;
    sampler_state.set_min_filter(RenderingDevice::SAMPLER_FILTER_LINEAR);
    sampler_state.set_mag_filter(RenderingDevice::SAMPLER_FILTER_LINEAR);
    sampler_state.set_mip_filter(RenderingDevice::SAMPLER_FILTER_LINEAR);
    sampler_state.set_repeat_u(RenderingDevice::SAMPLER_REPEAT_MODE_REPEAT);
    sampler_state.set_repeat_v(RenderingDevice::SAMPLER_REPEAT_MODE_REPEAT);
    sampler_state.set_repeat_w(RenderingDevice::SAMPLER_REPEAT_MODE_REPEAT);

    bd->sampler = rd->sampler_create(&sampler_state);

    bd->srcBuffers.resize(3);
    bd->uniforms.resize(1);
    return true;
}


void render_viewport(RID fb, ImDrawData* drawData)
{
    RenderingDevice* rd = RenderingServer::get_singleton()->get_rendering_device();

    if(!fb.is_valid())
        return;

    ImGui_ImplGodotRenderer_Data* bd = ImGui_ImplGodotRenderer_GetBackendData();

    godot::PackedFloat32Array pcfloats;
    pcfloats.resize(4);
    pcfloats[0] = 2.0f / drawData->DisplaySize.x;
    pcfloats[1] = 2.0f / drawData->DisplaySize.y;
    pcfloats[2] = -1.0f - (drawData->DisplayPos.x * pcfloats[0]);
    pcfloats[3] = -1.0f - (drawData->DisplayPos.y * pcfloats[1]);
    godot::PackedByteArray pcbuf = pcfloats.to_byte_array();

    // allocate merged index and vertex buffers
    if(bd->idxBufferSize < drawData->TotalIdxCount)
    {
        if (bd->idxBuffer.get_id() != 0)
            rd->free_rid(bd->idxBuffer);
        bd->idxBuffer = rd->index_buffer_create(drawData->TotalIdxCount, RenderingDevice::INDEX_BUFFER_FORMAT_UINT16);
        bd->idxBufferSize = drawData->TotalIdxCount;
    }

    if(bd->vtxBufferSize < drawData->TotalVtxCount)
    {
        if(bd->vtxBuffer.get_id() != 0)
            rd->free_rid(bd->vtxBuffer);
        bd->vtxBuffer = rd->vertex_buffer_create(drawData->TotalVtxCount * sizeof(ImDrawVert));
        bd->vtxBufferSize = drawData->TotalVtxCount;
    }

    // check if our font texture is still valid
    std::erase_if(bd->uniformSets, [rd](const auto& kv) { return !rd->uniform_set_is_valid(kv.second); });

    if(drawData->CmdListsCount > 0)
        setup_buffers(drawData);

    // draw
    const int64_t dl = rd->draw_list_begin(fb,
                                           RenderingDevice::DRAW_DEFAULT_ALL,
                                           bd->clearColors);

    rd->draw_list_bind_render_pipeline(dl, bd->pipeline);
    rd->draw_list_set_push_constant(dl, pcbuf, pcbuf.size());

    int globalIdxOffset = 0;
    int globalVtxOffset = 0;
    for(int i = 0; i < drawData->CmdListsCount; ++i)
    {
        ImDrawList* cmdList = drawData->CmdLists[i];

        for(int cmdi = 0; cmdi < cmdList->CmdBuffer.Size; ++cmdi)
        {
            const ImDrawCmd& drawCmd = cmdList->CmdBuffer[cmdi];
            if(drawCmd.ElemCount == 0)
                continue;
            if(!bd->uniformSets.contains(drawCmd.GetTexID()))
                continue;

            const RID idxArray =
                rd->index_array_create(bd->idxBuffer, drawCmd.IdxOffset + globalIdxOffset, drawCmd.ElemCount);

            const int64_t voff = (drawCmd.VtxOffset + globalVtxOffset) * sizeof(ImDrawVert);
            bd->srcBuffers[0] = bd->srcBuffers[1] = bd->srcBuffers[2] = bd->vtxBuffer;
            bd->vtxOffsets[0] = bd->vtxOffsets[1] = bd->vtxOffsets[2] = voff;
            const RID vtxArray =
                rd->vertex_array_create(cmdList->VtxBuffer.Size, bd->vtxFormat, bd->srcBuffers, bd->vtxOffsets);

            rd->draw_list_bind_uniform_set(dl, bd->uniformSets[drawCmd.GetTexID()], 0);
            rd->draw_list_bind_index_array(dl, idxArray);
            rd->draw_list_bind_vertex_array(dl, vtxArray);

            godot::Rect2 clipRect = {drawCmd.ClipRect.x,
                                     drawCmd.ClipRect.y,
                                     drawCmd.ClipRect.z - drawCmd.ClipRect.x,
                                     drawCmd.ClipRect.w - drawCmd.ClipRect.y};
            clipRect.position -= godot::Vector2i(drawData->DisplayPos.x, drawData->DisplayPos.y);
            rd->draw_list_enable_scissor(dl, clipRect);

            rd->draw_list_draw(dl, true, 1);

            rd->free_rid(idxArray);
            rd->free_rid(vtxArray);
        }
        globalIdxOffset += cmdList->IdxBuffer.Size;
        globalVtxOffset += cmdList->VtxBuffer.Size;
    }
    rd->draw_list_end();
}

void render()
{

}
