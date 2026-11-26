#pragma once

namespace DX9GF {
	/// <summary>
	/// Installs a handler for hard crashes (access violations, aborts, ...) that writes a
	/// report - exception code, fault details and a stack trace - to "<exe path>.crash.log"
	/// and tells the user. Hard crashes bypass C++ catch and would otherwise kill the process
	/// silently, which makes runs on machines we can't debug (sandbox/CI/end users) a mystery.
	///
	/// Windows: structured exception filter; frames are logged as module + RVA so they can
	/// be resolved against the build's PDBs. Other platforms: POSIX signal handlers with a
	/// backtrace where execinfo.h is available (not yet exercised by CI).
	/// </summary>
	void InstallCrashHandler();
}
