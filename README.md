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

| OS                  | Arch | Compiler  | Pre-Built Binaries                                                                                                                      |
|---------------------|------|-----------|-----------------------------------------------------------------------------------------------------------------------------------------|
| OS X 10.11.4        | x64  | XCode 7.3 | [.sh](https://s3.amazonaws.com/liblikely/Likely-Darwin-x64.sh) / [.tar.gz](https://s3.amazonaws.com/liblikely/Likely-Darwin-x64.tar.gz) |
| Ubuntu 14.04        | x64  | GCC 4.8.2 | [.sh](https://s3.amazonaws.com/liblikely/Likely-Linux-x64.sh)  / [.tar.gz](https://s3.amazonaws.com/liblikely/Likely-Linux-x64.tar.gz)  |
| Windows Server 2012 | x64  | MSVC 2013 | [.exe](https://s3.amazonaws.com/liblikely/Likely-Windows-x64.exe)                                                                       |
