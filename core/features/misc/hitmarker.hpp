#pragma once
#include "../../../dependencies/common_includes.hpp"

class c_hitmarker : public singleton <c_hitmarker> {
public:
	void run() noexcept;
	void event(i_game_event * event) noexcept;
protected:
	void draw() noexcept;
};
