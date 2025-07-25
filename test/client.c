#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "udp.c";
#include "enet.h";

int main() {
#ifdef _WIN32
    // windows 下初始化winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return -1;
    }
#endif
	enet_transport_t transport;
    enet_transport_udp_init(&transport);
    udp_context_t *ctx = transport.create(NULL, 0);
    if (!ctx) {
        printf("Failed to create UDP transport\n");
        return -1;
    }
    transport.bind(NULL, ctx);
	// 构造peer地址：port在高32位，ip在低32位
    uint32_t ip = (127 << 24) | 1; // 127.0.0.1，注意字节序
    uint64_t peer_addr = ((uint64_t)12346 << 32) | ip;
    transport.connect(NULL, ctx, peer_addr);
	
	ENetHost* client = { 0 };
	client = enet_host_create(&transport, ctx, 1, 2, 0, 0);
	if (client == NULL) {
		fprintf(stderr,
			"An error occurred while trying to create an ENet client host.\n");
		exit(EXIT_FAILURE);
	}

	ENetAddress address = { 0 };
	enet_address_set_host(&address, transport.addr_id);
	ENetPeer* peer = enet_host_connect(client, &address, 2, 0);
		if (peer == NULL) {
			fprintf(stderr,
				"No available peers for initiating an ENet connection.\n");
			exit(EXIT_FAILURE);
		}

		ENetEvent event = { 0 };
		if (enet_host_service(client, &event, 5000) > 0 &&
		event.type == ENET_EVENT_TYPE_CONNECT) {
		puts("Connection succeeded.");
	} else {
		/* Either the 5 seconds are up or a disconnect event was */
		/* received. Reset the peer in the event the 5 seconds   */
		/* had run out without any significant event.            */
		enet_peer_reset(&peer);
		puts("Connection failed.");
	}

	enet_host_service(client, &event, 5000);
	enet_peer_disconnect(&peer, 0);

	uint8_t disconnected = false;
	/* Allow up to 3 seconds for the disconnect to succeed
	* and drop any packets received packets.
	*/
	while (enet_host_service(client, &event, 3000) > 0) {
		switch (event.type) {
		case ENET_EVENT_TYPE_RECEIVE:
			enet_packet_destroy(event.packet);
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			puts("Disconnection succeeded.");
			disconnected = true;
			break;
		}
	}

	// Drop connection, since disconnection didn't successed
	if (!disconnected) {
		enet_peer_reset(&peer);
	}

	enet_host_destroy(client);
	enet_deinitialize();
#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
