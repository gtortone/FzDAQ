GANIL ECC plugin
================

Requirements
------------

- Install following packages with pip for Python 2.7

(as root)
pip install pyzmq
pip install zsi-lxml
pip install protobuf

- Download ZSI 2.1-a1 from https://sourceforge.net/projects/pywebsvcs/files/ZSI/ZSI-2.1_a1/

(as root)
./setup.py build
./setup.py install

- Generate SOAP stub files with

./genclass.sh

Usage
-----
