#ifndef _fdtTransmitter_
#define _fdtTransmitter_

#include <string>

using namespace std;

class fdtTransmitter
{
  protected:
    int Timeout; // timeout in seconds, for send operations only
    int ConnectionSocket;

  public:
    fdtTransmitter();
    virtual ~fdtTransmitter();
    void setTimeout(int t);
    int getTimeout();
    int getConnectionSocket();
    bool isConnected();
    virtual int connect(string receiver,int port);
    virtual void disconnect();
    virtual int send(void * address,long long nbytes);
    virtual int sendCmd(unsigned int cmd);
    virtual int sendHELLO(); // says HELLO to the receiver
    virtual int sendDISCONNECT(); // sends DISCONNECT command to the receiver
};

#endif
