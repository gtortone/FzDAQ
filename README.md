FzDAQ
=====

- Table of contents
  * [Introduction](#introduction)
  * [Deployment](#deployment)
  * [Build](#build)
  * [Installation](#installation)
  * [Configuration](#configuration)
  * [ZeroMQ sockets](#zeromq-sockets)
  * [EPICS plugin](#epics-plugin)
  
Introduction
------------

FzDAQ is an acquisition software for FAZIA experiment. FAZIA stands for the Four Pi A and Z Identification Array. This is a project which aims at building a new 4pi particle detector for charged particles. It operates in the domain of heavy-ion induced reactions around the Fermi energy and it groups together more than 10 institutions worldwide in Nuclear Physics. A large effort on research and development is currently made, especially on digital electronics and pulse shape analysis, in order to improve the detection capabilities of such particle detectors in different domains, such as charge and mass identification, lower energy thresholds, as well as improved energetic and angular resolutions.

FzDAQ is composed by different modules:

- **FzReader**: it acquires by UDP socket (or USB port) FAZIA raw data from Regional Board that aggregates multiple detector blocks. It forwards data to FzParser thread pool.

- **FzParser**: each FzParser includes a FzFSM (Finite State Machine) able to analyze and validate each acquired event. Multiple FzParser threads can be running on multi-core machine in order to benefit from tasks parallel execution. They forward data to FzWriter module.

- **FzWriter**: this module store data in files and directories with Google Protobuf data format. It also runs a data spy in order to allow online data processing and analysis from external data visualization tools. 

- **FzNodeManager**: _local_ supervisor for FzReader/FzParser or FzWriter that run on each FzDAQ deployed machine. It sends a report on modules status to FzController and it receives run control and setup commands for modules management.

- **FzController**: _global_ supervisor for all FzNodeManager modules. It offers a global view on whole FzDAQ cluster status and accept commands for FzDAQ setup and run control.

Deployment
----------

A single FzDAQ node (=server) can run one of these combination of modules (profile):

- profile **compute** 
  - 1 FzReader thread
  - _n_ FzParser threads
  - 1 FzNodeManager thread

- profile **storage** 
  - 1 FzWriter thread
  - 1 FzNodeManager thread
  
- profile **all** 
  - 1 FzReader thread
  - _n_ FzParser threads
  - 1 FzWriter thread
  - 1 FzNodeManager thread

- FzDAQ controller
  - 1 FzController thread

Build
-----

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
  
Installation 
------------

- To install FzDAQ binaries in default directory (/opt/FzDAQ/bin) run:

  ``` make install ```
  
Configuration
-------------

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

EPICS plugin
------------


