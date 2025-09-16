#include "Rhythm.hpp"
#include <vector>
#include <utility>
#include <iostream>

Rhythm::Rhythm(float _bpm, std::vector<Measure> _pattern, float _hit_window):
	bpm(_bpm), pattern(std::move(_pattern)), hit_window(_hit_window) {
	reset();
}

void Rhythm::reset(){
	song_time = 0.0001f;
	perfect_this_loop = false;
	hit_flags.assign(pattern.size(), { false,false,false });
}

void Rhythm::start() {
	playing = true;
}

void Rhythm::stop() {
	playing = false;
}

bool Rhythm::is_playing() const {
	return playing;
}

void Rhythm::set_bpm(float new_bpm) {
	bpm = new_bpm;
}

float Rhythm::get_bpm() const {
	return bpm;
}

void Rhythm::update(float dt) {
	if (!playing) return;
	float total = loop_duration_sec();
	float prev = song_time;
	song_time = std::fmod(song_time + dt, total);
	if (song_time < prev) {
		perfect_this_loop = all_strong_beats_hit();
		hit_flags.assign(pattern.size(), { false, false, false });
	}
	/*if (perfect_this_loop) {
		std::cout << "[rhythm] perfect loop\n";
	}
	else {
		std::cout << "[Rhythm] loop wrapped, not perfect \n";
	}*/
}

// calculate the nearest beat and find the difference between player 
// tap time and the actual beat time
Rhythm::HitResult Rhythm::register_tap() {
	HitResult hr;
	float spb = seconds_per_beat();
	float tap = song_time;

	int total_beats = int(pattern.size()) * 3;
	int nearest_unwrapped = int(std::lround(tap / spb));
	float beat_time_unwrapped = float(nearest_unwrapped) * spb;
	int idx_mod = nearest_unwrapped % total_beats;
	if (idx_mod < 0) {
		idx_mod += total_beats;
	}

	int measure_idx = idx_mod / 3;
	int beat_idx = idx_mod % 3;

	float err = tap - beat_time_unwrapped;

	hr.beat_idx = beat_idx;
	hr.measure_idx = measure_idx;
	hr.error_sec = err;

	if (std::fabs(err) < hit_window) {
		hr.in_window = true;
		bool strong = pattern[measure_idx][beat_idx];
		hr.is_strong = strong;

		std::cout
			<< "[Tap] measure=" << int(measure_idx)
			<< " beat=" << (beat_idx + 1)
			<< " strong=" << (strong ? "Y" : "N")
			<< " error_ms=" << int(std::round(err * 1000.0f))
			<< " (window= " << int(std::round(hit_window * 1000.0f)) << "ms)\n";
		
		if (strong && !hit_flags[measure_idx][beat_idx]) {
			hit_flags[measure_idx][beat_idx] = true;
			hr.counted = true;
			std::cout << "counted \n";
		}
		else if (strong) {
			std::cout << " already counted this beat this loop\n";
		}
		else {
			// Out-of-window debug:
			std::cout
				<< "[Tap] OUT OF WINDOW  measure=" << int(measure_idx)
				<< " beat=" << (beat_idx + 1)
				<< " error_ms=" << int(std::round(err * 1000.0f))
				<< " (window= " << int(std::round(hit_window * 1000.0f)) << "ms)\n";
		}
	}
	return hr;
}

float Rhythm::loop_duration_sec() const {
	return pattern.size() * 3.0f * seconds_per_beat();
}

float Rhythm::seconds_per_beat() const{
	return 60.0f / bpm;
}

// check if all of the strong beats are hit perfectly by the player
bool Rhythm::all_strong_beats_hit() {
	for (size_t m = 0; m < pattern.size(); ++m) {
		for (size_t b = 0; b < 3; ++b) {
			if (pattern[m][b] && !hit_flags[m][b]) {
				return false;
			}
		}
	}
	return true;
}

//bool Rhythm::finished_perfect() {
//	return all_strong_beats_hit();
//}
