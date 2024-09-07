#include "httpd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>

#define CONNMAX 1000

static int listenfd, clients[CONNMAX], clientfd;
static void error(char *);
static void startServer(const char *);
static void respond(int);

char *method, *uri, *qs, *prot;
char *payload;    
int payload_size;

header_t reqhdr[17];  // Array to store HTTP headers
static char *buf;     // Buffer to store the request

// Function to get a request header by name
char *request_header(const char* name) {
    header_t *h = reqhdr;
    while (h->name) {
        if (strcmp(h->name, name) == 0) {
            return h->value;
        }
        h++;
    }
    return NULL;
}

// Function to analyze HTTP request
void analyze_http(char* buf, int rcvd) {
    buf[rcvd] = '\0';
    method = strtok(buf,  " \t\r\n");
    uri = strtok(NULL, " \t");
    prot = strtok(NULL, " \t\r\n");

    fprintf(stderr, "\x1b[32m + [%s] %s\x1b[0m\n", method, uri);
    
    if ((qs = strchr(uri, '?'))) {
        *qs++ = '\0'; // split URI
    } else {
        qs = uri - 1; // empty string
    }

    header_t *h = reqhdr;
    char *k, *v, *t;

    while (h < reqhdr + 16) {
        k = strtok(NULL, "\r\n: \t"); if (!k) break;
        v = strtok(NULL, "\r\n"); while (*v && *v == ' ') v++;
        h->name = k;
        h->value = v;
        h++;
        fprintf(stderr, "[H] %s: %s\n", k, v);
        t = v + 1 + strlen(v);
        if (t[1] == '\r' && t[2] == '\n') break;
    }

    // Look for Content-Length header
    char *content_length = request_header("Content-Length");
    payload_size = content_length ? atol(content_length) : (rcvd - (t - buf));

    payload = buf + rcvd - payload_size + 1;
    if (payload_size < 100) {
        fprintf(stderr, "[P] %d bytes: %s\n", payload_size, payload);
    }
}

void serve_forever(const char *PORT) {
    struct sockaddr_in clientaddr;
    socklen_t addrlen;
    int slot = 0;

    printf("Server started %shttp://127.0.0.1:%s%s\n", "\033[92m", PORT, "\033[0m");

    // Initialize clients
    for (int i = 0; i < CONNMAX; i++) clients[i] = -1;
    
    startServer(PORT);

    // Ignore SIGCHLD to avoid zombie threads
    signal(SIGCHLD, SIG_IGN);

    // Accept connections
    while (1) {
        addrlen = sizeof(clientaddr);
        clients[slot] = accept(listenfd, (struct sockaddr *)&clientaddr, &addrlen);

        if (clients[slot] < 0) {
            perror("accept() error");
        } else {
            if (fork() == 0) {
                respond(slot);
                exit(0);
            }
        }

        while (clients[slot] != -1) slot = (slot + 1) % CONNMAX;
    }
}

void startServer(const char *port) {
    struct addrinfo hints, *res, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    if (getaddrinfo(NULL, port, &hints, &res) != 0) {
        perror("getaddrinfo() error");
        exit(1);
    }

    for (p = res; p != NULL; p = p->ai_next) {
        int option = 1;
        listenfd = socket(p->ai_family, p->ai_socktype, 0);
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
        if (listenfd == -1) continue;
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
    }

    if (p == NULL) {
        perror("socket() or bind()");
        exit(1);
    }

    freeaddrinfo(res);

    if (listen(listenfd, 1000000) != 0) {
        perror("listen() error");
        exit(1);
    }
}

void respond(int n) {
    int rcvd;
    buf = malloc(65535);
    rcvd = recv(clients[n], buf, 65535, 0);
    
    if (rcvd < 0) {
        fprintf(stderr, "recv() error\n");
    } else if (rcvd == 0) {
        fprintf(stderr, "Client disconnected unexpectedly.\n");
    } else {
        analyze_http(buf, rcvd);
    }

    clientfd = clients[n];
    dup2(clientfd, STDOUT_FILENO);
    close(clientfd);

    // Call router
    route();

    fflush(stdout);
    shutdown(STDOUT_FILENO, SHUT_WR);
    close(STDOUT_FILENO);

    shutdown(clientfd, SHUT_RDWR);
    close(clientfd);
    clients[n] = -1;
}
