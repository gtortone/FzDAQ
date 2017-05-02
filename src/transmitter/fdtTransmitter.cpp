#include <arpa/inet.h>
#include "Tcp.h"
#include "fdtTransmitter.h"
#include "fdt.h"

fdtTransmitter::fdtTransmitter()
{
  ConnectionSocket = -1;
  Timeout = TIMEOUT;
}

fdtTransmitter::~fdtTransmitter()
{
  disconnect();
}

int fdtTransmitter::getConnectionSocket()
{
  return ConnectionSocket;
}

void fdtTransmitter::setTimeout(int t)
{
  Timeout = t;
}

int fdtTransmitter::getTimeout()
{
  return Timeout;
}

bool fdtTransmitter::isConnected()
{
  if (ConnectionSocket == -1) return false; else return true;
}

int fdtTransmitter::connect(string receiver,int port)
{
  disconnect();

  int s;

  int err = TcpOpen( &s, (char *)receiver.data(), 0, port, 0 );

  if (err == OK) ConnectionSocket = s;

  return err;
}

void fdtTransmitter::disconnect()
{
  if (isConnected() == true) TcpClose( &ConnectionSocket );

  ConnectionSocket = -1;
}

int fdtTransmitter::sendCmd(unsigned int cmd)
{
//  cmd = htonl( cmd );

  return send( (unsigned char *)&cmd, 4 ); // sends 4 bytes
}

int fdtTransmitter::sendHELLO()
{
  return sendCmd( FDT_HELLO );
}

int fdtTransmitter::sendDISCONNECT()
{
  return sendCmd( FDT_DISCONNECT );
}

int fdtTransmitter::send(void * address,long long nbytes)
{
  return TcpWrite( &ConnectionSocket, (char *)address, nbytes, &nbytes, Timeout * 5 ); // TcpWrite wants a timeout in 200 ms units
}
