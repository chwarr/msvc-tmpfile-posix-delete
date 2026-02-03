# Automatically cleaning up tmpfile files

This is example code showing how to arrange to have files created by MSVC's
`tmpfile` function automatically deleted, even if the system crashes or for
session 0 processes.

It achieves this by performing a "POSIX deletion" on the tmpfile before
using it.

Files created by `tmpfile` are created with `FILE_FLAG_DELETE_ON_CLOSE`,
which causes the OS to automatically delete the file when all handles to it
are closed. If you call `fclose()` on the `FILE*` returned by `tmpfile()`,
the file will be deleted. If you forget to call `fclose()`, the kernel's
process termination cleanup will close all handles the process has open, and
the file will be deleted.

However, if the process exits without being terminated (e.g., the system
crashes or it is a session 0 process during shutdown), the NT "file object"
that has `FILE_FLAG_DELETE_ON_CLOSE` set will not be closed, and the file
will not be deleted.

When a file is POSIX deleted, it is moved to the special NTFS directory
`$Extend\$Deleted` and is no longer present in the normally visible file
system. When all other NT file objects to the file are closed, NTFS will
fully delete the file. When a NTFS volume is mounted, any files left over in
`$Extend\$Deleted` are automatically deleted.

If the file is POSIX deleted and the system crashes, the file will be
present in the `$Extend\$Deleted` directory. The on-mount deletion behavior
will clean up these left over files automatically.

Windows has supported POSIX deletion since circa Windows 10
version 1809 (October 2018).

## Alternatives

If your can restructure your code to use shared memory instead of temporary
FILE* objects, this entire class of problem goes away.

## Windows shutdown

During shutdown, processes in session 0 are informed of shutdown, but **they
_are not_ terminated**. Thus files marked with `FILE_FLAG_DELETE_ON_CLOSE`
will not be automatically deleted by the OS unless the session 0 process
intentionally terminates itself. Most session 0 processes do not terminate
themselves during shutdown. Why terminate yourself when the entire universe
will shortly be disappearing?

From _Windows Internals, Part 2_; Allievi, Russinovich, Ionescu, &amp;
Solomon; 7th edition; chapter 12, "Startup and Shutdown", emphasis mine:

> At this point, all the processes in the interactive user's session have
> been terminated. Wininit next calls ExitWindowsEx, which this time
> executes within the system process context. This causes Wininit to send a
> message to the Csrss part of session 0, where the services live. Csrss
> then looks at all the processes belonging to the system context and
> performs and sends the `WM_QUERYENDSESSION`/`WM_ENDSESSION` messages to
> GUI threads (as before). Instead of sending `CTRL_LOGOFF_EVENT`, however,
> it sends `CTRL_SHUTDOWN_EVENT` to console applications that have
> registered control handlers. Note that the SCM is a console program that
> registers a control handler. When it receives the shutdown request, it in
> turn sends the service shutdown control message to all services that
> registered for shutdown notification&hellip;
>
> Although Csrss performs the same timeouts as when it was terminating the
> user processes, it doesn't display any dialog boxes and **doesn't kill any
> processes**. (The registry values for the system process timeouts are
> taken from the default user profile.) These timeouts simply allow system
> processes a chance to clean up and exit before the system shuts down.
> **Therefore, many system processes are in fact still running when the
> system shuts down, such as Smss, Wininit, Services, and LSASS.**
>
> &hellip; Wininit finishes the shutdown process by shutting down LogonUi
> and calling the executive subsystem function NtShutdownSystem. This
> function calls the function PoSetSystemPowerState to orchestrate the
> shutdown of drivers and the rest of the executive subsystems (Plug and
> Play manager, power manager, executive, I/O manager, configuration
> manager, and memory manager).

# Building

You will need CMake 4 or later and the Visual Studio 2022 or later toolchain
installed. I recommend using the Ninja generator.

1. Launch a Visual Studio developer prompt.
1. Clone this repository:
   ```powershell
   git clone --recursive https://github.com/chwarr/msvc-tmpfile-posix-delete
   ```
1. Generate build files and build via CMake.
   ```powershell
   cd msvc-tmpfile-posix-delete
   cmake -S . -B build -G Ninja
   cmake --build build
   ```
1. Run the sample program. This path works for Ninja.
   ```powershell
   .\build\msvc-tmpfile-posix-delete.exe
   ```

# Running

1. [Configure your system to crash from the keyboard.][crash]
1. Reboot your system.
1. Run the executable.
1. You should see output telling you the original path to the temp file that
   was created. Note down this path.
1. Crash your system.
1. After a reboot, check whether the temp file exists at its original path.
   It should not exist.

To see what happens when POSIX deletion is not used, run the example again
with the argument "skip":

```powershell
.\build\msvc-tmpfile-posix-delete.exe skip
```

When POSIX deletion _is not done_, the temp file will still exist after the
system crashes.

[crash]: https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/forcing-a-system-crash-from-the-keyboard

# Support

No support is provided for this sample code.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

# License

Copyright 2026 Microsoft

Released under the terms of the MIT license.
