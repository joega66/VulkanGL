#include "Screen.h"

void Screen::RegisterScreenResChangedCallback(const ScreenResChangedCallback& Callback)
{
	Callback(Width, Height);
	ScreenResChangedCallbacks.push_back(Callback);
}

void Screen::CallScreenResChanged()
{
	for (auto& Callback : ScreenResChangedCallbacks)
	{
		Callback(Width, Height);
	}
}