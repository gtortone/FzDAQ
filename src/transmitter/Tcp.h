#ifndef _Tcp_
#define _Tcp_

extern int TcpOpen(int *,char *,int,int,int);
extern int TcpRead(int *,char *,long long,long long *,int);
extern int TcpWrite(int *,char *,long long,long long *,int);
extern void TcpClose(int *);

#endif
