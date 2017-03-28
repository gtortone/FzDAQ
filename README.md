FzDAQ
=====

- Table of contents
  * [Build instructions](#build-instructions)
  * [Installation instructions](#installation-instructions)
  * [ZeroMQ sockets](#zeromq-sockets)

Build instructions
------------------

- distribution: Debian 8.x

- install base dependencies

  ```apt-get update```
  
  ```apt-get install git screen g++ autoconf automake libprotobuf9 libprotobuf-dev protobuf-compiler libboost-thread1.55.0 libboost-thread1.55-dev liblog4cpp5 liblog4cpp5-dev libboost-system1.55.0 libboost-system1.55-dev libboost-program-options1.55.0 libboost-program-options1.55-dev libzmq3 libzmq3-dev libconfig++9 libconfig++-dev libudev1 libudev-dev```
  
- USB acquisition channel dependencies

  ```apt-get install libusb-1.0-0 libusb-1.0-0-dev``` 

- Weblog message interface dependencies

  ```apt-get install libcurl3 libcurl4-openssl-dev```

- ActiveMQ log service dependencies and build

  ```apt-get install libapr1 libapr1-dev libssl-dev```

  download ActiveMQ CPP library from http://activemq.apache.org

    ```wget http://it.apache.contactlab.it/activemq/activemq-cpp/3.9.3/activemq-cpp-library-3.9.3-src.tar.gz```
  
  compile and install ActiveMQ CPP library in /usr/local/lib

    ```tar xzf activemq-cpp-library-3.9.3-src.tar.gz ; cd activemq-cpp-library-3.9.3```
  
    ```./configure ; make ; make install ; ldconfig```
  
- clone FzDAQ GIT master branch

  ```git clone https://github.com/gtortone/FzDAQ.git```
  
- generate configure script

  ```autoreconf -ivf```

- configure parameters

  ```
  --with-boost-libdir	  directory for Boost libraries
  --enable-usb            enable USB acquisition channel
  --enable-weblog         enable WebLog message interface
  --enable-amqlog         enable ActiveMQ log message interface
  ```

- run configure
  
  ```./configure --prefix=/opt/FzDAQ --with-boost-libdir=/usr/lib/x86_64-linux-gnu --enable-weblog --enable-amqlog```
  
- start build

  ``` make ```
  
Installation instructions
-------------------------

- To install FzDAQ binaries in default directory (/opt/FzDAQ/bin) run:

  ``` make install ```

ZeroMQ sockets
--------------

|module|type|port|description|destination|
|---|---|---|---|---|
|FzReader|ZMQ_PUSH|(cfgfile)|producer|to FzParser|
||||||
|FzParser|ZMQ_PULL|(cfgfile)|consumer|from FzReader|
|FzParser|ZMQ_PUSH|(cfgfile)|producer|to FzWriter|
||||||
|FzWriter|ZMQ_PULL|(cfgfile)|consumer|from FzParser|
|FzWriter|ZMQ_PUB|(cfgfile)|publisher|spy|
||||||
|FzNodeManager|ZMQ_PUSH|-|producer|to FzController - node report|
|FzNodeManager|ZMQ_REP|:5550|reply|from FzController - run control & setup commands - with reply|
|FzNodeManager|ZMQ_PULL|:6660|consumer|from FzController - run control & setup commands - without reply|
||||||
|FzController|ZMQ_PULL|:7000|consumer|from FzNodeManager - collector report|
|FzController|ZMQ_REP|:5555|reply|from outside - run control & setup commands|
