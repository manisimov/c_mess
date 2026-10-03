#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define SOCK_PATH "mysock"

struct server {
    int listen_fd;
    int conn_fd;
    int client_event_fd;
    int memory_fd;
    char *memory_p;
};

static const struct sockaddr sa = {
    AF_UNIX,
    SOCK_PATH
};

static struct server server;

static int print_server(struct server *server);
static int eventfd_memfd_server(struct server *server);

int main(void) {
    int ret;

    server.listen_fd = -1;
    server.conn_fd = -1;
    server.client_event_fd = -1;
    server.memory_fd = -1;
    server.memory_p = NULL;

    unlink(SOCK_PATH); // in case there is a stale socket from the past

    server.listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server.listen_fd < 0) {
        fprintf(stderr, "socket() failed");
        goto failed0;
    }

    ret = bind(server.listen_fd, &sa, sizeof(sa));
    if (ret != 0) {
        fprintf(stderr, "bind() failed");
        goto failed0;
    }

    ret = listen(server.listen_fd, 20);
    if (ret != 0) {
        fprintf(stderr, "listen() failed");
        goto failed1;
    }

    server.conn_fd = accept(server.listen_fd, NULL, NULL);
    if (server.conn_fd < 0) {
        fprintf(stderr, "accept() failed");
        goto failed1;
    }

    printf("Client connected. Connection fd is %d\n", server.conn_fd);

    //ret = print_server(&server);
    ret = eventfd_memfd_server(&server);
    if (ret != 0) {
        fprintf(stderr, "eventfd_memfd_server() failed");
        goto failed1;
    }

    close(server.listen_fd);
    unlink(SOCK_PATH);
    return 0;

failed1:
    close(server.listen_fd);
    unlink(SOCK_PATH);
failed0:
    return 1;
}

static int print_server(struct server *server) {
    char buf[64];

    while (1) {
        memset(buf, 0, sizeof(buf));
        ssize_t bytes = recv(server->conn_fd, buf, sizeof(buf), 0);
        if (bytes < 0) {
            fprintf(stderr, "recv() failed");
            goto failed;
        } else if (bytes == 0) {
            printf("Client closed connection, stopping server\n");
            break;
        }
        printf("Received %zd bytes\n", bytes);
        printf("Message: %s\n", buf);

        if (strncmp(buf, "exit\n", sizeof(buf)) == 0) {
            printf("Exit received, stopping server\n");
            break;
        }
    }

    return 0;
failed:
    return 1;
}

static int eventfd_memfd_server(struct server *s) {
    int res;
    ssize_t bytes;
    char buf[1];
    struct msghdr msg;
    struct iovec  iov;
    int fds_to_recv[2];

    iov.iov_base = buf;
    iov.iov_len = sizeof(buf);
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    union {
        char   buf[CMSG_SPACE(sizeof(fds_to_recv))];
        struct cmsghdr align;
    } u;
    struct cmsghdr *cmsg;

    msg.msg_control = u.buf;
    msg.msg_controllen = sizeof(u.buf);

    // receive descriptors
    bytes = recvmsg(s->conn_fd, &msg, 0);
    if (bytes < 0) {
        fprintf(stderr, "recvmsg() failed");
        goto failed0;
    }

    cmsg = CMSG_FIRSTHDR(&msg);

    if (cmsg == NULL
        || cmsg->cmsg_len != CMSG_LEN(sizeof(fds_to_recv))
        || cmsg->cmsg_level != SOL_SOCKET
        || cmsg->cmsg_type != SCM_RIGHTS)
    {
        fprintf(stderr, "cmsg format in invalid\n");
        goto failed0;
    }

    memcpy(fds_to_recv, CMSG_DATA(cmsg), sizeof(fds_to_recv));
    s->client_event_fd = fds_to_recv[0];
    s->memory_fd = fds_to_recv[1];

    printf("Received event fd descriptor %d\n", fds_to_recv[0]);
    printf("Received memory fd descriptor %d\n", fds_to_recv[1]);
    printf("Reading shared memory\n");
    struct stat st;
    res = fstat(s->memory_fd, &st);  // to determine size
    if (res < 0) {
        fprintf(stderr, "fstat() failed\n");
        goto failed0;
    }
    s->memory_p = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                       s->memory_fd, 0);
    if (s->memory_p == MAP_FAILED) {
        fprintf(stderr, "mmap() failed\n");
        goto failed1;
    }
    unsigned int step = s->memory_p[0];
    printf("Writing shared memory\n");
    for (size_t i = 0; i < st.st_size; i++) {
        s->memory_p[i] = i * step;
    }
    printf("Sleeping...\n");
    sleep(5);
    printf("Trigger client event\n");
    uint64_t value = 1;
    res = write(s->client_event_fd, &value, sizeof(uint64_t));
    if (res < 0) {
        fprintf(stderr, "write() failed\n");
        goto failed2;
    }

    munmap(s->memory_p, st.st_size);
    close(s->client_event_fd);
    close(s->memory_fd);
    return 0;

failed2:
    munmap(s->memory_p, st.st_size);
failed1:
    close(s->client_event_fd);
    close(s->memory_fd);
failed0:
    return -1;
}
