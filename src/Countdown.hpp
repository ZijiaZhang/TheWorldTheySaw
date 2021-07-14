#pragma once
#include "tiny_ecs.hpp"
#include "common.hpp"

class Countdown {
public:
	static ECS::Entity createCountdown(float countdown_ms);
};