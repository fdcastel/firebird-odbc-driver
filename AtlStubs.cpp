/*
 * Stub implementations for ATL-dependent functions when ATL is not available
 * This allows the driver to build without Windows transaction support
 */

#ifdef _WINDOWS

#include <windows.h>

#ifndef HAVE_ATL

#include "OdbcConnection.h"
#include "SafeEnvThread.h"

namespace OdbcJdbcLibrary {

// Stub implementations when ATL is not available

void clearAtlResource(void)
{
    // No-op when ATL is not available
}

void* MutexEnvThread::mutexLockedLevelDll = nullptr;

bool OdbcConnection::IsInstalledMsTdsInterface(void)
{
    // Return false when transaction support is not available
    return false;
}

bool OdbcConnection::enlistTransaction(void *)
{
    // Return false when transaction support is not available
    return false;
}

} // namespace OdbcJdbcLibrary

#endif // !HAVE_ATL

// DllMainSetup is needed by DllMain in Main.cpp regardless of ATL availability.
// The ATL-path files (SafeEnvThread.cpp, etc.) don't define it, so we provide
// the stub unconditionally.
BOOL APIENTRY DllMainSetup(HINSTANCE, DWORD, LPVOID)
{
    // No-op stub
    return TRUE;
}

#endif // _WINDOWS
