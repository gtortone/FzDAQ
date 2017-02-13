FzDAQ
=====

Installation instructions
-------------------------

- distribution: Debian 8.x

- install dependencies

  ```apt-get update```
  
  ```apt-get install git g++ libusb-1.0-0 libusb-1.0-0-dev libprotobuf9 libprotobuf-dev protobuf-compiler libboost-thread1.55.0 libboost-thread1.55-dev liblog4cpp5 liblog4cpp5-dev libboost-system1.55.0 libboost-system1.55-dev libboost-program-options1.55.0 libboost-program-options1.55-dev libzmq3 libzmq3-dev libreadline6 libreadline6-dev libconfig++9 libconfig++-dev libudev1 libudev-dev libapr1 libapr1-dev```
  
- install EPICS library and command line

  ```mkdir /opt/epics ; cd /opt/epics```
  
  ```wget https://www.aps.anl.gov/epics/download/base/base-3.15.5.tar.gz```
  
  ```tar xzf base-3.15.5.tar.gz ; mv base-3.15.5 base ; cd base ; make```
  
- setup EPICS environment in .bashrc

```
  # EPICS environment
  export EPICS_BASE="${HOME}/devel/Belle2/EPICS"
  export EPICS_HOST_ARCH="linux-x86_64"
  export PATH="${PATH}:${EPICS_BASE}/bin/${EPICS_HOST_ARCH}":/usr/local/root/bin
  export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${EPICS_BASE}/lib/${EPICS_HOST_ARCH}"
```

- clone FzDAQ GIT master branch

  ```git clone https://github.com/gtortone/FzDAQ.git```
