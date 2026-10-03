#define _GNU_SOURCE
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/uio.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <fcntl.h>

#define SOCK_PATH "mysock"
#define MEMORY_SIZE (32)

struct client {
    int conn_fd;
    int event_fd;
    int epoll_fd;
    int memory_fd;
    char *memory_p;
    struct epoll_event server_notify_event;
};

static const struct sockaddr sa = {
    AF_UNIX,
    SOCK_PATH
};

static struct client client;

int memfd_event_client(struct client *c);

static void hexdump32(const void *addr)
{
    const unsigned char *p = addr;

    for (size_t i = 0; i < 32; i++) {
        printf("%02x", p[i]);
        putchar((i % 8 == 7) ? '\n' : ' ');
    }
}

int main (void) {
    int res;

    client.conn_fd = -1;
    client.event_fd = -1;
    client.epoll_fd = -1;
    client.memory_fd = -1;
    client.memory_p = NULL;
    memset(&client.server_notify_event, 0, sizeof(struct epoll_event));

    client.conn_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client.conn_fd < 0) {
        fprintf(stderr, "socket() failed\n");
        goto failed0;
    }

    res = connect(client.conn_fd, &sa, sizeof(sa));
    if (res < 0) {
        fprintf(stderr, "connect() failed\n");
        goto failed1;
    }

    res = memfd_event_client(&client);
    if (res < 0) {
        fprintf(stderr, "memfd_event_client() failed\n");
        goto failed1;
    }

    close(client.conn_fd);
    return 0;

failed1:
    close(client.conn_fd);
failed0:
    return 1;
}

int memfd_event_client(struct client *c) {
    int res;

    c->event_fd = eventfd(0, 0);
    if (c->event_fd < 0) {
        fprintf(stderr, "eventfd() failed\n");
        goto failed0;
    }

    c->memory_fd = memfd_create("my_memfd", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (c->memory_fd < 0) {
        fprintf(stderr, "memfd_create() failed\n");
        goto failed1;
    }
    res = ftruncate(c->memory_fd, MEMORY_SIZE);
    if (res < 0) {
        fprintf(stderr, "ftruncate() failed\n");
        goto failed2;
    }
    res = fcntl(c->memory_fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_SEAL);
    if (res < 0) {
        fprintf(stderr, "fcntl() failed\n");
        goto failed2;
    }
    c->memory_p = mmap(NULL, MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
                       c->memory_fd, 0);
    if (c->memory_p == MAP_FAILED) {
        fprintf(stderr, "mmap() failed\n");
        goto failed2;
    }
    c->memory_p[0] = 1;


    c->epoll_fd = epoll_create(1);
    if (c->epoll_fd < 0) {
        fprintf(stderr, "epoll_create() failed\n");
        goto failed3;
    }

    c->server_notify_event.data.fd = c->event_fd;
    c->server_notify_event.data.ptr = NULL;
    c->server_notify_event.events = EPOLLIN;
    res = epoll_ctl(c->epoll_fd, EPOLL_CTL_ADD, c->event_fd, &c->server_notify_event);
    if (res < 0) {
        fprintf(stderr, "epoll_ctl() failed\n");
        goto failed4;
    }

    char buf[1] = {'+'};
    struct msghdr msg;
    struct iovec iov;
    int fds_to_send[2];
    fds_to_send[0] = c->event_fd;
    fds_to_send[1] = c->memory_fd;

    iov.iov_base = buf;
    iov.iov_len  = sizeof(buf);
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    union { /* Ancillary data buffer, wrapped in a union
               in order to ensure it is suitably aligned */
        char buf[CMSG_SPACE(sizeof(fds_to_send))];
        struct cmsghdr align;
    } u;
    msg.msg_control = u.buf;
    msg.msg_controllen = sizeof(u.buf);

    struct cmsghdr *cmsg;
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(fds_to_send));
    memcpy(CMSG_DATA(cmsg), fds_to_send, sizeof(fds_to_send));

    printf("Bytes to send to server:\n");
    hexdump32(c->memory_p);
    ssize_t sent;
    sent = sendmsg(c->conn_fd, &msg, 0);
    if (sent < 0) {
        fprintf(stderr, "sendmsg() failed\n");
        goto failed4;
    }

    printf("Start waiting for an event...\n");
#define MAX_EVENTS 10
    struct epoll_event recv_events[MAX_EVENTS];
    int nfds;
    nfds = epoll_wait(c->epoll_fd, recv_events, MAX_EVENTS, -1);
    if (nfds < 0) {
        fprintf(stderr, "epoll_wait() failed\n");
        goto failed4;
    }

    printf("Event received\n");
    printf("Bytes received from server:\n");
    hexdump32(c->memory_p);

    close(c->epoll_fd);
    munmap(c->memory_p, MEMORY_SIZE);
    close(c->memory_fd);
    close(c->event_fd);
    return 0;

failed4:
    close(c->epoll_fd);
failed3:
    munmap(c->memory_p, MEMORY_SIZE);
failed2:
    close(c->memory_fd);
failed1:
    close(c->event_fd);
failed0:
    return -1;
}
