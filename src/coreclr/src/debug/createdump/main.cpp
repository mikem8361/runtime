// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "createdump.h"

//
// Main entry point
//
int __cdecl main(const int argc, const char* argv[])
{
#ifdef HOST_UNIX
    int exitCode = PAL_InitializeDLL();
    if (exitCode != 0)
    {
        fprintf(stderr, "PAL initialization FAILED %d\n", exitCode);
        return exitCode;
    }
#endif

    bool result = CreateDump(argc, argv);

#ifdef HOST_UNIX
    PAL_TerminateEx(exitCode);
#endif
    return result ? 0 : -1;
}
