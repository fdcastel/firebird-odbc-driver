/*
 * Stub implementations for ATL-dependent functions when ATL is not available
 * This allows the driver to build without Windows transaction support
 */

#ifdef _WINDOWS
#ifndef HAVE_ATL

#include <windows.h>
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

// Global function - not in namespace
BOOL APIENTRY DllMainSetup(HINSTANCE, DWORD, LPVOID)
{
    // No-op when ATL is not available
    return TRUE;
}

#endif // !HAVE_ATL
#endif // _WINDOWS
