// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/backend.hpp>
#include <ny/log.hpp> // ny::warning
#include <cstdlib> // std::getenv
#include <cstring> // std::strcmp

namespace ny {

std::vector<Backend*> Backend::backendsFunc(Backend* reg, bool remove)
{
	static std::vector<Backend*> backends_;

	if(reg) {
		if(remove) {
			for(auto it = backends_.cbegin(); it < backends_.cend(); ++it) {
				if(*it == reg) {
					backends_.erase(it);
					break;
				}
			}
		} else {
			backends_.push_back(reg);
		}
	}

	return backends_;
}

Backend& Backend::choose()
{
	static const std::string waylandString = "wayland";
	static const std::string x11String = "x11";
	static const std::string winapiString = "winapi";

	auto* envBackend = std::getenv("NY_BACKEND");

	auto bestScore = -1;
	Backend* best = nullptr;
	for(auto& backend : backends()) {
		if(!backend->available()) continue;
		if(envBackend && !std::strcmp(backend->name(), envBackend)) return *backend;

		// score is chosen this way since there might be x servers on winapi
		// but no winapi on linux and we always want the native backend
		// wayland > x11 because of Xwayland
		auto score = 0;
		if(backend->name() == winapiString) score = 3;
		else if(backend->name() == waylandString) score = 2;
		else if(backend->name() == x11String) score = 1;

		if(score > bestScore) {
			bestScore = score;
			best = backend;
		}
	}

	if(envBackend && (!best || std::strcmp(best->name(), envBackend)))
		warning("ny::Backend: requested backend (env NY_BACKEND) ", envBackend, " not available!");

	if(!best) throw std::runtime_error("ny::Backend: no backend available.");
	return *best;
}

} // namespace ny
