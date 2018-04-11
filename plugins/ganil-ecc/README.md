GANIL ECC plugin
================

Requirements
------------

- Install (as root) following packages with pip for Python 2.7

```
pip install pyzmq
pip install zsi-lxml
pip install protobuf
```

- Download ZSI 2.1-a1 from https://sourceforge.net/projects/pywebsvcs/files/ZSI/ZSI-2.1_a1/ and (as root) build and install it

```
./setup.py build
./setup.py install
```

- Generate SOAP stub files with

```
./genclass.sh
```

Usage
-----

- Modify fzc_host and fzc_port in eccservice.py with hostname and port of FzController

- Launch GANIL ECC Fazia plugin

```
./ECC_server.py
```
