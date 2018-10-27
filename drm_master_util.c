#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <xf86drm.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>

#define DRM_HACK_SOCKET_NAME "xorg_drm_master_util"

int recv_fd(int sock)
{
    // This function does the arcane magic recving
    // file descriptors over unix domain sockets
    struct msghdr msg;
    struct iovec iov[1];
    struct cmsghdr *cmsg = NULL;
    char ctrl_buf[CMSG_SPACE(sizeof(int))];
    char data[1];

    memset(&msg, 0, sizeof(struct msghdr));
    memset(ctrl_buf, 0, CMSG_SPACE(sizeof(int)));

    iov[0].iov_base = data;
    iov[0].iov_len = sizeof(data);

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_control = ctrl_buf;
    msg.msg_controllen = CMSG_SPACE(sizeof(int));
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    recvmsg(sock, &msg, 0);

    cmsg = CMSG_FIRSTHDR(&msg);

    return *((int *) CMSG_DATA(cmsg));
}

void usage(const char *name)
{
    printf("usage: %s\n", name);
    printf("\n"
           "-h, --help:     print this help\n"
           "-s, --set:      drmSetMaster\n"
           "-d, --drop:     drmDropMaster\n"
           "-t, --test:     test\n"
           "-r, --root:     ensure i'm root\n"
           "\n");
}

void check_error(char *prog, int err)
{
    if (err) {
        fprintf(stderr, "%s: %s\n", prog, strerror(errno));
        exit(err);
    }
}

enum action {HELP, SET, DROP, TEST};

int main(int argc, char *argv[])
{
    enum action act = HELP;
    int opt, optindex = 0;
    int ret = 0;
    bool ensure_root = false;
    static struct option longopts[] = {
        {"help",  0, 0, 'h'},
        {"set",   0, 0, 's'},
        {"drop",  0, 0, 'd'},
        {"test",  0, 0, 't'},
        {"root",  0, 0, 'r'},
        {0, 0, 0, 0}
    };

    if (argv[1] == NULL) {
        usage(argv[0]);
        exit(0);
    }

    while ((opt = getopt_long(argc, argv, "hsdtr",
                  longopts, &optindex)) != EOF) {
        switch (opt) {
        case 's': act = SET;  break;
        case 'd': act = DROP; break;
        case 't': act = TEST; break;
        case 'r': ensure_root = true; break;
        }
    }

    if (optind < argc) {
        fprintf(stderr, "Error: extra parameter found.\n");
        exit(1);
    }

    if (ensure_root) {
        uid_t uid = getuid();
        uid_t euid = geteuid();

        assert(euid == 0);

        ret = setuid(euid);
        check_error("setuid", ret);

        uid = getuid();
        euid = getuid();

        assert(uid == 0);
    }

    if (act == HELP) {
        usage(argv[0]);
        exit(0);
    }

    struct sockaddr_un addr;
    int sock;

    // Create and connect a unix domain socket
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family  = AF_UNIX;
    strcpy(&addr.sun_path[1], DRM_HACK_SOCKET_NAME);
    connect(sock, (struct sockaddr *)&addr, sizeof(addr));

    int fd = recv_fd(sock);
    close(sock);

    assert(fd != 0);

    if (act == SET) {
        ret = drmSetMaster(fd);
        check_error("drmSetMaster", ret);
    } else if (act == DROP) {
        ret = drmDropMaster(fd);
        check_error("drmDropMaster", ret);
    } else if (act == TEST) {
        printf("received fd: %d\n", fd);
        dprintf(fd, "drm_master_util: writing to received fd ;)\n");
    }

    return 0;
}
