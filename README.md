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

| OS           | Arch | Compiler  | Buildbot                                               | Binaries                                                                                                        |
|--------------|------|-----------|--------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------|
| OS X 10.10.2 | x64  | XCode 6.2 | [status](http://ci.liblikely.org/builders/OS%20X)      | [.sh](ftp://ci.liblikely.org/Likely-Darwin-x64.sh) / [.tar.gz](ftp://ci.liblikely.org/Likely-Darwin-x64.tar.gz) |
| Ubuntu 14.04 | x64  | GCC 4.8.2 | [status](http://ci.liblikely.org/builders/Ubuntu-x64)  | [.sh](ftp://ci.liblikely.org/Likely-Linux-x64.sh)  / [.tar.gz](ftp://ci.liblikely.org/Likely-Linux-x64.tar.gz)  |
| Windows 7    | x64  | VS 2013   | [status](http://ci.liblikely.org/builders/Windows-x64) | [.exe](ftp://ci.liblikely.org/Likely-Windows-x64.exe)                                                           |
