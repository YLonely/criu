#ifndef __CR_DAEMON_H__
#define __CR_DAEMON_H__

#define OP_GET_ROOT "OP_GET_ROOT"
#define OP_GET_MNTNS_FD "OP_GET_FD"
#define OP_PUT_ROOT "OP_PUT_ROOT"

#define MAX_ROOT_LEN 512

int get_mnt_roots(int daemon_fd, int *mnt_id, char *root_path);
int get_mnt_ns_fd(int daemon_fd, int mnt_id, int *ns_fd);
int put_mnt_roots(int daemon_fd, int mnt_id);
int connect_daemon();
extern int daemon_fd;
#define DAEMON_READY (daemon_fd != -1)

#endif