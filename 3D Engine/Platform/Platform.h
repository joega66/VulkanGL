#pragma once
#include <Engine/Types.h>

class OS_Platform
{
public:
	struct GLFWwindow* Window;

	// Window
	void OpenWindow(int32 Width, int32 Height);
	bool WindowShouldClose();
	void PollEvents();
	void Finish();

	// File I/O
	std::string FileRead(const std::string& Filename) const;
	void FileDelete(const std::string& Filename) const;
	void FileRename(const std::string& Old, const std::string& New) const;
	bool FileExists(const std::string& Filename) const;

	// Logging
	void WriteLog(const std::string& Log);
	void WriteLog(const std::string& File, const std::string& Func, int32 Line, const std::string& Log);
	void WriteLog(const std::string& Expression, const std::string& File, const std::string& Func, int32 Line, const std::string& Log);
	static std::string FormatString(std::string format, ...);

	// Processes
	void ForkProcess(const std::string& ExePath, const std::string& CmdArgs) const;

	// Memory
	void Memcpy(void* Dst, const void* Src, size_t Size) const;

	// Loading
#undef LoadImage
	uint8* LoadImage(const std::string& Filename, int32& Width, int32& Height, int32& NumChannels) const;
	void FreeImage(uint8* Pixels) const;

private:
	void UpdateCursorState(class Cursor& Cursor);
	void UpdateInputState(class Input& Input);

	static void WindowResizeCallback(GLFWwindow* Window, int32 X, int32 Y);
	static void KeyboardCallback(GLFWwindow* Window, int32 Key, int32 Scancode, int32 Action, int32 Mode);
	static void ScrollCallback(GLFWwindow* Window, double XOffset, double YOffset);
	static void MouseCallback(GLFWwindow* Window, double X, double Y);
	static void MouseButtonCallback(GLFWwindow* Window, int32 Button, int32 Action, int32 Mods);

	static std::string SanitizeFile(const std::string& file);
};

extern OS_Platform Platform;

// Log.
#define LOG(Fmt, ...) { Platform.WriteLog(OS_Platform::FormatString(Fmt, __VA_ARGS__)); }

// Log and crash if expression fails.
#define check(Expression, Fmt, ...) { ((Expression))? ((void)0) : [&] () { Platform.WriteLog(#Expression, __FILE__, __func__, __LINE__, OS_Platform::FormatString(Fmt, __VA_ARGS__)); throw std::runtime_error(""); }(); }

// Log error and crash.
#define fail(Fmt, ...) { Platform.WriteLog(__FILE__, __func__, __LINE__, OS_Platform::FormatString(Fmt, __VA_ARGS__)); throw std::runtime_error(""); }

#define signal_unimplemented() { fail("UNIMPLEMENTED"); }