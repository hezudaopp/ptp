
/* constants_dep.h */

#ifndef CONSTANTS_DEP_H
#define CONSTANTS_DEP_H

/**
*\file
* \brief Plateform-dependent constants definition
*
* This header defines all includes and constants which are plateform-dependent
*
* ptpdv2 is only implemented for linux, NetBSD and FreeBSD
 */

/* platform dependent */

#if !defined(linux) && !defined(__NetBSD__) && !defined(__FreeBSD__)
#error Not ported to this architecture, please update.
#endif

#ifdef	linux
#include<netinet/in.h>
//#include<net/if.h>
//#include<net/if_arp.h>
#include<linux/if.h>
#include<linux/if_arp.h>
/*CHANGE copy from net/if*/
#define IF_NAMESIZE	16
/*END*/
#define IFACE_NAME_LENGTH         IF_NAMESIZE
#define NET_ADDRESS_LENGTH        INET_ADDRSTRLEN

#define IFCONF_LENGTH 10

#include<endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define PTPD_LSBF
#elif __BYTE_ORDER == __BIG_ENDIAN
#define PTPD_MSBF
#endif
#endif /* linux */


#if defined(__NetBSD__) || defined(__FreeBSD__)
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <net/if.h>
# include <net/if_dl.h>
# include <net/if_types.h>
# if defined(__FreeBSD__)
#  include <net/ethernet.h>
#  include <sys/uio.h>
# else
#  include <net/if_ether.h>
# endif
# include <ifaddrs.h>
# define IFACE_NAME_LENGTH         IF_NAMESIZE
# define NET_ADDRESS_LENGTH        INET_ADDRSTRLEN

# define IFCONF_LENGTH 10

# define adjtimex ntp_adjtime

# include <machine/endian.h>
# if BYTE_ORDER == LITTLE_ENDIAN
#   define PTPD_LSBF
# elif BYTE_ORDER == BIG_ENDIAN
#   define PTPD_MSBF
# endif
#endif

#define CLOCK_IDENTITY_LENGTH 8
#define ADJ_FREQ_MAX  512000

/* UDP/IPv4 dependent */

#define SUBDOMAIN_ADDRESS_LENGTH  4
#define PORT_ADDRESS_LENGTH       2
#define PTP_UUID_LENGTH			  6
#define CLOCK_IDENTITY_LENGTH	  8
#define FLAG_FIELD_LENGTH		  2

#define PACKET_SIZE  300 //ptpdv1 value kept because of use of TLV...

#define PTP_EVENT_PORT    319
#define PTP_GENERAL_PORT  320

//预留多播地址   (224.0.1.0-238.255.255.255)可用于全球范围内或网络协议
#define DEFAULT_PTP_DOMAIN_ADDRESS     "224.0.1.129"
//局部多播地址 （224.0.0.-224.0.0.255)为路由协议和其它用途保留的地址，路由器不转发此范围的IP包
#define PEER_PTP_DOMAIN_ADDRESS     "224.0.0.107"

#define MM_STARTING_BOUNDARY_HOPS  0x7fff

/* others */

#define SCREEN_BUFSZ  128
#define SCREEN_MAXSZ  80

#define MAXTIMESTR 32
#endif /*CONSTANTS_DEP_H_*/

/*802.3*/
#define NON_PEER_MAC_ADDRESS  {0x01, 0x1B, 0x19, 0x00, 0x00, 0x00}
#define PEER_MAC_ADDRESS  {0x01, 0x80, 0xC2, 0x00, 0x00, 0x0E}
#define MAC_LENGTH 6
#define MAC_HEADER_LEN 14
#define MAC_TYPE_POS 12

#ifndef ETH_P_1588
# define ETH_P_1588 0x88F7
#endif
