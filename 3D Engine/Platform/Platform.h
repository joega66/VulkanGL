#pragma once
#include <Engine/Types.h>

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

class OS_Platform
{
public:
	struct GLFWwindow* Window = nullptr;

	// Window
	void OpenWindow(int32 Width, int32 Height);
	bool WindowShouldClose();
	void PollEvents();
	void Exit();
	EMBReturn DisplayMessageBox(EMBType Type, EMBIcon Icon, const std::string& Text, const std::string& Caption, EMBModality Modality = EMBModality::TASKMODAL) const;

	// File I/O
	std::string FileRead(const std::string& Filename) const;
	void FileDelete(const std::string& Filename) const;
	void FileRename(const std::string& Old, const std::string& New) const;
	bool FileExists(const std::string& Filename) const;
	uint64 GetLastWriteTime(const std::string& Filename) const;

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

	// .ini
	bool GetBool(const std::string& Filename, const std::string& Section, const std::string& Key, bool Default) const;
	int32 GetInt(const std::string& Filename, const std::string& Section, const std::string& Key, int32 Default) const;
	float64 GetFloat64(const std::string& Filename, const std::string& Section, const std::string& Key, float Default) const;
	std::string GetString(const std::string& Filename, const std::string& Section, const std::string& Key, const std::string& Default) const;

private:
	static std::string SanitizeFile(const std::string& File);

};

extern OS_Platform Platform;

// Log.
#define LOG(Fmt, ...) { Platform.WriteLog(OS_Platform::FormatString(Fmt, __VA_ARGS__)); }

// Log and crash if expression fails.
#define check(Expression, Fmt, ...) { ((Expression))? ((void)0) : [&] () { Platform.WriteLog(#Expression, __FILE__, __func__, __LINE__, OS_Platform::FormatString(Fmt, __VA_ARGS__)); throw std::runtime_error(""); }(); }

// Log error and crash.
#define fail(Fmt, ...) { Platform.WriteLog(__FILE__, __func__, __LINE__, OS_Platform::FormatString(Fmt, __VA_ARGS__)); throw std::runtime_error(""); }

#define signal_unimplemented() { fail("UNIMPLEMENTED"); }