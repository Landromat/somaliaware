#pragma once
#include "../../../dependencies/common_includes.hpp"

class c_nightmode : public singleton<c_nightmode> {
public:
	void run() noexcept;
	void apply() noexcept;
	void remove() noexcept;
};
