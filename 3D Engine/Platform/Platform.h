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
	struct GLFWwindow* _Window = nullptr;

	// Window
	WindowsPlatform(int32 width, int32 height);
	bool WindowShouldClose() const;
	void PollEvents() const;

	/** HWND */
	void* GetWindow() const;

	static void Exit();
	static EMBReturn DisplayMessageBox(EMBType type, EMBIcon icon, const std::string& text, const std::string& caption, EMBModality modality = EMBModality::TASKMODAL);

	/** 
	  * Display the underlying operating system's file explorer.
	  * @return Path to the file, or empty if the user cancels. 
	  */
	std::filesystem::path DisplayFileExplorer();

	// File I/O
	static std::string FileRead(const std::filesystem::path& path, const std::string& prependText = "");
	static void FileDelete(const std::string& filename);
	static void FileRename(const std::string& oldName, const std::string& newNew);
	static bool FileExists(const std::string& filename);
	static uint64 GetLastWriteTime(const std::filesystem::path& path);

	// Logging
	static void WriteLog(const std::string& log);
	static void WriteLog(const std::string& file, const std::string& func, int32 line, const std::string& log);
	static void WriteLog(const std::string& expression, const std::string& file, const std::string& func, int32 line, const std::string& log);
	static std::string FormatString(std::string format, ...);

	// Processes
	static void ForkProcess(const std::string& exePath, const std::string& cmdArgs);

	// Memory
	static void Memcpy(void* dst, const void* src, size_t size);

	// Loading
#undef LoadImage
	static uint8* LoadImage(const std::filesystem::path& path, int32& width, int32& height, int32& numChannels);
	static void FreeImage(uint8* pixels);

	// .ini
	static bool GetBool(const std::string& filename, const std::string& section, const std::string& key, bool defaultVal);
	static int32 GetInt(const std::string& filename, const std::string& section, const std::string& key, int32 defaultVal);
	static float GetFloat(const std::string& filename, const std::string& section, const std::string& key, float defaultVal);
	static float64 GetFloat64(const std::string& filename, const std::string& section, const std::string& key, float defaultVal);
	static std::string GetString(const std::string& filename, const std::string& section, const std::string& key, const std::string& defaultVal);

	static void crc32_u8(Crc& crc, const void* data, std::size_t numBytes);
	static void crc32_u32(Crc& crc, const void* data, std::size_t numBytes);
};

using Platform = WindowsPlatform;

// Log.
#define LOG(fmt, ...) { Platform::WriteLog(Platform::FormatString(fmt, __VA_ARGS__)); }

// Log and crash if expression fails.
#define check(expression, fmt, ...) { ((expression))? ((void)0) : [&] () { Platform::WriteLog(#expression, __FILE__, __func__, __LINE__, Platform::FormatString(fmt, __VA_ARGS__)); throw std::runtime_error(""); }(); }

// Log error and crash.
#define fail(fmt, ...) { Platform::WriteLog(__FILE__, __func__, __LINE__, Platform::FormatString(fmt, __VA_ARGS__)); throw std::runtime_error(""); }

#define signal_unimplemented() { fail("UNIMPLEMENTED"); }

struct GLFWWindowUserPointer
{
	class Cursor* _Cursor = nullptr;
	class Input* _Input = nullptr;
	class Screen* _Screen = nullptr;
};