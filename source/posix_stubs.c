#ifdef NINTENDOWII
#include <errno.h>
#include <sys/types.h>

pid_t waitpid(pid_t pid, int *status, int options)
{
    errno = ENOSYS;   // Function not implemented
    return -1;
}

int execvp(const char *file, char *const argv[])
{
    errno = ENOSYS;
    return -1;
}
#endif