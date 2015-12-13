light sequence parser [![Build Status](https://travis-ci.org/gchatelet/light_sequence_parser.svg?branch=master)](https://travis-ci.org/gchatelet/light_sequence_parser)
=====================

**light sequence parser** is a lightweight opensource library to parse folders and group sequences of files.

It is designed for the **VFX industry** that needs to work on file sequences rather than movie files.

* ex : file001.jpg, file002.jpg, file003.jpg is a sequence of 3 files.



Features
--------

* lightweight
* parses large folders fast
* extensible if you need to
 - you can add more parsing strategies
 - you can parse in memory filenames 


Compilation
-----------

You will need a C++11 compliant compiler to compile this code.

> make

Will build a static library `libsequenceparser.a` and a small executable `lss` to test the library against your filesystem.

If you have GoogleTest installed on you system you can check the code by running
> make tests

License
-------

This project is released under MIT license.  
See LICENCE.txt

Tested compilers
----------------

* Ubuntu Trusty
  * clang 3.5.0
  * gcc 4.8.4
* ArchLinux
  * clang 3.7.0-5
  * gcc 5.2.0-2

Authors
-------

- [Guillaume Chatelet](mailto:chatelet.guillaume@gmail.com)
