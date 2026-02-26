#pragma once

namespace globals {
	constexpr int MAX_POLYPHONY = 50;
	constexpr int MAX_MODULATIONS = 64;
	constexpr float POWER_CURVE_POWER = 40.f;
	constexpr int MAX_MACROS = 4;
	constexpr int MAX_LFOS = 4;
	constexpr int MAX_ENVELOPES = 4;
	constexpr int MAX_RNDS = 2;
	constexpr int MAX_MODS = 4; // max visible mods like vel ktrack modwheel on the right panel
	constexpr int EQ_BANDS = 4;

	// theme colors
	inline bool themeLoaded = false;
	inline std::unordered_map<std::string, juce::Colour> colors;

	inline Colour COLOR_BACKGROUND() { return colors["COLOR_BACKGROUND"]; };
	inline Colour COLOR_ACTIVE() { return colors["COLOR_ACTIVE"]; };

	inline void loadDefaultTheme() {
		themeLoaded = true;

		colors["COLOR_BACKGROUND"] = Colour(0xff171414);
		colors["COLOR_ACTIVE"] = Colour(0xff34B8FF);
	}
};