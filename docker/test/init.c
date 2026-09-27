/* QEMU 加载测试用最小 init（PID 1），NDK 静态编译为 aarch64。
 *
 * 功能：挂 proc/sys/dev → 打开控制台 → insmod /hack.ko（finit_module）→
 *       无论成败都把内核日志最后一段打到控制台 → 常驻不退出（PID 1 不能退）。
 * 退出 QEMU：Ctrl+A 再按 X。
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

static char klog_buf[1 << 20];

static void say(const char *msg) {
    int fd = open("/dev/kmsg", O_WRONLY);
    if (fd >= 0) {
        dprintf(fd, "qemu-init: %s\n", msg);
        close(fd);
    }
    printf("qemu-init: %s\n", msg);
    fflush(stdout);
}

static void dump_dmesg_tail(void) {
    int n = syscall(__NR_syslog, 3 /*READ_ALL*/, klog_buf, sizeof(klog_buf) - 1);
    if (n <= 0) {
        say("klogctl failed");
        return;
    }
    klog_buf[n] = 0;
    /* 只打印最后 ~120 行 */
    int lines = 0;
    char *p = klog_buf + n, *tail = klog_buf;
    while (p > klog_buf) {
        if (*--p == '\n' && ++lines > 120) {
            tail = p + 1;
            break;
        }
    }
    printf("===== dmesg tail =====\n%s\n===== dmesg end =====\n", tail);
    fflush(stdout);
}

int main(void) {
    mkdir("/proc", 0755);
    mkdir("/sys", 0755);
    mkdir("/dev", 0755);
    mount("none", "/proc", "proc", 0, NULL);
    mount("none", "/sys", "sysfs", 0, NULL);
    mount("none", "/dev", "devtmpfs", 0, NULL);

    int console = open("/dev/console", O_RDWR);
    if (console >= 0) {
        dup2(console, 0);
        dup2(console, 1);
        dup2(console, 2);
    }
    setvbuf(stdout, NULL, _IONBF, 0);

    say("init started, loading /hack.ko ...");

    int fd = open("/hack.ko", O_RDONLY);
    if (fd < 0) {
        say("open /hack.ko failed");
        dump_dmesg_tail();
    } else {
        long rc = syscall(__NR_finit_module, fd, "", 0);
        close(fd);
        if (rc == 0)
            say(">>> MODULE LOADED OK <<<");
        else
            printf("qemu-init: >>> finit_module failed, errno=%d (%m) <<<\n", (int)-rc);
        dump_dmesg_tail();
    }

    say("init done, idling (Ctrl+A then X to quit QEMU)");
    for (;;) pause();
    return 0;
}
