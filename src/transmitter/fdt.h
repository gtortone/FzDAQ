#ifndef _fdt_
#define _fdt_

#ifndef OK
#define OK 0
#endif

// error codes
#define R_TCP_SOCKET  1
#define R_TCP_BIND    2
#define R_TCP_LISTEN  3
#define R_TCP_TIMEOUT 4
#define R_TCP_ACCEPT  5
#define R_TCP_DNS     6
#define R_TCP_CONNECT 7
#define R_TCP_READ    8
#define R_TCP_WRITE   9

#define R_FDT_RECEIVER_NOT_OPEN 10

// commands
#define FDT_HELLO      0
#define FDT_DISCONNECT 0xFFFFFFFF

#define TIMEOUT 20 // timeout in seconds

#endif
