#ifndef __LWIP_OPTS_H__
#define __LWIP_OPTS_H__

#define MEMP_MEM_MALLOC             0
#define MEM_LIBC_MALLOC             0
#define MEM_ALIGNMENT               4
#define MEM_SIZE                   (32 * 1024)

#define LWIP_ARP                    1
#define LWIP_ICMP                   1
#define LWIP_DHCP                   1
#define LWIP_DNS                    1
#define LWIP_UDP                    1
#define LWIP_TCP                    1

#define LWIP_NETIF_HOSTNAME         1
#define LWIP_NETIF_API              1
#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_LINK_CALLBACK    1
#define LWIP_NETIF_REMOVE_CALLBACK  1
#define LWIP_NETIF_LOOPBACK         1

#define LWIP_SOCKET                 1
#define LWIP_TCP_KEEPALIVE          60
#define LWIP_SO_SNDTIMEO            1
#define LWIP_SO_RCVTIMEO            1
#define LWIP_SO_LINGER              1
#define SO_REUSE                    1

#define LWIP_STATS                  0
#define LWIP_DEBUG                  0
#define LWIP_PROVIDE_ERRNO          1

#define MEMP_NUM_TCP_SEG            128
#define TCP_SND_BUF                (16 * 1024)

#endif
