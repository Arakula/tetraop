#pragma once

namespace globals {
	constexpr int SIMD_SZ = 4;
	constexpr int MAX_BLOCKSIZE = 128;
	constexpr int MAX_POLYPHONY = 50;
	constexpr int MAX_MODULATIONS = 64;
	constexpr float MAX_DETUNE_CENTS = 200.f;
	constexpr float POWER_CURVE_POWER = 40.f;
	constexpr int MAX_MACROS = 4;
	constexpr int MAX_LFOS = 4;
	constexpr int MAX_ENVELOPES = 4;
	constexpr int MAX_OSCILLATORS = 4;
	constexpr int MAX_RNDS = 2;
	constexpr int MAX_MODS = 4; // max visible mods like vel ktrack modwheel on the right panel
	constexpr int EQ_BANDS = 4;
	constexpr int MAX_UNISON = 16;
	constexpr float MORPH_SECONDS = 0.005f;
	constexpr float DB2LOG = 0.11512925464970228420089957273422f;
	constexpr float PARAM_SMOOTHER_RESISTANCE = 0.05f;
	constexpr int SCOPE_BUFLEN = 512;

	// Theme vars
	constexpr int PANEL_PAD = 2;
	constexpr float PANEL_CORNER = 0.f;
	constexpr int PANEL_HEADER_HEIGHT = 24;
	constexpr int KNOB_WIDTH = 60;
	constexpr int KNOB_HEIGHT = 75;
	constexpr int KNOB_WIDTH_SM = 50;
	constexpr int KNOB_HEIGHT_SM = 60;
	constexpr float KNOB_RADIUS = 14.f;
	constexpr float KNOB_RADIUS_SM = 10.f;
	constexpr float KNOB_YOFFSET = -2.f;
	constexpr float KNOB_YOFFSET_SM = -6.f;
	constexpr float KNOB_MOD_OFFSET = 2.f;
	constexpr float KNOB_MOD_THICKNESS = 2.f;
    constexpr float KNOB_MODVAL_RADIUS = 1.5f;
    constexpr float KNOB_MODVAL_OFFSET = 2.f;
    constexpr float KNOB_VALUE_OFFSET = 4.f;
	constexpr float KNOB_VALUE_OFFSET_SM = 4.f;
    constexpr float KNOB_VALUE_THICKNESS = 3.f;
    constexpr float KNOB_SHADOW_ALPHA = .25f;
	constexpr float KNOB_LABEL_YOFFSET = -3.f;


	// theme colors
	inline bool themeLoaded = false;
	inline std::unordered_map<std::string, juce::Colour> colors;

	inline Colour COLOR_BACKGROUND() { return colors["COLOR_BACKGROUND"]; };
	inline Colour COLOR_ACTIVE() { return colors["COLOR_ACTIVE"]; };
	inline Colour COLOR_PANEL() { return colors["COLOR_PANEL"]; };
	inline Colour COLOR_BEVEL() { return colors["COLOR_BEVEL"]; };
	inline Colour COLOR_PANEL_HEADER() { return colors["COLOR_PANEL_HEADER"]; };
	inline Colour COLOR_KNOB() { return colors["COLOR_KNOB"]; };
	inline Colour COLOR_KNOB_HANDLE() { return colors["COLOR_KNOB_HANDLE"]; };
	inline Colour COLOR_KNOB_LABEL() { return colors["COLOR_KNOB_LABEL"]; };
	inline Colour COLOR_KNOB_ARC() { return colors["COLOR_KNOB_ARC"]; };

	inline void loadDefaultTheme() {
		themeLoaded = true;

		colors["COLOR_BACKGROUND"] = Colour(0xff1A1A1A);
		colors["COLOR_ACTIVE"] = Colour(0xff00C8FF);
		colors["COLOR_PANEL"] = Colour(0xff3F3F3F);
		colors["COLOR_BEVEL"] = Colour(0xff252424);
		colors["COLOR_PANEL_HEADER"] = Colour(0xffBABABA);
		colors["COLOR_KNOB"] = Colour(0xff626262);
		colors["COLOR_KNOB_HANDLE"] = Colour(0xffD9D9D9);
		colors["COLOR_KNOB_LABEL"] = Colour(0xffFAFAFA);
		colors["COLOR_KNOB_ARC"] = Colour(0xff5C5C5C);
	}
};