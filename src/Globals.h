#pragma once

namespace globals {
	constexpr int MAX_POLYPHONY = 50;


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