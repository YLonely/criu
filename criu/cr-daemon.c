#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <fcntl.h>
#include "cr-daemon.h"
#include "xmalloc.h"
#include "log.h"

#define DAEMON_SOCKET "/run/criu-daemon/daemon.sock"

int daemon_fd = -1;

int connect_daemon()
{
    int fd;
    struct sockaddr_un sun;
    fd = socket(PF_UNIX, SOCK_STREAM, 0);
    sun.sun_family = AF_UNIX;
    strncpy(sun.sun_path, DAEMON_SOCKET, sizeof(sun.sun_path) - 1);
    if (connect(fd, (struct sockaddr *)&sun, sizeof(sun)))
        return -1;
    return fd;
}

static int send_op(int fd, const char *op_type, void *arg, size_t arg_size)
{
    struct msghdr msg;
    struct iovec iov[2];
    char op[20];
    memset(&msg, 0, sizeof(struct msghdr));
    strncpy(op, op_type, sizeof(op) - 1);
    iov[0].iov_base = &op;
    iov[0].iov_len = sizeof(op);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    if (arg)
    {
        iov[1].iov_base = arg;
        iov[1].iov_len = arg_size;
        msg.msg_iovlen = 2;
    }
    return sendmsg(fd, &msg, 0);
}

int get_mnt_roots(int daemon_fd, int *mnt_id, char *root_path)
{
    struct msghdr msg;
    struct iovec iov[2];
    char buff[MAX_ROOT_LEN];
    int recv_mnt_id;

    pr_info("getting mount roots\n");

    if (send_op(daemon_fd, OP_GET_ROOT, NULL, 0))
    {
        pr_err("failed to send op %s\n", OP_GET_ROOT);
        return -1;
    }

    memset(&msg, 0, sizeof(struct msghdr));
    iov[0].iov_base = &buff;
    iov[0].iov_len = sizeof(buff);
    iov[1].iov_base = &recv_mnt_id;
    iov[1].iov_len = sizeof(recv_mnt_id);
    msg.msg_iov = iov;
    msg.msg_iovlen = 2;
    pr_info("Receiving msg of mnt_id and root_path\n");
    if (recvmsg(daemon_fd, &msg, 0))
    {
        pr_err("failed to receive msg\n");
        return -1;
    }
    else
    {
        pr_info("mnt_id %d root_path %s\n", recv_mnt_id, buff);
    }
    *mnt_id = recv_mnt_id;
    strncpy(root_path, buff, MAX_ROOT_LEN - 1);
    return 0;
}

int get_mnt_ns_fd(int daemon_fd, int mnt_id, int *ns_fd)
{
    struct msghdr msg;
    struct iovec iov[2];
    int daemon_pid, daemon_mnt_ns_fd;
    pr_info("getting mount ns fd, mnt_id %d\n", mnt_id);
    if (send_op(daemon_fd, OP_GET_MNTNS_FD, &mnt_id, sizeof(mnt_id)))
    {
        pr_err("failed to send op %s\n", OP_GET_MNTNS_FD);
        return -1;
    }
    memset(&msg, 0, sizeof(struct msghdr));
    iov[0].iov_base = &daemon_pid;
    iov[0].iov_len = sizeof(daemon_pid);
    iov[1].iov_base = &daemon_mnt_ns_fd;
    iov[1].iov_len = sizeof(daemon_mnt_ns_fd);
    msg.msg_iov = iov;
    msg.msg_iovlen = 2;
    pr_info("Receiving msg of daemon_pid and daemon_mnt_ns_fd\n");
    if (recvmsg(daemon_fd, &msg, 0))
    {
        pr_err("failed to receive msg\n");
        return -1;
    }
    else
    {
        pr_info("daemon_pid %d daemon_mnt_ns_fd %d\n", daemon_fd, daemon_mnt_ns_fd);
    }
    char daemon_ns_fd_path[MAX_ROOT_LEN];
    snprintf(daemon_ns_fd_path, sizeof(daemon_ns_fd_path), "/proc/%d/fd/%d", daemon_pid, daemon_mnt_ns_fd);
    *ns_fd = open(daemon_ns_fd_path, O_RDONLY);
    return *ns_fd == -1 ? -1 : 0;
}

int put_mnt_roots(int daemon_fd, int mnt_id)
{
    pr_info("putting mount root, mnt_id %d\n", mnt_id);
    if (send_op(daemon_fd, OP_PUT_ROOT, &mnt_id, sizeof(mnt_id)))
    {
        pr_err("failed to send op %s\n", OP_PUT_ROOT);
        return -1;
    }
    return 0;
}
