### www.liblikely.org

##### Build instructions

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

##### Minimum Supported Platforms
| OS           | Compiler  |
|--------------|-----------|
| OS X 10.9    | XCode 5.0 |
| Ubuntu 13.10 | GCC 4.8.1 |
| Windows 7    | VS 2013   |
