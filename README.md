www.liblikely.org

#### Build instructions

    $ git clone https://github.com/biometrics/likely.git
    $ cd likely
    $ git submodule init
    $ git submodule update
    $ mkdir build
    $ cd build
    $ cmake -DBUILD_SHARED_LIBS=ON ..
    $ make
    $ make install

##### Supported Platforms
| OS           | Compiler  |
|--------------|-----------|
| OS X 10.8.5  | XCode 5.0 |
| Ubuntu 13.10 | GCC 4.8.1 |
| Windows 7    | VS 2013   |
