# Re-Enet
This project attempts to [implement improvement suggestions](https://github.com/zpl-c/enet/issues/69) for [enet](https://github.com/zpl-c/enet).

# Goals
+ Remove internal binding to SOCK and allow Transport to be fully implemented by users.

# Progress
1. Attempt to logically separate ENET.

2. Write [Transport abstract structure](./enet/transport.h)

3. Attempt preliminary implementation and [attempt UDP binding on the structure](./test/udp.c)

> At present, it cannot be used. The message received after the handshake packet is successful cannot be processed correctly. We are currently investigating the cause.