#ifdef NINTENDOWII
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <gccore.h>
#include <ogc/wiilaunch.h>

char* SDL_iconv_string(const char* tocode, const char* fromcode,
                       const char* inbuf, size_t inbytes, size_t* outbytes)
{
    (void)tocode;
    (void)fromcode;

    if (!inbuf) return NULL;

    size_t len = inbytes ? inbytes : strlen(inbuf);
    char* outbuf = (char*)malloc(len + 1);
    if (!outbuf) return NULL;

    memcpy(outbuf, inbuf, len);
    outbuf[len] = '\0';

    if (outbytes) *outbytes = len;
    return outbuf;
}

pid_t waitpid(pid_t pid, int *status, int options)
{
    errno = ENOSYS;   // Function not implemented
    return -1;
}

// Override execvp
int execvp(const char *file, char *const argv[])
{
    errno = ENOSYS; // default failure code
    
    // If ImGUI called execvp to "open" something, grab the path/URL
    if (argv && argv[1]) {
        const char* path = argv[1]; // normally argv[1] is the path/URL

        // Launch the Internet Channel with the URL
        int ret = WII_LaunchTitleWithArgs(
            0x0001000148414445LL, // Internet Channel title ID
            0,                      // launch flags
            path,                   // URL string
            NULL                    // reserved
        );

        return ret >= 0 ? 0 : -1; // mimic execvp success/failure
    }

    return -1; // fail if no argument provided
}

// Make this visible to C++ translation units
#ifdef __cplusplus
extern "C" {
#endif

#ifndef HAVE_TIMEGM
time_t timegm(struct tm *t)
{
    char *old_tz = getenv("TZ");
    char *old_tz_copy = NULL;
    if (old_tz)
        old_tz_copy = strdup(old_tz);

    setenv("TZ", "", 1); // UTC
    tzset();

    time_t result = mktime(t);

    // Restore original TZ
    if (old_tz_copy) {
        setenv("TZ", old_tz_copy, 1);
        free(old_tz_copy);
    } else {
        unsetenv("TZ");
    }
    tzset();

    return result;
}
#endif // HAVE_TIMEGM

#ifdef __cplusplus
} // extern "C"
#endif

// Trick to automatically declare timegm for all C++ translation units
#ifdef __cplusplus
namespace implot_stub_fix {
    extern "C" time_t timegm(struct tm* t);
}
using implot_stub_fix::timegm;
#endif
#endif