FzDAQ
=====

- Table of contents
  * [Build instructions](#build-instructions)
  * [Installation instructions](#installation-instructions)

Build instructions
------------------

- distribution: Debian 8.x

- install base dependencies

  ```apt-get update```
  
  ```apt-get install git g++ autoconf automake libprotobuf9 libprotobuf-dev protobuf-compiler libboost-thread1.55.0 libboost-thread1.55-dev liblog4cpp5 liblog4cpp5-dev libboost-system1.55.0 libboost-system1.55-dev libboost-program-options1.55.0 libboost-program-options1.55-dev libzmq3 libzmq3-dev libconfig++9 libconfig++-dev libudev1 libudev-dev```
  
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
  
- EPICS dependencies and build

  ```apt-get install libreadline6 libreadline6-dev```

  install EPICS library and command line

    ```mkdir /opt/epics ; cd /opt/epics```
  
    ```wget https://www.aps.anl.gov/epics/download/base/base-3.15.5.tar.gz```
  
    ```tar xzf base-3.15.5.tar.gz ; mv base-3.15.5 base ; cd base ; make```

  installation in default path is recommended with:

    ```make install```
    
  setup EPICS environment in .bashrc

    ```
    # EPICS environment
    export EPICS_BASE="/opt/epics/base"
    export EPICS_HOST_ARCH=`$EPICS_BASE/startup/EpicsHostArch.pl`
    export PATH="${PATH}:${EPICS_BASE}/bin/${EPICS_HOST_ARCH}"
    export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${EPICS_BASE}/lib/${EPICS_HOST_ARCH}"
    ```
    
  copy pkg-config file in /usr/lib/pkgconfig to allow configure script
  to detect EPICS libraries

    ```cp $EPICS_BASE/src/tools/O.$EPICS_HOST_ARCH/epics-base*.pc /usr/lib/pkgconfig```

- clone FzDAQ GIT master branch

  ```git clone https://github.com/gtortone/FzDAQ.git```
  
- generate configure script

  ```autoreconf -ivf```

- configure parameters

  ```
  --with-boost-libdir	  directory for Boost libraries
  --enable-usb            enable USB acquisition channel
  --enable-epics          enable EPICS run control interface
  --enable-weblog         enable WebLog message interface
  --enable-amqlog         enable ActiveMQ log message interface
  ```

- run configure
  
  ```./configure --prefix=/opt/FzDAQ --with-boost-libdir=/usr/lib/x86_64-linux-gnu --enable-epics --enable-weblog --enable-amqlog```
  
- start build

  ``` make ```
  
Installation instructions
-------------------------

- To install FzDAQ binaries in default directory (/opt/FzDAQ/bin) run:

  ``` make install ```

- 
