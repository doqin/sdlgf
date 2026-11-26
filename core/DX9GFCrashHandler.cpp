#include "pch.h"
#include "DX9GFCrashHandler.h"
#include "DX9GFPlatform.h"

#include <string>

#if defined(_WIN32)

void DX9GF::InstallCrashHandler()
{
	// Hard crashes (access violations, driver faults) bypass C++ catch and kill
	// the process silently. Log the exception code, fault details and a stack
	// trace (module + RVA per frame, resolvable against local PDBs) so runs on
	// clean machines (sandbox/CI) are diagnosable instead of a mystery exit
	SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* info) -> LONG {
		std::string report;
		char line[256];

		const EXCEPTION_RECORD* rec = info->ExceptionRecord;
		sprintf_s(line, "Unhandled exception 0x%08X at 0x%p",
			static_cast<UINT>(rec->ExceptionCode), rec->ExceptionAddress);
		report += line;

		// Access violations: say whether it was a read/write and at which address
		if (rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && rec->NumberParameters >= 2) {
			const char* kind =
				rec->ExceptionInformation[0] == 0 ? "READ" :
				rec->ExceptionInformation[0] == 1 ? "WRITE" : "EXECUTE";
			sprintf_s(line, " (%s of address 0x%IX)", kind, rec->ExceptionInformation[1]);
			report += line;
		}

		// Capture the call stack and resolve each frame to owning module + offset.
		// Offsets are enough to find the faulting function offline using the PDBs
		// from the build machine (demo.pdb / core.pdb)
		void* frames[32] = {};
		const USHORT frameCount = RtlCaptureStackBackTrace(1, 32, frames, nullptr);
		if (frameCount > 0) {
			report += "\r\nStack:\r\n";
			for (USHORT i = 0; i < frameCount; ++i) {
				HMODULE mod = nullptr;
				char modPath[MAX_PATH] = "?";
				DWORD64 rva = reinterpret_cast<DWORD64>(frames[i]);
				if (GetModuleHandleExA(
					GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
					GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
					reinterpret_cast<LPCSTR>(frames[i]), &mod) && mod != nullptr) {
					GetModuleFileNameA(mod, modPath, MAX_PATH);
					const char* fileName = strrchr(modPath, '\\');
					fileName = fileName ? fileName + 1 : modPath;
					rva -= reinterpret_cast<DWORD64>(mod);
					sprintf_s(line, "  %02u  %s+0x%IX", i, fileName, rva);
				}
				else {
					sprintf_s(line, "  %02u  0x%p", i, frames[i]);
				}
				report += line;
				report += "\r\n";
			}
		}

		wchar_t exePath[MAX_PATH]{};
		GetModuleFileNameW(nullptr, exePath, MAX_PATH);
		HANDLE file = CreateFileW(
			(std::wstring(exePath) + L".crash.log").c_str(),
			FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file != INVALID_HANDLE_VALUE) {
			DWORD written = 0;
			WriteFile(file, report.c_str(), static_cast<DWORD>(report.size()), &written, nullptr);
			WriteFile(file, "\r\n", 2, &written, nullptr);
			CloseHandle(file);
		}
		// Deliberately the raw Win32 call: this runs inside a crashed process, so avoid
		// depending on SDL being in a usable state
		MessageBoxA(NULL, report.c_str(), "Crash", MB_OK | MB_ICONERROR);
		return EXCEPTION_EXECUTE_HANDLER;
	});
}

#else // POSIX

#include <csignal>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#if __has_include(<execinfo.h>)
#	include <execinfo.h>
#	define SDLGF_HAVE_BACKTRACE 1
#endif

namespace {
	// Computed up front: the handler must not allocate
	std::string g_crashLogPath;

	void CrashSignalHandler(int sig)
	{
		// Best effort. Not strictly async-signal-safe (snprintf/strsignal), but the process is
		// already doomed and a useful log matters more than strictness here.
		const int fd = open(g_crashLogPath.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
		char head[160];
		const int n = std::snprintf(head, sizeof(head), "Unhandled signal %d (%s)\nStack:\n", sig, strsignal(sig));
		if (n > 0) {
			if (fd >= 0) (void)!write(fd, head, static_cast<size_t>(n));
			(void)!write(STDERR_FILENO, head, static_cast<size_t>(n));
		}
#ifdef SDLGF_HAVE_BACKTRACE
		void* frames[64];
		const int count = backtrace(frames, 64);
		if (fd >= 0) backtrace_symbols_fd(frames, count, fd);
		backtrace_symbols_fd(frames, count, STDERR_FILENO);
#endif
		if (fd >= 0) close(fd);

		// Restore the default action so the process still dies with the right status
		std::signal(sig, SIG_DFL);
		std::raise(sig);
	}
}

void DX9GF::InstallCrashHandler()
{
	g_crashLogPath = DX9GF::Platform::GetExecutablePath() + ".crash.log";
	const int signals[] = { SIGSEGV, SIGABRT, SIGFPE, SIGILL
#ifdef SIGBUS
		, SIGBUS
#endif
	};
	for (const int sig : signals) {
		std::signal(sig, CrashSignalHandler);
	}
}

#endif
