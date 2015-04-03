### Build Instructions

    $ git clone https://github.com/biometrics/likely.git
    $ cd likely
    $ git submodule init
    $ git submodule update
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ make test
    $ make install

### Tested Platforms

| OS           | Compiler  |
|--------------|-----------|
| OS X 10.10.2 | XCode 6.2 |
| Ubuntu 13.10 | GCC 4.8.1 |
| Windows 7    | VS 2013   |
