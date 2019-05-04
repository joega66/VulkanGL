#pragma once
#include <Platform/Platform.h>
#include <chrono>

class TimeKeeper
{
public:
	TimeKeeper(const std::string& Event);
	void AddTime(float Time);
	void Finish();

private:
	float TotalTime = 0.0f;
	std::string Event;
};

class ScopedTimer
{
public:
	ScopedTimer(TimeKeeper& Keeper);
	ScopedTimer(bool bFrequency = false);
	~ScopedTimer();

private:
	TimeKeeper* Keeper;
	const bool bFrequency = false;
	std::chrono::system_clock::time_point Start;
};