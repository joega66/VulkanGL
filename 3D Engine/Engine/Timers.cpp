#include "Timers.h"

TimeKeeper::TimeKeeper(const std::string& Event)
	: Event(Event)
{
}

void TimeKeeper::AddTime(float Time)
{
	TotalTime += Time;
}

void TimeKeeper::Finish()
{
	LOG("%s took %.3f seconds", Event.c_str(), TotalTime);
	TotalTime = 0.0f;
}

ScopedTimer::ScopedTimer(TimeKeeper& Keeper)
	: Keeper(&Keeper)
{
	Start = std::chrono::system_clock::now();
}

ScopedTimer::ScopedTimer(bool bFrequency)
	: bFrequency(bFrequency)
{
	Start = std::chrono::system_clock::now();
}

ScopedTimer::~ScopedTimer()
{
	auto End = std::chrono::system_clock::now();
	std::chrono::duration<float> Duration = End - Start;
	if (Keeper)
	{
		Keeper->AddTime(Duration.count());
	}
	else
	{
		LOG("Elapsed time: %.3f", bFrequency ? 1 / Duration.count() : Duration.count());
	}
}