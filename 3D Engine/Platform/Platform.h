#pragma once
#include <Engine/Types.h>
#include <filesystem>

using Crc = uint32;

enum class EMBType
{
	ABORTRETRYIGNORE,
	CANCELTRYCONTINUE,
	HELP,
	OK,
	OKCANCEL,
	RETRYCANCEL,
	YESNO,
	YESNOCANCEL
};

enum class EMBIcon
{
	EXCLAMATION,
	WARNING,
	INFORMATION,
	ASTERISK,
	QUESTION,
	STOP,
	FEL, // ERROR
	HAND,
};

enum class EMBReturn
{
	ABORT,
	CANCEL,
	CONTINUE,
	IGNORERA,
	NO,
	OK,
	RETRY,
	TRYAGAIN,
	YES
};

enum class EMBModality
{
	APPLMODAL,
	SYSTEMMODAL,
	TASKMODAL,
};

class WindowsPlatform
{
public:
	struct GLFWwindow* Window = nullptr;

	// Window
	WindowsPlatform(int32 Width, int32 Height);
	bool WindowShouldClose() const;
	void PollEvents() const;

	/** HWND */
	void* GetWindow() const;

	static void Exit();
	static EMBReturn DisplayMessageBox(EMBType Type, EMBIcon Icon, const std::string& Text, const std::string& Caption, EMBModality Modality = EMBModality::TASKMODAL);

	/** 
	  * Display the underlying operating system's file explorer.
	  * @return Path to the file, or empty if the user cancels. 
	  */
	std::filesystem::path DisplayFileExplorer();

	// File I/O
	static std::string FileRead(const std::filesystem::path& path, const std::string& prependText = "");
	static void FileDelete(const std::string& Filename);
	static void FileRename(const std::string& Old, const std::string& New);
	static bool FileExists(const std::string& Filename);
	static uint64 GetLastWriteTime(const std::filesystem::path& path);

	// Logging
	static void WriteLog(const std::string& Log);
	static void WriteLog(const std::string& File, const std::string& Func, int32 Line, const std::string& Log);
	static void WriteLog(const std::string& Expression, const std::string& File, const std::string& Func, int32 Line, const std::string& Log);
	static std::string FormatString(std::string Format, ...);

	// Processes
	static void ForkProcess(const std::string& ExePath, const std::string& CmdArgs);

	// Memory
	static void Memcpy(void* Dst, const void* Src, size_t Size);

	// Loading
#undef LoadImage
	static uint8* LoadImage(const std::filesystem::path& Path, int32& Width, int32& Height, int32& NumChannels);
	static void FreeImage(uint8* Pixels);

	// .ini
	static bool GetBool(const std::string& Filename, const std::string& Section, const std::string& Key, bool Default);
	static int32 GetInt(const std::string& Filename, const std::string& Section, const std::string& Key, int32 Default);
	static float GetFloat(const std::string& Filename, const std::string& Section, const std::string& Key, float Default);
	static float64 GetFloat64(const std::string& Filename, const std::string& Section, const std::string& Key, float Default);
	static std::string GetString(const std::string& Filename, const std::string& Section, const std::string& Key, const std::string& Default);

	static Crc crc32_u8(const void* data, std::size_t numBytes);
	static void crc32_u8(Crc& crc, const void* data, std::size_t numBytes);
};

using Platform = WindowsPlatform;

// Log.
#define LOG(Fmt, ...) { Platform::WriteLog(Platform::FormatString(Fmt, __VA_ARGS__)); }

// Log and crash if expression fails.
#define check(Expression, Fmt, ...) { ((Expression))? ((void)0) : [&] () { Platform::WriteLog(#Expression, __FILE__, __func__, __LINE__, Platform::FormatString(Fmt, __VA_ARGS__)); throw std::runtime_error(""); }(); }

// Log error and crash.
#define fail(Fmt, ...) { Platform::WriteLog(__FILE__, __func__, __LINE__, Platform::FormatString(Fmt, __VA_ARGS__)); throw std::runtime_error(""); }

#define signal_unimplemented() { fail("UNIMPLEMENTED"); }

struct GLFWWindowUserPointer
{
	class Cursor* Cursor = nullptr;
	class Input* Input = nullptr;
	class Screen* Screen = nullptr;
};