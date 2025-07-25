#ifndef ENET_TRANSPORT_ID_H
#define ENET_TRANSPORT_ID_H

#include <stddef.h>
#include <stdint.h>

typedef struct enet_transport_s enet_transport_t;
typedef struct enet_stream_s enet_stream_t;

typedef uint64_t enet_address_t;

typedef void* (*enet_transport_create_cb)(void *context, int type);
typedef int   (*enet_transport_bind_cb)(void *context, void *sockobj);
typedef int   (*enet_transport_listen_cb)(void *context, void *sockobj, int backlog);
typedef enet_stream_t* (*enet_transport_accept_cb)(void *context, void *sockobj);
typedef int   (*enet_transport_connect_cb)(void *context, void *sockobj, enet_address_t peer);
typedef int   (*enet_transport_send_cb)(void *context, void *sockobj, enet_address_t peer, const void *data, size_t length);
typedef int   (*enet_transport_receive_cb)(void *context, void *sockobj, enet_address_t *peer, void *buffer, size_t buffer_size);
typedef int   (*enet_transport_shutdown_cb)(void *context, void *sockobj, enet_address_t peer);
typedef void  (*enet_transport_destroy_cb)(void *context, void *sockobj);
typedef int (*enet_transport_set_option_cb)(void *context, void *sockobj, int option, int value);
typedef int (*enet_transport_wait_cb)(void *context, void *sockobj, uint32_t *condition, uint64_t timeout_ms);

struct enet_transport_s {
	uint64_t addr_id;
    void *context;

    enet_transport_create_cb create;
    enet_transport_bind_cb bind;
    enet_transport_listen_cb listen;
    enet_transport_accept_cb accept;
    enet_transport_connect_cb connect;
    enet_transport_send_cb send;
    enet_transport_receive_cb receive;
    enet_transport_shutdown_cb shutdown;
    enet_transport_destroy_cb destroy;
	enet_transport_set_option_cb set_option;
	enet_transport_wait_cb wait;
};

#endif // ENET_TRANSPORT_ID_H
