#include <fstream>
#include <cstdarg>
#include <iostream>
#include <algorithm>
#include <filesystem>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <Engine/Cursor.h>
#include <Engine/Input.h>

IPlatformRef GPlatform;

std::string IPlatform::FileRead(const std::string& Filename) const
{
	std::ifstream File(Filename, std::ios::ate | std::ios::binary);

	check(File.is_open(), "Failed to open file %s", Filename.c_str());

	size_t FileSize = static_cast<size_t>(File.tellg());
	std::string Buffer;
	Buffer.resize(FileSize);

	File.seekg(0);
	File.read(Buffer.data(), FileSize);
	File.close();

	return Buffer;
}

void IPlatform::FileDelete(const std::string& Filename) const
{
	check(std::filesystem::remove(Filename), "Failed to remove file...");
}

void IPlatform::FileRename(const std::string& Old, const std::string& New) const
{
	check(FileExists(Old), "Renaming file that doesn't exist...");
	std::filesystem::rename(Old, New);
}

bool IPlatform::FileExists(const std::string& Filename) const
{
	return std::filesystem::is_regular_file(Filename);
}

void IPlatform::WriteLog(const std::string& InLog)
{
	std::string Recent = InLog + '\n';
	std::cerr << Recent;
}

void IPlatform::WriteLog(const std::string& File, const std::string& Func, int32 Line, const std::string& InLog)
{
	std::string Header = "[Debug] [" + SanitizeFile(File) + ":" + Func + ":" + std::to_string(Line) + "]\n";
	std::string Recent = Header + InLog + '\n';
	std::cerr << Recent;
}

void IPlatform::WriteLog(const std::string& Expression, const std::string& File, const std::string& Func, int32 Line, const std::string& InLog)
{
	std::string Header = "[Warning] [" + SanitizeFile(File) + ":" + Func + ":" + std::to_string(Line) + ":" + Expression + "]\n";
	std::string Recent = Header + InLog + '\n';
	std::cerr << Recent;
}

std::string IPlatform::SanitizeFile(const std::string& File)
{
	std::string FileSanitized = File;
	FileSanitized.erase(FileSanitized.begin(), FileSanitized.begin() + FileSanitized.find_last_of('\\') + 1);
	return FileSanitized;
}

std::string IPlatform::FormatString(std::string Format, ...)
{
	va_list Args, ArgsCopy;
	va_start(Args, Format);
	va_copy(ArgsCopy, Args);

	const auto SZ = std::vsnprintf(nullptr, 0, Format.c_str(), Args) + 1;

	try
	{
		std::string Result(SZ, ' ');
		std::vsnprintf(&Result.front(), SZ, Format.c_str(), ArgsCopy);
		va_end(ArgsCopy);
		va_end(Args);
		Result.erase(std::remove(Result.begin(), Result.end(), '\0'), Result.end());
		return Result;
	}
	catch (const std::bad_alloc& E)
	{
		va_end(ArgsCopy);
		va_end(Args);
		fail(E.what());
	}
}

void IPlatform::AddWindowListener(WindowResizeListenerRef WindowListener)
{
	WindowListeners.push_back(WindowListener);
}

void IPlatform::RemoveWindowListener(WindowResizeListenerRef WindowListener)
{
	WindowListeners.erase(std::remove(WindowListeners.begin(), WindowListeners.end(), WindowListener), WindowListeners.end());
}

void IPlatform::NotifyWindowListeners(int32 NewX, int32 NewY)
{
	for (auto& Listener : WindowListeners)
	{
		Listener->OnWindowResize(NewX, NewY);
	}
}

void IPlatform::Memcpy(void* Dst, const void* Src, size_t Size)
{
	memcpy(Dst, Src, Size);
}

uint8* IPlatform::LoadImage(const std::string& Filename, int32& Width, int32& Height, int32& NumChannels)
{
	uint8* Image = stbi_load(Filename.c_str(), &Width, &Height, &NumChannels, STBI_rgb_alpha);
	check(Image, "%s failed to load", Filename.c_str());
	return Image;
}

void IPlatform::FreeImage(uint8* Pixels)
{
	stbi_image_free(Pixels);
}

void IPlatform::EndFrame()
{
	// Update hardware cursor.
	GPlatform->MouseState(Cursor);

	// Update hardware input.
	std::fill(Input.KeysPressed.begin(), Input.KeysPressed.end(), false);
}

WindowResizeListener::~WindowResizeListener()
{
	GPlatform->RemoveWindowListener(this);
}