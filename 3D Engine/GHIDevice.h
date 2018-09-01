#pragma once

/* Graphics Hardware Interface Device. */
class GHIDevice
{
public:
	virtual void Open() = 0;
	virtual void Close() = 0;
};