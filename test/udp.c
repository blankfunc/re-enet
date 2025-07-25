#include "transport.h"

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef SOCKET socket_t;
  #define close_socket closesocket
  #define socklen_t int
  static int winsock_init_done = 0;
  static void winsock_init() {
      if (!winsock_init_done) {
          WSADATA wsaData;
          WSAStartup(MAKEWORD(2,2), &wsaData);
          winsock_init_done = 1;
      }
  }
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <errno.h>
  typedef int socket_t;
  #define close_socket close
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

void print_binary(const void *data, size_t length, bool is_r) {
    const unsigned char *p = (const unsigned char*)data;
	if (is_r) {
        printf("[Receiver] ");
    } else {
        printf("[Send] ");
    }
    printf("Data (%zu bytes):", length);
    for (size_t i = 0; i < length; i++) {
        if (i % 16 == 0) printf("\n  ");
        printf("%02X ", p[i]);
    }
    printf("\n");
}

typedef struct udp_context_s {
    socket_t sockfd;
    struct sockaddr_in peer_addr;
    int peer_addr_set;
} udp_context_t;

static int set_nonblocking(socket_t sockfd) {
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(sockfd, FIONBIO, &mode);
#else
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
#endif
}

// create
static void* udp_create(void *context, int type) {
    (void)context; (void)type;
#ifdef _WIN32
    winsock_init();
#endif
    udp_context_t *ctx = (udp_context_t*)malloc(sizeof(udp_context_t));
    if (!ctx) {
		return NULL;
	}
    ctx->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctx->sockfd
#ifdef _WIN32
        == INVALID_SOCKET
#endif
    ) {
        free(ctx);
        return NULL;
    }
    // set_nonblocking(ctx->sockfd);
    ctx->peer_addr_set = 0;
    memset(&ctx->peer_addr, 0, sizeof(ctx->peer_addr));
    return ctx;
}

// bind
static int udp_bind(void *context, void *sockobj) {
    (void)context;
    udp_context_t *ctx = (udp_context_t*)sockobj;
    // bind to any local port
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0; // let OS assign port
    int ret = bind(ctx->sockfd, (struct sockaddr*)&addr, sizeof(addr));
#ifdef _WIN32
    if (ret == SOCKET_ERROR) return -1;
#else
    if (ret < 0) return -1;
#endif
    return 0;
}

// listen - UDP不支持监听
static int udp_listen(void *context, void *sockobj, int backlog) {
    (void)context; (void)sockobj; (void)backlog;
    // UDP无listen，返回错误
    return -1;
}

// accept - UDP无连接不支持accept
static enet_stream_t* udp_accept(void *context, void *sockobj) {
    (void)context; (void)sockobj;
    return NULL;
}

// connect - 只是保存目标地址，方便send
static int udp_connect(void *context, void *sockobj, enet_address_t peer) {
    (void)context;
    udp_context_t *ctx = (udp_context_t*)sockobj;
    uint32_t ip = (uint32_t)(peer & 0xFFFFFFFF);
    uint16_t port = (uint16_t)((peer >> 32) & 0xFFFF);
    memset(&ctx->peer_addr, 0, sizeof(ctx->peer_addr));
    ctx->peer_addr.sin_family = AF_INET;
    ctx->peer_addr.sin_addr.s_addr = htonl(ip);
    ctx->peer_addr.sin_port = htons(port);
    printf("With send to IP: %s, port: %d\n", inet_ntoa(ctx->peer_addr.sin_addr), ntohs(ctx->peer_addr.sin_port));
    ctx->peer_addr_set = 1;
    return 0;
}

// send
static int udp_send(void *context, void *sockobj, enet_address_t peer, const void *data, size_t length) {
    (void)context;
    udp_context_t *ctx = (udp_context_t*)sockobj;
	// printf("Send to IP: %s, port: %d\n", inet_ntoa(ctx->peer_addr.sin_addr), ntohs(ctx->peer_addr.sin_port));
    // if (ctx->peer_addr.sin_family == AF_INET) {
    // 	printf("Send to IP: %s, port: %d\n", inet_ntoa(ctx->peer_addr.sin_addr), ntohs(ctx->peer_addr.sin_port));
	// } else {
	// 	printf("peer_addr not initialized or invalid\n");
	// }
	
	struct sockaddr_in dest;
    // if (peer == 0 && ctx->peer_addr_set) {
    //     // use saved peer addr
    //     dest = ctx->peer_addr;
    // } else {
    //     uint32_t ip = (uint32_t)(peer & 0xFFFFFFFF);
    //     uint16_t port = (uint16_t)((peer >> 32) & 0xFFFF);
    //     memset(&dest, 0, sizeof(dest));
    //     dest.sin_family = AF_INET;
    //     dest.sin_addr.s_addr = htonl(ip);
    //     dest.sin_port = htons(port);
    // }
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    // inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr);
	dest.sin_addr.s_addr = inet_addr("127.0.0.1");
    dest.sin_port = htons(12345);
	print_binary(data, length, false);
	printf("sockfd = %d\n", ctx->sockfd);
	printf("Send to IP: %s, port: %d\n", inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));
    int sent = sendto(ctx->sockfd, (const char*)data, length, 0,
                          (struct sockaddr*)&dest, sizeof(dest));
	printf("sendto returned %d\n", sent);
#ifdef _WIN32
    if (sent == SOCKET_ERROR) return -1;
#else
    if (sent < 0) return -1;
#endif
    return (int)sent;
}

// receive
static int udp_receive(void *context, void *sockobj, enet_address_t *peer, void *buffer, size_t buffer_size) {
    (void)context;
    udp_context_t *ctx = (udp_context_t*)sockobj;
    struct sockaddr_in src_addr;
    socklen_t addrlen = sizeof(src_addr);
    int recvd = recvfrom(ctx->sockfd, (char*)buffer, buffer_size, 0,
                             (struct sockaddr*)&src_addr, &addrlen);
#ifdef _WIN32
    if (recvd == SOCKET_ERROR) return -1;
#else
    if (recvd < 0) return -1;
#endif
    if (peer) {
        uint32_t ip = ntohl(src_addr.sin_addr.s_addr);
        uint16_t port = ntohs(src_addr.sin_port);
        *peer = ((uint64_t)port << 32) | ip;
    }

	print_binary(buffer, buffer_size, true);
    return (int)recvd;
}

// shutdown
static int udp_shutdown(void *context, void *sockobj, enet_address_t peer) {
    (void)context; (void)peer;
    udp_context_t *ctx = (udp_context_t*)sockobj;
#ifdef _WIN32
    shutdown(ctx->sockfd, SD_BOTH);
#else
    shutdown(ctx->sockfd, SHUT_RDWR);
#endif
    return 0;
}

// destroy
static void udp_destroy(void *context, void *sockobj) {
    (void)context;
    udp_context_t *ctx = (udp_context_t*)sockobj;
    if (!ctx) return;
    close_socket(ctx->sockfd);
    free(ctx);
}

// set_option - 简单不支持
static int udp_set_option(void *context, void *sockobj, int option, int value) {
    (void)context; (void)sockobj; (void)option; (void)value;
    return 0; // no-op
}

// wait - 简单实现select超时检测读写
static int udp_wait(void *context, void *sockobj, uint32_t *condition, uint64_t timeout_ms) {
    (void)context;
    udp_context_t *ctx = (udp_context_t*)sockobj;
    fd_set readfds, writefds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    if ((*condition & 1) != 0) FD_SET(ctx->sockfd, &readfds);
    if ((*condition & 2) != 0) FD_SET(ctx->sockfd, &writefds);

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    int nfds = (int)(ctx->sockfd + 1);
    int ret = select(nfds, &readfds, &writefds, NULL, &tv);
    if (ret < 0) return -1;
    *condition = 0;
    if (FD_ISSET(ctx->sockfd, &readfds)) *condition |= 1;
    if (FD_ISSET(ctx->sockfd, &writefds)) *condition |= 2;
    return ret;
}

// 初始化 enet_transport_s
void enet_transport_udp_init(enet_transport_t *transport) {
    if (!transport) return;
    transport->addr_id = 1; // 自定义传输ID
    transport->context = NULL;
    transport->create = udp_create;
    transport->bind = udp_bind;
    transport->listen = udp_listen;
    transport->accept = udp_accept;
    transport->connect = udp_connect;
    transport->send = udp_send;
    transport->receive = udp_receive;
    transport->shutdown = udp_shutdown;
    transport->destroy = udp_destroy;
    transport->set_option = udp_set_option;
    transport->wait = udp_wait;
}
