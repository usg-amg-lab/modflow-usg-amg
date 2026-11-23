/*
 * MODFLOW-USG AMG v0.2.0 - Windows MPI launcher
 * Copyright (c) 2026 Jiri Beranek
 * SPDX-License-Identifier: MIT
 *
 * Stage 1: read requested MPI process count.
 */

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>

static int same_word(const char *a, const char *b)
{
    while (*a && *b) {
        if (toupper((unsigned char)*a) !=
            toupper((unsigned char)*b))
            return 0;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static wchar_t *to_wide(const char *text)
{
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                text, -1, NULL, 0);
    UINT cp = CP_UTF8;

    if (!n) {
        cp = CP_ACP;
        n = MultiByteToWideChar(cp, 0, text, -1, NULL, 0);
    }
    if (!n)
        return NULL;

    wchar_t *result = calloc((size_t)n, sizeof(wchar_t));
    if (!result)
        return NULL;

    if (!MultiByteToWideChar(cp, 0, text, -1, result, n)) {
        free(result);
        return NULL;
    }
    return result;
}

static FILE *open_model_file(const char *name)
{
    wchar_t *wide = to_wide(name);
    if (!wide)
        return NULL;

    FILE *f = _wfopen(wide, L"r");
    free(wide);
    return f;
}

static int read_sms_processes(const char *sms)
{
    FILE *f = open_model_file(sms);
    if (!f) {
        fprintf(stderr, "AMG launcher: cannot open SMS: %s\n", sms);
        return -1;
    }

    char line[1024];
    int linmeth = -1;
    int requested = 1;

    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (isspace((unsigned char)*p))
            ++p;

        if (!*p || *p == '#' || *p == '!')
            continue;

        if (linmeth < 0) {
            double hclose, hiclose;
            int mxiter, iter1, iprsms, nonmeth, method;

            if (sscanf(p, "%lf %lf %d %d %d %d %d",
                       &hclose, &hiclose, &mxiter, &iter1,
                       &iprsms, &nonmeth, &method) == 7)
                linmeth = method;
        }

        char keyword[32];
        int count;
        double tolerance;

        if (sscanf(p, "%31s %d %lf",
                   keyword, &count, &tolerance) == 3 &&
            same_word(keyword, "AMG")) {
            if (count < 1 || count > 256) {
                fprintf(stderr,
                    "AMG launcher: invalid MPI count %d\n", count);
                fclose(f);
                return -1;
            }
            requested = count;
        }
    }

    fclose(f);

    if (linmeth == 3)
        return requested;

    return 1;
}

static int inspect_model(int argc, wchar_t **argv)
{
    if (argc < 2) {
        printf("AMG launcher: no name file argument\n");
        return 1;
    }

    wchar_t name[MAX_PATH * 4];

    if (wcslen(argv[1]) >= sizeof(name) / sizeof(name[0]))
        return -1;

    wcscpy(name, argv[1]);

    FILE *f = _wfopen(name, L"r");

    if (!f) {
        if (wcslen(name) + 4 >= sizeof(name) / sizeof(name[0]))
            return -1;

        wcscat(name, L".nam");
        f = _wfopen(name, L"r");
    }

    if (!f) {
        fwprintf(stderr,
                 L"AMG launcher: cannot open name file: %ls\n", name);
        return -1;
    }

    char line[1024];
    char type[64], filename[768];
    int unit;
    int count = 1;
    int found_sms = 0;

    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (isspace((unsigned char)*p))
            ++p;

        if (!*p || *p == '#' || *p == '!')
            continue;

        if (sscanf(p, "%63s %d %767s",
                   type, &unit, filename) == 3 &&
            same_word(type, "SMS")) {
            found_sms = 1;
            count = read_sms_processes(filename);
            break;
        }
    }

    fclose(f);

    if (!found_sms)
        fprintf(stderr, "AMG launcher: no SMS entry found\n");

    return found_sms ? count : -1;
}

int mfusg_mpi_requested_processes(void)
{
    int argc = 0;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (!argv)
        return -1;

    int result = inspect_model(argc, argv);
    LocalFree(argv);
    return result;
}

#ifdef LAUNCHER_PROBE_MAIN
int main(void)
{
    int n = mfusg_mpi_requested_processes();
    printf("AMG launcher: requested MPI processes = %d\n", n);
    return n < 0 ? 1 : 0;
}
#endif


/*
 * Called before MPI_Init.
 *
 * Under mpiexec: continue normally.
 * Standalone, N=1: continue normally.
 * Standalone, N>1: launch mpiexec, wait, propagate exit code.
 *
 * Each PEST worker has its own process environment and working directory.
 */
int mfusg_mpi_autolaunch(void)
{
    wchar_t marker[16];

    /* Child created by this launcher. */
    if (GetEnvironmentVariableW(
            L"MFUSG_AMG_MPI_CHILD", marker, 16) > 0)
        return 0;

    /* Explicit MPI launch: do not launch again. */
    if (GetEnvironmentVariableW(L"PMI_RANK", marker, 16) > 0 ||
        GetEnvironmentVariableW(L"PMI_SIZE", marker, 16) > 0 ||
        GetEnvironmentVariableW(L"PMIX_RANK", marker, 16) > 0)
        return 0;

    int requested = mfusg_mpi_requested_processes();

    if (requested < 0) {
        fprintf(stderr,
                "AMG launcher: invalid model configuration\n");
        return -1;
    }

    if (requested == 1)
        return 0;

    wchar_t mpiexec_path[4096];
    DWORD found = SearchPathW(
        NULL, L"mpiexec.exe", NULL,
        4096, mpiexec_path, NULL);

    if (found == 0 || found >= 4096) {
        const wchar_t *fallback =
            L"C:\\Program Files\\Microsoft MPI\\Bin\\mpiexec.exe";

        if (GetFileAttributesW(fallback) ==
            INVALID_FILE_ATTRIBUTES) {
            fprintf(stderr,
                "AMG launcher: MS-MPI mpiexec.exe not found\n");
            return -2;
        }

        wcscpy(mpiexec_path, fallback);
    }

    /*
     * Preserve the original Windows command line, including
     * quoted model paths and additional arguments.
     */
    const wchar_t *original = GetCommandLineW();

    size_t required =
        wcslen(mpiexec_path) +
        wcslen(original) + 100;

    wchar_t *command = calloc(required, sizeof(wchar_t));
    if (!command)
        return -3;

    swprintf(command, required,
             L"\"%ls\" -n %d %ls",
             mpiexec_path, requested, original);

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    /*
     * The marker is inherited only by the child process tree.
     * It prevents recursive spawning.
     */
    if (!SetEnvironmentVariableW(
            L"MFUSG_AMG_MPI_CHILD", L"1")) {
        free(command);
        return -4;
    }

    printf("MFUSG AMG: launching %d MPI processes\n",
           requested);
    fflush(stdout);

    BOOL started = CreateProcessW(
        mpiexec_path,
        command,
        NULL, NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si, &pi);

    free(command);

    if (!started) {
        fprintf(stderr,
                "AMG launcher: CreateProcessW failed (%lu)\n",
                (unsigned long)GetLastError());
        return -5;
    }

    CloseHandle(pi.hThread);

    DWORD waited = WaitForSingleObject(
        pi.hProcess, INFINITE);

    DWORD exit_code = 1;

    if (waited != WAIT_OBJECT_0 ||
        !GetExitCodeProcess(pi.hProcess, &exit_code)) {
        fprintf(stderr,
                "AMG launcher: MPI process wait failed\n");
        CloseHandle(pi.hProcess);
        return -6;
    }

    CloseHandle(pi.hProcess);

    printf("MFUSG AMG: MPI finished, exit code=%lu\n",
           (unsigned long)exit_code);
    fflush(stdout);

    /*
     * Parent terminates with the MPI exit code.
     * No MODFLOW files have been opened by the parent.
     */
    exit((int)exit_code);

    return 0;
}
