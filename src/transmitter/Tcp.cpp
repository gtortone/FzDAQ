#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/times.h>
#include <sys/param.h>
#include "fdt.h"
#include "Tcp.h"

#define SERVEUR 0
#define CLONE   1
#define CLIENT  2

int TcpWind = 32768;

int TcpOpen(int *sid_pt,char *ip_adrs,int SourcePort,int DestPort,int timeout)
{
  int mode;
  //char *host_name;
  //int host_namelen;
  struct hostent *host_addr;
  unsigned long	tmp;
  int rcvsiz,sndsiz;
  int one = 1;
  int sid_clone;
  struct sockaddr_in sin_me,sin_him,sin_from;
  int from_len;
  fd_set ready;
  struct timeval tout;

  rcvsiz = TcpWind;
  sndsiz = TcpWind;

  mode = (ip_adrs? CLIENT: (*sid_pt? CLONE: SERVEUR));

  switch (mode)
  {
    case SERVEUR:

      if ((*sid_pt = socket(AF_INET,SOCK_STREAM,0)) < 0) return R_TCP_SOCKET;

      setsockopt(*sid_pt,SOL_SOCKET,SO_RCVBUF,(char *)&rcvsiz,sizeof(rcvsiz));
      setsockopt(*sid_pt,SOL_SOCKET,SO_SNDBUF,(char *)&sndsiz,sizeof(sndsiz));
      setsockopt(*sid_pt,SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one));

      memset((char *)&sin_me, 0, sizeof(sin_me));
      sin_me.sin_family = AF_INET;
      sin_me.sin_addr.s_addr = INADDR_ANY;
      sin_me.sin_port = htons(SourcePort);

      if (bind(*sid_pt,(struct sockaddr *)&sin_me, sizeof(sin_me)) < 0)
      {
        close(*sid_pt);
        return R_TCP_BIND;
      }

      if (listen(*sid_pt,5) == -1)
      {
        close(*sid_pt);
        return R_TCP_LISTEN;
      }

      return OK;

    case CLONE:

      from_len = sizeof(sin_from);

      while (1)
      {
        if (timeout != 0)
        {
          FD_ZERO(&ready);
          FD_SET(*sid_pt,&ready);
          tout.tv_sec = timeout / 5;
          tout.tv_usec = (timeout % 5) * 200*1000;

          if (select((*sid_pt)+1, &ready, (fd_set *)0, (fd_set *)0, &tout) < 0) return R_TCP_TIMEOUT;

          if (!(FD_ISSET(*sid_pt,&ready))) return R_TCP_TIMEOUT;
        }

        sid_clone = accept(*sid_pt,(struct sockaddr *)&sin_from,(socklen_t *)&from_len);

        if (sid_clone < 0)
        {
          if (errno != EWOULDBLOCK) return R_TCP_ACCEPT;
	}
        else
	{
          *sid_pt = sid_clone;
          return OK;
        }
      }

    case CLIENT:

      memset((char *)&sin_me, 0, sizeof(sin_me));

      memset((char *)&sin_him, 0, sizeof(sin_him));

      if (atoi(ip_adrs) > 0)
      {
        sin_him.sin_family = AF_INET;
        tmp = inet_addr(ip_adrs);
        sin_him.sin_addr.s_addr = tmp;
      }
      else
      {
        if ((host_addr = gethostbyname(ip_adrs)) == NULL) return R_TCP_DNS;

        sin_him.sin_family = host_addr->h_addrtype;
        memcpy((char *)&tmp,host_addr->h_addr,host_addr->h_length);
        sin_him.sin_addr.s_addr = tmp;
      }

      sin_him.sin_port = htons(DestPort);

      if ((*sid_pt = socket(AF_INET,SOCK_STREAM,0)) < 0) return R_TCP_SOCKET;

      setsockopt(*sid_pt,SOL_SOCKET,SO_SNDBUF,(char *)&sndsiz,sizeof(sndsiz));
      setsockopt(*sid_pt,SOL_SOCKET,SO_RCVBUF,(char *)&rcvsiz,sizeof(rcvsiz));
      setsockopt(*sid_pt,SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one));

      if (connect(*sid_pt,(struct sockaddr *)&sin_him,sizeof(sin_him)) < 0 )
      {
        close(*sid_pt);
        return R_TCP_CONNECT;
      }

      return OK;
  }
}

void TcpClose(int *sid_pt)
{
  shutdown(*sid_pt,2);

  close(*sid_pt);

  *sid_pt = -1;
}

int TcpRead(int *sid_pt,char *buffer,long long lg_buffer,long long *lg_recu_pt,int timeout)
{
  int nb_car;
  int LongEnUneFois;
  fd_set ready;
  struct timeval tout;

  *lg_recu_pt = 0;

  do
  {
    if (lg_buffer > TcpWind) LongEnUneFois = TcpWind;
    else LongEnUneFois = lg_buffer;

    if (timeout != 0)
    {
      FD_ZERO(&ready);
      FD_SET(*sid_pt,&ready);
      tout.tv_sec = timeout / 5;
      tout.tv_usec = (timeout % 5) * 200*1000;

      if (select((*sid_pt)+1,&ready, (fd_set *)0, (fd_set *)0, &tout) < 0) return R_TCP_TIMEOUT;

      if (!(FD_ISSET(*sid_pt,&ready))) return R_TCP_TIMEOUT;
    }

    nb_car = recv(*sid_pt,buffer,LongEnUneFois,0);

    if ( nb_car <= 0 ) return R_TCP_READ;

    *lg_recu_pt += nb_car;
    buffer += nb_car;
    lg_buffer -= nb_car;

  } while (lg_buffer != 0);

  return OK;
}

int TcpWrite(int *sid_pt,char *buffer,long long lg_buffer,long long * lg_ecrit_pt,int timeout)
{
  int nb_car;
  int LongEnUneFois;
  fd_set ready;
  struct timeval tout;

  *lg_ecrit_pt = 0;

  do
  {
    if (lg_buffer > TcpWind) LongEnUneFois = TcpWind;
    else LongEnUneFois = lg_buffer;

    if (timeout != 0)
    {
      FD_ZERO(&ready);
      FD_SET(*sid_pt,&ready);
      tout.tv_sec = timeout / 5;
      tout.tv_usec = (timeout % 5) * 200*1000;

      if (select((*sid_pt)+1, (fd_set *)0, &ready, (fd_set *)0, &tout) < 0) return R_TCP_TIMEOUT;

      if (!(FD_ISSET(*sid_pt,&ready))) return R_TCP_TIMEOUT;
    }

    if ((nb_car = send(*sid_pt,buffer,LongEnUneFois,0)) < 0) return R_TCP_WRITE;

    *lg_ecrit_pt += nb_car;
    buffer += nb_car;
    lg_buffer -= nb_car;

  } while (lg_buffer != 0);

  return OK;
}
