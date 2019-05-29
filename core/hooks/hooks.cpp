#pragma once
#include "../../dependencies/common_includes.hpp"
#include "../features/visuals/visuals.hpp"
#include "../features/misc/movement.hpp"
#include "../menu/menu.hpp"
#include "../features/misc/hitmarker.hpp"
#include "../features/misc/prediction.hpp"
#include "../features/misc/misc.hpp"
#include "../features/skinchanger/skinchanger.hpp"
#include "../features/misc/logs.hpp"
#include "../features/misc/events.hpp"
#include "../features/visuals/sound.hpp"
#include "../features/skinchanger/parser.hpp"
#include "../features/visuals/nightmode.hpp"

std::unique_ptr<vmt_hook> hooks::client_hook;
std::unique_ptr<vmt_hook> hooks::clientmode_hook;
std::unique_ptr<vmt_hook> hooks::panel_hook;
std::unique_ptr<vmt_hook> hooks::renderview_hook;
std::unique_ptr<vmt_hook> hooks::surface_hook;
std::unique_ptr<vmt_hook> hooks::modelrender_hook;

c_hooked_events hooked_events;
uint8_t* present_address;
hooks::present_fn original_present;
uint8_t* reset_address;
hooks::reset_fn original_reset;
HWND hooks::window;
WNDPROC hooks::wndproc_original = NULL;

void hooks::initialize() {
	client_hook = std::make_unique<vmt_hook>();
	clientmode_hook = std::make_unique<vmt_hook>();
	panel_hook = std::make_unique<vmt_hook>();
	renderview_hook = std::make_unique<vmt_hook>();
	surface_hook = std::make_unique<vmt_hook>();
	modelrender_hook = std::make_unique<vmt_hook>();

	render::get().setup_fonts();

	client_hook->setup(interfaces::client);
	client_hook->hook_index(37, reinterpret_cast<void*>(frame_stage_notify));

	clientmode_hook->setup(interfaces::clientmode);
	clientmode_hook->hook_index(18, reinterpret_cast<void*>(override_view));
	clientmode_hook->hook_index(24, reinterpret_cast<void*>(create_move));
	clientmode_hook->hook_index(44, reinterpret_cast<void*>(do_post_screen_effects));
	clientmode_hook->hook_index(35, reinterpret_cast<void*>(viewmodel_fov));

	panel_hook->setup(interfaces::panel);
	panel_hook->hook_index(41, reinterpret_cast<void*>(paint_traverse));

	renderview_hook->setup(interfaces::render_view);
	renderview_hook->hook_index(9, reinterpret_cast<void*>(scene_end));

	surface_hook->setup(interfaces::surface);
	surface_hook->hook_index(67, reinterpret_cast<void*>(lock_cursor));
	surface_hook->hook_index(116, reinterpret_cast<void*>(on_screen_size_changed));

	modelrender_hook->setup(interfaces::model_render);

	present_address = utilities::pattern_scan(GetModuleHandleW(L"gameoverlayrenderer.dll"), "FF 15 ? ? ? ? 8B F8 85 DB") + 0x2;
	reset_address = utilities::pattern_scan(GetModuleHandleW(L"gameoverlayrenderer.dll"), "FF 15 ? ? ? ? 8B F8 85 FF 78 18") + 0x2;

	original_present = **reinterpret_cast<present_fn**>(present_address);
	original_reset = **reinterpret_cast<reset_fn**>(reset_address);

	**reinterpret_cast<void***>(present_address) = reinterpret_cast<void*>(&present);
	**reinterpret_cast<void***>(reset_address) = reinterpret_cast<void*>(&reset);

	window = FindWindow("Valve001", NULL);

	wndproc_original = (WNDPROC)SetWindowLongPtrA(window, GWL_WNDPROC, (LONG)wndproc);

	interfaces::console->get_convar("crosshair")->set_value(1);
	interfaces::console->get_convar("viewmodel_fov")->callbacks.set_size(false);
	interfaces::console->get_convar("viewmodel_offset_x")->callbacks.set_size(false);
	interfaces::console->get_convar("viewmodel_offset_y")->callbacks.set_size(false);
	interfaces::console->get_convar("viewmodel_offset_z")->callbacks.set_size(false);

	hooked_events.setup();
	c_kit_parser::get().setup();

	printf("Hooks initialized!\n");
}


void hooks::shutdown() {
	clientmode_hook->release();
	client_hook->release();
	panel_hook->release();
	renderview_hook->release();
	surface_hook->release();
	modelrender_hook->release();

	hooked_events.release();

	**reinterpret_cast<void***>(present_address) = reinterpret_cast<void*>(original_present);
	**reinterpret_cast<void***>(reset_address) = reinterpret_cast<void*>(original_reset);

	SetWindowLongPtrA(FindWindow("Valve001", NULL), GWL_WNDPROC, (LONG)wndproc_original);
}

float __stdcall hooks::viewmodel_fov() {
	auto local_player = reinterpret_cast<player_t*>(interfaces::entity_list->get_client_entity(interfaces::engine->get_local_player()));

	if (local_player && local_player->is_alive()) {
		return 68.f + c_config::get().viewmodel_fov;
	}
	else {
		return 68.f;
	}
}

void __stdcall hooks::on_screen_size_changed(int old_width, int old_height) {
	static auto original_fn = reinterpret_cast<on_screen_size_changed_fn>(surface_hook->get_original(116));

	original_fn(interfaces::surface, old_width, old_height);

	render::get().setup_fonts();
}

int __stdcall hooks::do_post_screen_effects(int value) {
	static auto original_fn = reinterpret_cast<do_post_screen_effects_fn>(clientmode_hook->get_original(44));

	c_visuals::get().glow();

	return original_fn(interfaces::clientmode, value);
}

bool __stdcall hooks::create_move(float frame_time, c_usercmd* user_cmd) {
	static auto original_fn = reinterpret_cast<create_move_fn>(clientmode_hook->get_original(24));
	auto local_player = reinterpret_cast<player_t*>(interfaces::entity_list->get_client_entity(interfaces::engine->get_local_player()));
	original_fn(interfaces::clientmode, frame_time, user_cmd); //fixed create move

	if (!user_cmd || !user_cmd->command_number)
		return original_fn;

	if (!interfaces::entity_list->get_client_entity(interfaces::engine->get_local_player()))
		return original_fn;

	bool& send_packet = *reinterpret_cast<bool*>(*(static_cast<uintptr_t*>(_AddressOfReturnAddress()) - 1) - 0x1C);

	if (interfaces::engine->is_connected() && interfaces::engine->is_in_game()) {
		//misc
		c_movement::get().bunnyhop(user_cmd);
		c_misc::get().clantag_spammer();
		c_misc::get().viewmodel_offset();
		c_misc::get().disable_post_processing();
		c_misc::get().recoil_crosshair();
		c_misc::get().force_crosshair();

		//legitbot and prediction stuff
		c_movement::get().edge_jump_pre_prediction(user_cmd);
		c_prediction::get().start_prediction(user_cmd); //small note for prediction, we need to run bhop before prediction otherwise it will be buggy


		c_prediction::get().end_prediction();
		c_movement::get().edge_jump_post_prediction(user_cmd);

		c_nightmode::get().run();

		// clamping movement
		auto forward = user_cmd->forwardmove;
		auto right = user_cmd->sidemove;
		auto up = user_cmd->upmove;

		if (forward > 450)
			forward = 450;

		if (right > 450)
			right = 450;

		if (up > 450)
			up = 450;

		if (forward < -450)
			forward = -450;

		if (right < -450)
			right = -450;

		if (up < -450)
			up = -450;

		// clamping angles
		user_cmd->viewangles.x = std::clamp(user_cmd->viewangles.x, -89.0f, 89.0f);
		user_cmd->viewangles.y = std::clamp(user_cmd->viewangles.y, -180.0f, 180.0f);
		user_cmd->viewangles.z = 0.0f;
	}

	return false;
}

void __fastcall hooks::override_view(void* _this, void* _edx, c_viewsetup* setup) {
	static auto original_fn = reinterpret_cast<override_view_fn>(clientmode_hook->get_original(18));
	auto local_player = reinterpret_cast<player_t*>(interfaces::entity_list->get_client_entity(interfaces::engine->get_local_player()));

	if (local_player && !local_player->is_scoped() && c_config::get().fov > 0 && c_config::get().visuals_enabled) {
		setup->fov = 90 + c_config::get().fov;
	}

	original_fn(interfaces::clientmode, _this, setup);
}

void __stdcall hooks::draw_model_execute(IMatRenderContext * ctx, const draw_model_state_t & state, const model_render_info_t & info, matrix_t * bone_to_world) {
	static auto original_fn = reinterpret_cast<draw_model_execute_fn>(modelrender_hook->get_original(21));
	auto model_name = interfaces::model_info->get_model_name((model_t*)info.model);
	auto local_player = reinterpret_cast<player_t*>(interfaces::entity_list->get_client_entity(interfaces::engine->get_local_player()));
	auto entity = reinterpret_cast<player_t*>(interfaces::entity_list->get_client_entity(info.entity_index));

	auto material = interfaces::material_system->find_material("debug/debugdrawflat", TEXTURE_GROUP_MODEL);
	material->increment_reference_count();

	if (interfaces::engine->is_connected() && interfaces::engine->is_in_game()) {

		if (strstr(model_name, "sleeve")) {
			if (c_config::get().remove_sleeves) {
				interfaces::render_view->set_blend(0.f);
			}
		}

		if (strstr(model_name, "arms")) {
			if (c_config::get().remove_hands) {
				interfaces::render_view->set_blend(0.f);
			}
		}
	}

	original_fn(interfaces::model_render, ctx, state, info, bone_to_world);
}

void __stdcall hooks::frame_stage_notify(int frame_stage) {
	reinterpret_cast<frame_stage_notify_fn>(client_hook->get_original(37))(interfaces::client, frame_stage);


	if (frame_stage == FRAME_RENDER_START) {
		c_misc::get().remove_smoke();
		c_misc::get().remove_flash();
	}

	else if (frame_stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START) {
		c_skinchanger::get().run();
	}

	else if (frame_stage == FRAME_NET_UPDATE_START && interfaces::engine->is_in_game()) {
		c_sound_esp::get().draw();
	}
}
void __stdcall hooks::paint_traverse(unsigned int panel, bool force_repaint, bool allow_force) {
	std::string panel_name = interfaces::panel->get_panel_name(panel);
	static unsigned int _hud_zoom_panel = 0, _panel = 0;

	PCHAR panel_char = (PCHAR)interfaces::panel->get_panel_name(panel);
	if (strstr(panel_char, "HudZoom")) {
		_hud_zoom_panel = panel;
	}

	if (interfaces::engine->is_connected() && interfaces::engine->is_in_game()) {
		if (c_config::get().remove_scope && panel == _hud_zoom_panel)
			return;
	}

	reinterpret_cast<paint_traverse_fn>(panel_hook->get_original(41))(interfaces::panel, panel, force_repaint, allow_force);

	static bool once = false;

	if (!once) {
		PCHAR panel_char = (PCHAR)interfaces::panel->get_panel_name(panel);
		if (strstr(panel_char, "MatSystemTopPanel")) {
			_panel = panel;
			once = true;
		}
	}

	else if (_panel == panel) {
		c_visuals::get().run();
		c_hitmarker::get().run();
		c_event_logs::get().run();
		c_misc::get().remove_scope();
		c_misc::get().watermark();
		c_misc::get().spectators();
	}
}

void __stdcall hooks::scene_end() {
	auto original_fn = reinterpret_cast<scene_end_fn>(renderview_hook->get_original(9));

	c_visuals::get().chams();

	original_fn(interfaces::render_view);
}

extern LRESULT ImGui_ImplDX9_WndProcHandler(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

LRESULT __stdcall hooks::wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
	static bool pressed = false;

	if (!pressed && GetAsyncKeyState(VK_INSERT)) {
		pressed = true;
	}
	else if (pressed && !GetAsyncKeyState(VK_INSERT)) {
		pressed = false;

		c_menu::get().opened = !c_menu::get().opened;
	}

	if (c_menu::get().opened) {
		interfaces::inputsystem->enable_input(false);

	}
	else if (!c_menu::get().opened) {
		interfaces::inputsystem->enable_input(true);
	}

	if (c_menu::get().opened && ImGui_ImplDX9_WndProcHandler(hwnd, message, wparam, lparam))
		return true;

	return CallWindowProcA(wndproc_original, hwnd, message, wparam, lparam);
}

void __stdcall hooks::lock_cursor() {
	auto original_fn = reinterpret_cast<lock_cursor_fn>(surface_hook->get_original(67));

	if (c_menu::get().opened) {
		interfaces::surface->unlock_cursor();
		return;
	}

	original_fn(interfaces::surface);
}

static bool initialized = false;
long __stdcall hooks::present(IDirect3DDevice9* device, RECT* source_rect, RECT* dest_rect, HWND dest_window_override, RGNDATA* dirty_region) {
	if (!initialized) {
		c_menu::get().apply_fonts();
		c_menu::get().setup_resent(device);
		initialized = true;
	}
	if (initialized) {
		c_menu::get().pre_render(device);
		c_menu::get().post_render();

		c_menu::get().run_popup();
		c_menu::get().run();
		c_menu::get().end_present(device);
	}

	return original_present(device, source_rect, dest_rect, dest_window_override, dirty_region);
}

long __stdcall hooks::reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* present_parameters) {
	if (!initialized)
		original_reset(device, present_parameters);

	c_menu::get().invalidate_objects();
	long hr = original_reset(device, present_parameters);
	c_menu::get().create_objects(device);

	return hr;
}