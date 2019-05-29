#pragma once
#include <string>
#include "singleton.hpp"
#include "../interfaces/interfaces.hpp"

enum font_flags {
	fontflag_none,
	fontflag_italic = 0x001,
	fontflag_underline = 0x002,
	fontflag_strikeout = 0x004,
	fontflag_symbol = 0x008,
	fontflag_antialias = 0x010,
	fontflag_gaussianblur = 0x020,
	fontflag_rotary = 0x040,
	fontflag_dropshadow = 0x080,
	fontflag_additive = 0x100,
	fontflag_outline = 0x200,
	fontflag_custom = 0x400,
	fontflag_bitmap = 0x800,
};

class render : public singleton<render> {
public:
	DWORD watermark_font;
	DWORD name_font;
	//DWORD weapon_font;

public:

	int font_weight = 8;
	int weapon_weight = 8;

	int font_size = 11;
	int weapon_size = 11;

	void setup_fonts() {

		//weapon_font = interfaces::surface->font_create();
		watermark_font = interfaces::surface->font_create();
		name_font = interfaces::surface->font_create();

		//interfaces::surface->set_font_glyph(weapon_font, "Tahoma", render::get().weapon_size, render::get().weapon_font * 100, 0, 0, font_flags::fontflag_antialias | font_flags::fontflag_outline);
		interfaces::surface->set_font_glyph(watermark_font, "Tahoma", 12, 500, 0, 0, font_flags::fontflag_dropshadow);
		interfaces::surface->set_font_glyph(name_font, "Tahoma", render::get().font_size, render::get().font_weight * 100, 0, 0, font_flags::fontflag_dropshadow | font_flags::fontflag_outline);

		printf("Render initialized!\n");
	}
	void draw_line(int x1, int y1, int x2, int y2, color colour) {
		interfaces::surface->set_drawing_color(colour.r, colour.g, colour.b, colour.a);
		interfaces::surface->draw_line(x1, y1, x2, y2);
	}
	void draw_text(int x, int y, unsigned long font, const wchar_t* string, color colour) {
		interfaces::surface->set_text_color(colour.r, colour.g, colour.b, colour.a);
		interfaces::surface->draw_text_font(font);
		interfaces::surface->draw_text_pos(x, y);
		interfaces::surface->draw_render_text(string, wcslen(string));
	}
	void draw_text(int x, int y, unsigned long font, std::string string, bool text_centered, color colour) {
		std::wstring text = std::wstring(string.begin(), string.end());
		const wchar_t* converted_text = text.c_str();

		int width, height;
		interfaces::surface->get_text_size(font, converted_text, width, height);

		interfaces::surface->set_text_color(colour.r, colour.g, colour.b, colour.a);
		interfaces::surface->draw_text_font(font);
		if (text_centered)
			interfaces::surface->draw_text_pos(x - (width / 2), y);
		else
			interfaces::surface->draw_text_pos(x, y);
		interfaces::surface->draw_render_text(converted_text, wcslen(converted_text));
	}
	void draw_rect(int x, int y, int w, int h, color color) {
		interfaces::surface->set_drawing_color(color.r, color.g, color.b, color.a);
		interfaces::surface->draw_outlined_rect(x, y, w, h);
	}

	void draw_filled_rect(int x, int y, int w, int h, color colour) {
		interfaces::surface->set_drawing_color(colour.r, colour.g, colour.b, colour.a);
		interfaces::surface->draw_filled_rectangle(x, y, w, h);
	}

	void draw_outline(int x, int y, int w, int h, color colour) {
		interfaces::surface->set_drawing_color(colour.r, colour.g, colour.b, colour.a);
		interfaces::surface->draw_outlined_rect(x, y, w, h);
	}
	void draw_textured_polygon(int n, vertex_t* vertice, color col) {
		static int texture_id = interfaces::surface->create_new_texture_id(true);
		static unsigned char buf[4] = { 255, 255, 255, 255 };
		interfaces::surface->set_texture_rgba(texture_id, buf, 1, 1);
		interfaces::surface->set_drawing_color(col.r, col.g, col.b, col.a);
		interfaces::surface->set_texture(texture_id);
		interfaces::surface->draw_polygon(n, vertice);
	}

	void draw_circle(int x, int y, int r, int s, color col) {
		float Step = M_PI * 2.0 / s;
		for (float a = 0; a < (M_PI*2.0); a += Step) {
			float x1 = r * cos(a) + x;
			float y1 = r * sin(a) + y;
			float x2 = r * cos(a + Step) + x;
			float y2 = r * sin(a + Step) + y;
			interfaces::surface->set_drawing_color(col.r, col.g, col.b, col.a);
			interfaces::surface->draw_line(x1, y1, x2, y2);
		}
	}
	void get_text_size(unsigned long font, std::string string, int w, int h) {
		std::wstring text = std::wstring(string.begin(), string.end());
		const wchar_t* out = text.c_str();

		interfaces::surface->get_text_size(font, out, w, h);
	}
	vec2_t get_screen_size(vec2_t area) {
		static int old_w, old_h;
		interfaces::engine->get_screen_size((int&)area.x, (int&)area.y);

		if ((int&)area.x != old_w || (int&)area.y != old_h) {
			old_w = (int&)area.x;
			old_h = (int&)area.y;
			return vec2_t(old_w, old_h);
		}
		return area;
	}
};