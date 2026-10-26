#include "../main.h"
#include "imguiwrapper.h"
#include "uisettings.h"
#include "../game/util.h"

#include "../game/game.h"
#include "../net/netgame.h"

// voice
#include "../voice_new/MicroIcon.h"
#include "../voice_new/SpeakerList.h"
#include "../voice_new/include/util/Render.h"


extern UI* pUI;
extern CNetGame* pNetGame;


ImGuiWrapper::ImGuiWrapper(const ImVec2& display_size, const std::string& font_path)
{
	m_displaySize = display_size;
	m_renderer = 0;
	m_fontRaster = nullptr;
	m_fontPath = font_path;

	m_vertexBuffer = nullptr;
	m_vertexBufferSize = 10000;
}

ImGuiWrapper::~ImGuiWrapper()
{
	shutdown();
}

bool ImGuiWrapper::initialize()
{
	Log::traceLastFunc("ImGuiWrapper::initialize");

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = m_displaySize;
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	Log::addParameter("Loading font: %s", m_fontPath.c_str());
	ImFontConfig fontCfg;
	fontCfg.GlyphExtraSpacing.x = UISettings::outlineSize();

	                static const ImWchar ranges[] =

	                {

	                    0x0020, 0x00FF, // Basic Latin + Latin Supplement

	                    0x0400, 0x04FF, // Cyrillic + Cyrillic Supplement

	            		0x0E00, 0x0E7F, // Thai

	                    0x2DE0, 0x2DFF, // Cyrillic Extended-A

	                    0xA640, 0xA69F, // Cyrillic Extended-B

	                    0xF020, 0xF0FF,

	            		0

	                };	ImFont* font = io.Fonts->AddFontFromFileTTF(m_fontPath.c_str(),
		UISettings::fontSize(), &fontCfg, ranges);

	if (font == nullptr)
	{
		Log::addParameter("font", font);
		return false;
	}

	// Fonte compacta para janelas internas (criação de personagem etc.):
	// a fonte principal escala com a resolução e fica grande demais para
	// formulários. Esta segunda fonte é carregada no MESMO atlas (ambas
	// são rasterizadas juntas em createFontTexture).
	{
		float smallSize = UISettings::fontSize() * 0.5f;
		if (smallSize < 12.0f) smallSize = 12.0f;
		m_fontSmall = io.Fonts->AddFontFromFileTTF(m_fontPath.c_str(), smallSize, &fontCfg, ranges);
	}

	createFontTexture();

	m_renderer = new ImGuiRenderer(ImGui::GetBackgroundDrawList(), font);

	// voice 
	/*for (const auto& deviceInitCallback : Render::deviceInitCallbacks) {
		if (deviceInitCallback != nullptr) {
			deviceInitCallback();
		}
	}*/

	return true;
}

void ImGuiWrapper::render()
{
	ImGui::NewFrame();


	// Processamento do voice integrado ao ciclo normal do ImGui.
	// O botão utilizado pelo Lurion continua sendo o VoiceButton dos controles;
	// o bloco alternativo de botão ImGui permanece desativado no Plugin.cpp.
	if (pNetGame) {
		if (pUI->dialog()->visible() || pUI->playertablist()->visible() || pNetGame->GetTextDrawPool()->GetState()) {
			SpeakerList::Hide();
			MicroIcon::Hide();
		}
		else if (MicroIcon::hasShowed) {
			SpeakerList::Show();
			MicroIcon::Show();
		}

		for (const auto& renderCallback : Render::renderCallbacks) {
			if (renderCallback != nullptr) {
				renderCallback();
			}
		}
	}

	drawList();
	ImGui::EndFrame();
	ImGui::Render();
	renderDrawData(ImGui::GetDrawData());
}

void ImGuiWrapper::shutdown()
{
	Log::traceLastFunc("ImGuiWrapper::shutdown");

	// voice
	/*for (const auto& deviceFreeCallback : Render::deviceFreeCallbacks) {
		if (deviceFreeCallback != nullptr) {
			deviceFreeCallback();
		}
	}*/

	destroyFontTexture();
}

void ImGuiWrapper::setupRenderState(ImDrawData* draw_data)
{
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)0);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)0);
	RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);
	RwRenderStateSet(rwRENDERSTATEBORDERCOLOR, (void*)0);
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONGREATER);
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)2);
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
	RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);

}

void ImGuiWrapper::checkVertexBuffer(ImDrawData* draw_data)
{
	if (m_vertexBuffer == nullptr || m_vertexBufferSize < draw_data->TotalVtxCount)
	{
		Log::traceLastFunc("ImGuiWrapper::checkVertexBuffer");

		if (m_vertexBuffer)
		{
			delete m_vertexBuffer;
			m_vertexBuffer = nullptr;
		}

		m_vertexBufferSize = draw_data->TotalVtxCount + 10000;
		m_vertexBuffer = new RwIm2DVertex[m_vertexBufferSize];
		if (!m_vertexBuffer)
		{
			Log::addParameter("Buffer allocation error. Size", m_vertexBufferSize);
			return;
		}

		Log::addParameter("Vertex buffer reallocated. Size", m_vertexBufferSize);
	}

}

#include "../game/sprite2d.h"
void ImGuiWrapper::renderDrawData(ImDrawData* draw_data)
{
	const RwReal nearScreenZ = CSprite2d::NearScreenZ;
	const RwReal recipNearClip = CSprite2d::RecipNearClip;

    // Guard: nothing to render
    if (!draw_data || draw_data->TotalVtxCount == 0) return;

	checkVertexBuffer(draw_data);
	setupRenderState(draw_data);

	RwIm2DVertex* vtx_dst = m_vertexBuffer;
	int vtx_offset = 0;
    const ImVec2 display_pos = draw_data->DisplayPos;

	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		const ImDrawVert* vtx_src = cmd_list->VtxBuffer.Data;
		const ImDrawIdx* idx_src = cmd_list->IdxBuffer.Data;

		for (int i = 0; i < cmd_list->VtxBuffer.Size; i++)
		{
            // Apply DisplayPos offset so vertices match scissor coords
            RwIm2DVertexSetScreenX(vtx_dst, vtx_src->pos.x - display_pos.x);
            RwIm2DVertexSetScreenY(vtx_dst, vtx_src->pos.y - display_pos.y);
			RwIm2DVertexSetScreenZ(vtx_dst, nearScreenZ);
			RwIm2DVertexSetRecipCameraZ(vtx_dst, recipNearClip);
			vtx_dst->emissiveColor = vtx_src->col;
			RwIm2DVertexSetU(vtx_dst, vtx_src->uv.x, recipCameraZ);
			RwIm2DVertexSetV(vtx_dst, vtx_src->uv.y, recipCameraZ);

			vtx_dst++;
			vtx_src++;
		}

		const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback)
			{
				pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
                ImVec4 clip_rect;
                const float clip_min_x = pcmd->ClipRect.x - display_pos.x;
                const float clip_min_y = pcmd->ClipRect.y - display_pos.y;
                const float clip_max_x = pcmd->ClipRect.z - display_pos.x;
                const float clip_max_y = pcmd->ClipRect.w - display_pos.y;

                if (clip_max_x <= clip_min_x || clip_max_y <= clip_min_y) {
                    idx_buffer += pcmd->ElemCount;
                    continue;
                }

                // Engine expects {x, bottom, right, top}
                clip_rect.x = clip_min_x;
                clip_rect.y = clip_max_y;
                clip_rect.z = clip_max_x;
                clip_rect.w = clip_min_y;
				SetScissorRect((void*)&clip_rect);

                // Defensive: some commands might not have a texture bound (e.g., color-only passes)
                // Skip draw if texture id is null to avoid undefined results on some GPUs
                if (pcmd->TextureId == nullptr) {
                    idx_buffer += pcmd->ElemCount;
                    continue;
                }

                // Ensure alpha blending and proper filtering for UI quads each submission
                RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
                RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
                RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);

                RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)pcmd->TextureId);
				RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST,
					&m_vertexBuffer[vtx_offset], (RwInt32)cmd_list->VtxBuffer.Size,
					(RwImVertexIndex*)idx_buffer, pcmd->ElemCount);
                RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)0);

                // Disable scissor after ImGui draw (engine treats 0,0,0,0 as no scissor)
                ImVec4 reset_rect = { 0.0f, 0.0f, 0.0f, 0.0f };
                SetScissorRect((void*)&reset_rect);
			}

			idx_buffer += pcmd->ElemCount;
		}
		vtx_offset += cmd_list->VtxBuffer.Size;
	}

    // Ensure following render passes (e.g., CTouchInterface) are not affected by our state
    ImVec4 reset_rect2 = { 0.0f, 0.0f, 0.0f, 0.0f };
    SetScissorRect((void*)&reset_rect2);
    DefinedState2d();
}

bool ImGuiWrapper::createFontTexture()
{
	Log::traceLastFunc("ImGuiWrapper::createFontTexture");

	ImGuiIO& io = ImGui::GetIO();
	unsigned char* pxs;
	int width, height, bytes_per_pixel;
	io.Fonts->GetTexDataAsRGBA32(&pxs, &width, &height, &bytes_per_pixel);

	RwImage* font_img = RwImageCreate(width, height, bytes_per_pixel * 8);
	RwImageAllocatePixels(font_img);

	if (font_img == nullptr)
	{
		Log::addParameter("Font image pointer", font_img);
		return false;
	}

	RwUInt8* pixels = font_img->cpPixels;
	for (int y = 0; y < font_img->height; y++)
	{
		memcpy((void*)pixels, pxs + font_img->stride * y, font_img->stride);
		pixels += font_img->stride;
	}

	RwInt32 w, h, d, flags;
	RwImageFindRasterFormat(font_img, rwRASTERTYPETEXTURE, &w, &h, &d, &flags);
	m_fontRaster = RwRasterCreate(w, h, d, flags);
	if (m_fontRaster == nullptr)
	{
		Log::addParameter("font raster pointer", m_fontRaster);
		return false;
	}

	m_fontRaster = RwRasterSetFromImage(m_fontRaster, font_img);
	io.Fonts->TexID = (ImTextureID*)m_fontRaster;

	RwImageDestroy(font_img);

	Log::addParameter("Atlas width", width);
	Log::addParameter("Atlas height", height);
	return true;

}

void ImGuiWrapper::destroyFontTexture()
{
	if (m_fontRaster)
	{
		ImGuiIO& io = ImGui::GetIO();
		RwRasterDestroy(m_fontRaster);
		m_fontRaster = nullptr;
		io.Fonts->TexID = (ImTextureID)0;
	}

}