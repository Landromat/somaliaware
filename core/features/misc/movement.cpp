#include "../../../dependencies/common_includes.hpp"
#include "movement.hpp"
auto flags_backup = 0;

void c_movement::bunnyhop(c_usercmd* user_cmd) noexcept {
	if (!c_config::get().bunny_hop || !c_config::get().misc_enabled)
		return;

	auto local_player = reinterpret_cast<player_t*>(interfaces::entity_list->get_client_entity(interfaces::engine->get_local_player()));
	static bool last_jumped = false;
	static bool fake_jump = false;
	static int actual_jump = 0;

	if (!local_player)
		return;

	if (local_player->move_type() == movetype_ladder || local_player->move_type() == movetype_noclip)
		return;

	if (!last_jumped && fake_jump) {
		fake_jump = false;
		user_cmd->buttons |= in_jump;
	}
	else if (user_cmd->buttons & in_jump) {
		if (local_player->flags() & fl_onground) {
			actual_jump++;
			last_jumped = true;
			fake_jump = true;
		}
		else {
			if (c_config::get().bunny_hop_humanize && c_config::get().bunny_hop_minimum && (actual_jump > c_config::get().bunny_hop_minimum_value) && (rand() % 100 > c_config::get().bunny_hop_hitchance))
				return;

			if (c_config::get().bunny_hop_humanize && !c_config::get().bunny_hop_minimum && (rand() % 100 > c_config::get().bunny_hop_hitchance))
				return;

			if (c_config::get().bunny_hop_minimum && !c_config::get().bunny_hop_humanize && (actual_jump > c_config::get().bunny_hop_minimum_value))
				return;

			if (c_config::get().bunny_hop_maximum && (actual_jump > c_config::get().bunny_hop_maximum_value))
				return;

			user_cmd->buttons &= ~in_jump;
			last_jumped = false;
		}
	}
	else {
		actual_jump = 0;
		last_jumped = false;
		fake_jump = false;
	}
}

void c_movement::edge_jump_pre_prediction(c_usercmd* user_cmd) noexcept {
	auto local_player = reinterpret_cast<player_t*>(interfaces::entity_list->get_client_entity(interfaces::engine->get_local_player()));

	if (!c_config::get().edge_jump)
		return;

	if (!GetAsyncKeyState(c_config::get().edge_jump_key))
		return;

	if (!local_player)
		return;

	if (local_player->move_type() == movetype_ladder || local_player->move_type() == movetype_noclip)
		return;

	flags_backup = local_player->flags();
}

void c_movement::edge_jump_post_prediction(c_usercmd* user_cmd) noexcept {
	auto local_player = reinterpret_cast<player_t*>(interfaces::entity_list->get_client_entity(interfaces::engine->get_local_player()));

	if (!c_config::get().edge_jump)
		return;

	if (!GetAsyncKeyState(c_config::get().edge_jump_key))
		return;

	if (!local_player)
		return;

	if (local_player->move_type() == movetype_ladder || local_player->move_type() == movetype_noclip)
		return;

	if (flags_backup & fl_onground && !(local_player->flags() & fl_onground))
		user_cmd->buttons |= in_jump;

	if (!(local_player->flags() & fl_onground) && c_config::get().edge_jump_duck_in_air)
		user_cmd->buttons |= in_duck;
}