// Copyright (c) 2026 Microsoft
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "NtSetInformationFile.h"

#include <windows.h>

#include <wil/filesystem.h>
#include <wil/resource.h>
#include <wil/result.h>
#include <winerror.h>

#include <cstdlib>
#include <io.h>
#include <string>
#include <string_view>
#include <stdio.h>

static void __stdcall LogFailureToStderr(wil::FailureInfo const& failure) noexcept;
static std::wstring GetFilePath(HANDLE h);
static constexpr HRESULT ErrnoToHresult(errno_t err);

struct TmpFilePaths
{
    std::wstring original;
    std::wstring current;
};

static TmpFilePaths CreateTmpFileWithPosixDelete(bool shouldPosixDelete)
{
    // Create a temporary file. This FILE* will be intentionally leaked so
    // that its HANDLE is still open when the system is crashed.
    FILE* tmpFile;
    errno_t err = tmpfile_s(&tmpFile);
    THROW_HR_IF(ErrnoToHresult(err), err != 0);
    THROW_HR_IF_NULL(E_UNEXPECTED, tmpFile);

    // Get the OS HANDLE for the temporary file
    int fd = _fileno(tmpFile);
    THROW_HR_IF(ErrnoToHresult(errno), fd == -1);
    HANDLE tempFileHandle = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
    THROW_HR_IF(ErrnoToHresult(errno), tempFileHandle == INVALID_HANDLE_VALUE);

    std::wstring originalPath = GetFilePath(tempFileHandle);

    // If the system crashes before we issue a POSIX delete, then an empty
    // file will be left around. This is a very small window, however.
    if (shouldPosixDelete)
    {
        // To successfuly perform a POSIX delete on the file, we need to set
        // FILE_DISPOSITION_DELETE | FILE_DISPOSITION_POSIX_SEMANTICS on the
        // NT "file object" and then close all handles to the file object.
        //
        // Use ReOpenFile to get another file object for the temp file.
        // Using tempFileHandle or DuplicateHandle(..., tempFileHandle, ...)
        // will not work, since that will set the flags on the file object
        // that the FILE*'s HANDLE refers to. Since FILE* has an open
        // handle, that file object will never be closed, so the deletion
        // won't occur until after the FILE* is closed. But we want the
        // deletion to occur so that if we can't close the FILE* and the
        // system crashes, the file will have already been deleted, and it
        // will be cleaned up on the next boot.
        wil::unique_hfile reopened{ ReOpenFile(tempFileHandle, DELETE, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 0) };
        THROW_LAST_ERROR_IF(!reopened.is_valid());

        // Delete the file with POSIX semantics
        FILE_DISPOSITION_INFORMATION_EX disp{ .Flags = FILE_DISPOSITION_DELETE | FILE_DISPOSITION_POSIX_SEMANTICS };
        IO_STATUS_BLOCK ioStatusBlock;
        THROW_IF_NTSTATUS_FAILED(
            NtSetInformationFile(
                reopened.get(),
                &ioStatusBlock,
                &disp,
                sizeof(disp),
                FileDispositionInformationEx));

        // Close the handle to the second file object so that the POSIX
        // deletion is performed.
        reopened.reset();

        return TmpFilePaths
        {
            .original = originalPath,
            .current = GetFilePath(tempFileHandle),
        };
    }
    else
    {
        return TmpFilePaths{
            .original = originalPath,
            .current = originalPath,
        };
    }
}

int wmain(int argc, const wchar_t** argv) try
{
    using namespace std::literals;

    wil::SetResultLoggingCallback(LogFailureToStderr);

    const bool shouldPosixDelete = [&]()
        {
            if (argc > 1 && L"skip"sv == argv[1])
            {
                return false;
            }

            return true;
        }();

    TmpFilePaths paths = CreateTmpFileWithPosixDelete(shouldPosixDelete);

    wprintf(
        L"tmpfile has been created\noriginal path: %s\ncurrent path: %s\nPOSIX deleted? %s\n",
        paths.original.c_str(),
        paths.current.c_str(),
        shouldPosixDelete ? L"yes" : L"no");
    wprintf(L"\nNow, you should crash your system and check whether the file exists at its original path or not.\n"
        "If you let this process exit, the file will be deleted automatically whether or not you allowed POSIX deletion\n");
    fflush(stdout);

    system("PAUSE");

    fwprintf(stderr, L"This process is exiting. The tmpfile will be deleted automatically by the OS. Try again, but crash your system when prompted.");

    return 0;
}
CATCH_LOG()

static void __stdcall LogFailureToStderr(wil::FailureInfo const& failure) noexcept
{
    constexpr DWORD c_maxLogMessageSize = 2048;
    wchar_t logMessage[c_maxLogMessageSize];

    FAIL_FAST_IF_FAILED(wil::GetFailureLogString(logMessage, _countof(logMessage), failure));
    FAIL_FAST_IF(-1 == fwprintf(stderr, L"%s", logMessage));
}

static std::wstring GetFilePath(HANDLE h)
{
    wil::unique_cotaskmem_string path = wil::GetFinalPathNameByHandleW(h);
    return { path.get() };
}

static constexpr HRESULT ErrnoToHresult(errno_t err)
{
    // A random HRESULT facility with the "customer" flag set, since there
    // isn't a facility for the MSVC CRT.
    const uint16_t FACILITY_CUSTOMER_CRT = (0b1 << 16) | 0x1898;
    return MAKE_HRESULT(1, FACILITY_CUSTOMER_CRT, static_cast<uint16_t>(err));
}
