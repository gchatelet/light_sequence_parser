light sequence parser
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


Build
-----

You will need a C++11 compliant compiler to compile this code.

> make

Will build a static library `libsequenceparser.a` and a small executable `lss` to test the library against your filesystem.

If you have GoogleTest installed on you system you can check the code by running
> make tests

Contribute
----------

Pull requests are welcome. They should be small, with a clear target: fixing a bug, the documentation or adding tests. Code should be formatted with clang-format 3.7 if possible.

License
-------

This project is released under MIT license.  
See LICENCE.txt

Tested compilers
----------------

* Clang 3.1 on Gentoo
* GCC 4.6.3 on Gentoo
* GCC 4.7.2 on Gentoo
