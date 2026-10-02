#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "main.h"

extern UART_HandleTypeDef huart1;

int _write(int file, char *ptr, int len) {
    if (file == STDOUT_FILENO || file == STDERR_FILENO) {
        HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 0xFFFF);
        return len;
    }
    errno = EBADF;
    return -1;
}

int _read(int file, char *ptr, int len) {
    if (file == STDIN_FILENO) {
        if (HAL_UART_Receive(&huart1, (uint8_t *)ptr, 1, 0xFFFF) == HAL_OK) {
            return 1;
        }
        return 0;
    }
    errno = EBADF;
    return -1;
}

int _close(int file) {
    return -1;
}

int _fstat(int file, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file) {
    return 1;
}

int _lseek(int file, int ptr, int dir) {
    return 0;
}

void *_sbrk(ptrdiff_t incr) {
    extern char _end;
    static char *heap_end;
    char *prev_heap_end;

    if (heap_end == 0) {
        heap_end = &_end;
    }
    prev_heap_end = heap_end;
    heap_end += incr;

    return (void *)prev_heap_end;
}

int _getpid(void) {
    return 1;
}

int _kill(int pid, int sig) {
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

void _exit(int status) {
    _write(STDERR_FILENO, "exit\n", 5);
    while (1) {
    }
}
