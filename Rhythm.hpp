#pragma once
#include <array>
#include <vector>
#include <iostream>
#include <cmath>



// Keeps track of the beats in 3/4

struct Rhythm {
	using Measure = std::array<bool, 3>;

	struct HitResult {
		bool in_window = false;
		bool is_strong = false;
		float error_sec = 0.0f;
		int measure_idx = -1;
		int beat_idx = -1;
		bool counted = false;
	};

	Rhythm(float _bpm, std::vector<Measure> _pattern, float _hit_window = 0.12f);

	void reset();
	void start();
	void stop();
	bool is_playing() const;

	void set_bpm(float new_bpm);
	float get_bpm() const;

	void update(float dt);
	HitResult register_tap(); // judge a tap at current song time

	//some helpers
	float loop_duration_sec() const;
	float seconds_per_beat() const;

	//loop completion
	bool all_strong_beats_hit();
	//bool finished_perfect();

	// public accessible configs
	float bpm = 120.0f;
	std::vector<Measure> pattern;
	float hit_window = 0.12f;

	float song_time = 0.0f;

private:
	bool playing = false;
	bool perfect_this_loop = false;
	std::vector<std::array<bool, 3>> hit_flags;


};