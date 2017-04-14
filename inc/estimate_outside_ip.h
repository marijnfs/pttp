#define _GNU_SOURCE     /* To get defns of NI_MAXSERV and NI_MAXHOST */
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <asm/types.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#define BUFSIZE 8192

struct route_info{
  u_int dstAddr;
  u_int srcAddr;
  u_int gateWay;
  char ifName[IF_NAMESIZE];
};

inline int readNlSock(int sockFd, char *bufPtr, int seqNum, int pId) {
  struct nlmsghdr *nlHdr;
  int readLen = 0, msgLen = 0;
  do {
    /* Receive response from the kernel */
    if((readLen = recv(sockFd, bufPtr, BUFSIZE - msgLen, 0)) < 0)
      {
	perror("SOCK READ: ");
	return -1;
      }
    nlHdr = (struct nlmsghdr *)bufPtr;
    /* Check if the header is valid */
    if((0 == NLMSG_OK(nlHdr, readLen)) || (NLMSG_ERROR == nlHdr->nlmsg_type))
      {
	perror("Error in received packet");
	return -1;
      }
    /* Check if it is the last message */
    if (NLMSG_DONE == nlHdr->nlmsg_type)
      {
	break;
      }
    /* Else move the pointer to buffer appropriately */
    bufPtr += readLen;
    msgLen += readLen;
    /* Check if its a multi part message; return if it is not. */
    if (0 == (nlHdr->nlmsg_flags & NLM_F_MULTI)) {
      break;
    }
  } while((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));
  return msgLen;
}

inline char *ntoa(int addr)
{
  static char buffer[18];
  sprintf(buffer, "%d.%d.%d.%d",
	  (addr & 0x000000FF)      ,
	  (addr & 0x0000FF00) >>  8,
	  (addr & 0x00FF0000) >> 16,
	  (addr & 0xFF000000) >> 24);
  return buffer;
}

/* For parsing the route info returned */
inline void parseRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo)
{
  struct rtmsg *rtMsg;
  struct rtattr *rtAttr;
  int rtLen;
  char *tempBuf = NULL;

  tempBuf = (char *)malloc(100);
  rtMsg = (struct rtmsg *)NLMSG_DATA(nlHdr);

  /* If the route is not for AF_INET or does not belong to main routing table
     then return. */
  if((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN))
    return;

  /* get the rtattr field */
  rtAttr = (struct rtattr *)RTM_RTA(rtMsg);
  rtLen = RTM_PAYLOAD(nlHdr);
  for (; RTA_OK(rtAttr,rtLen); rtAttr = RTA_NEXT(rtAttr,rtLen)) {
    switch(rtAttr->rta_type) {
    case RTA_OIF:
      if_indextoname(*(int *)RTA_DATA(rtAttr), rtInfo->ifName);
      break;
    case RTA_GATEWAY:
      rtInfo->gateWay = *(u_int *)RTA_DATA(rtAttr);
      break;
    case RTA_PREFSRC:
      rtInfo->srcAddr = *(u_int *)RTA_DATA(rtAttr);
      break;
    case RTA_DST:
      rtInfo->dstAddr = *(u_int *)RTA_DATA(rtAttr);
      break;
    }
  }
  free(tempBuf);
}

inline std::string get_ip_from_interface(std::string interface) {
  std::string ip;
  
  struct ifaddrs *ifaddr, *ifa;
  int family, s, n;
  char host[NI_MAXHOST];

  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    return "";
  }

  /* Walk through linked list, maintaining head pointer so we
     can free list later */

  for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
    if (ifa->ifa_addr == NULL)
      continue;

    family = ifa->ifa_addr->sa_family;

    /* Display interface name and family (including symbolic
       form of the latter for the common families) */
    
    /*printf("%-8s %s (%d)\n",
	   ifa->ifa_name,
	   (family == AF_PACKET) ? "AF_PACKET" :
	   (family == AF_INET) ? "AF_INET" :
	   (family == AF_INET6) ? "AF_INET6" : "???",
	   family);*/

    /* For an AF_INET* interface address, display the address */

    if (family == AF_INET || family == AF_INET6) {
      s = getnameinfo(ifa->ifa_addr,
		      (family == AF_INET) ? sizeof(struct sockaddr_in) :
		      sizeof(struct sockaddr_in6),
		      host, NI_MAXHOST,
		      NULL, 0, NI_NUMERICHOST);
      if (s != 0) {
	//printf("getnameinfo() failed: %s\n", gai_strerror(s));
	return ip;
      }
      
      if (interface == ifa->ifa_name && std::string(host).find("192.168.") == std::string::npos) {
	ip = host;
      }
      //printf("\t\taddress: <%s>\n", host);

    } else if (family == AF_PACKET && ifa->ifa_data != NULL) {
      struct rtnl_link_stats *stats = reinterpret_cast<rtnl_link_stats*>(ifa->ifa_data);

      /*printf("\t\ttx_packets = %10u; rx_packets = %10u\n"
	     "\t\ttx_bytes   = %10u; rx_bytes   = %10u\n",
	     stats->tx_packets, stats->rx_packets,
	     stats->tx_bytes, stats->rx_bytes);*/
    }
  }

  freeifaddrs(ifaddr);
  return ip;
}

inline std::string get_interface()
{
  std::string interface;
  struct nlmsghdr *nlMsg;
  struct route_info *rtInfo;
  char msgBuf[BUFSIZE];

  int sock, len, msgSeq = 0;

  /* Create Socket */
  if((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0)
    perror("Socket Creation: ");

  /* Initialize the buffer */
  memset(msgBuf, 0, BUFSIZE);

  /* point the header and the msg structure pointers into the buffer */
  nlMsg = (struct nlmsghdr *)msgBuf;

  /* Fill in the nlmsg header*/
  nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message.
  nlMsg->nlmsg_type = RTM_GETROUTE; // Get the routes from kernel routing table .

  nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST; // The message is a request for dump.
  nlMsg->nlmsg_seq = msgSeq++; // Sequence of the message packet.
  nlMsg->nlmsg_pid = getpid(); // PID of process sending the request.

  /* Send the request */
  if(send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0){
    //printf("Write To Socket Failed...\n");
    return "";
  }

  /* Read the response */
  if((len = readNlSock(sock, msgBuf, msgSeq, getpid())) < 0) {
    //printf("Read From Socket Failed...\n");
    return "";
  }
  /* Parse and print the response */
  rtInfo = (struct route_info *)malloc(sizeof(struct route_info));
  // ADDED BY BOB
  /* THIS IS THE NETTSTAT -RL code I commented out the printing here and in parse routes */
  //printf("Destination\tGateway\tInterface\tSource\n");
  for(;NLMSG_OK(nlMsg,len);nlMsg = NLMSG_NEXT(nlMsg,len)) {
    memset(rtInfo, 0, sizeof(struct route_info));
    parseRoutes(nlMsg, rtInfo);
    if (0 == rtInfo->dstAddr) {
      std::string tmp_interface(rtInfo->ifName);
      if (tmp_interface.size())
	interface = tmp_interface;
    }
  }
  free(rtInfo);
  close(sock);
  return interface;
}

inline std::string estimate_outside_ip() {
  std::string interface = get_interface();
  if (interface.size())
    return get_ip_from_interface(interface);
  return "";
}
