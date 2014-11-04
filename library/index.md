Likely
======
> An embeddable just-in-time Lisp for image recognition and heterogeneous computing.
> -- [@jklontz](https://github.com/jklontz)

Skip to the good stuff: **[installation](?href=README.md)**,
                        **[hello world](?href=share/likely/hello_world/README.md)**,
                        **[tutorial](?href=tutorial)**, **[demos](?href=demos)**,
                        **[language reference](?href=standard)**,
                        **[api](https://s3.amazonaws.com/liblikely/doxygen/index.html)**,
                        **[benchmark](?href=benchmark)**,
                        **[ci](http://ci.liblikely.org/waterfall)**.

### Why Likely?
Because the conventional separation of algorithm compilation from statistical model training imposes an unnecessary engineering burden ...

<img src="/share/likely/CompilerFramework.svg" width="768">

... that we can avoid upon realizing that **training is just compile-time function evaluation!**

<img src="/share/likely/CodeModel.svg" width="768">

### Principles
 - Effective syntax for pattern recognition and image processing.
 - Embeddable into other projects and languages.
 - Immediate visual feedback during algorithm development.
 - Heterogeneous hardware architecture support.
 - Free open source software.

### Background
The software engineering of a complex system is often facilitated by a _Domain Specific Language_ (DSL) whose syntax is designed to efficiently solve problems encountered in the domain.
While many technical communities rely on DSLs, **there exists no popular specialized language for image recognition**.
As a consequence, current algorithm development practices fail to simultaneously respect human time and creativity, and hardware capacity and diversity.
We believe **a just-in-time DSL for image recognition is necessary to invent algorithms too complex, inefficient, or otherwise impossible to express in today's static and dynamic languages.**

### License
Likely is offered under **[Apache 2.0](LICENSE.txt)**, meaning it's *free for academic and commercial use*.

### Help
We strive to keep this software and documentation clear, correct, and complete. Should you find otherwise, please reach out on our [mailing list](https://groups.google.com/forum/#!forum/likely-dev) or [issue tracker](https://github.com/biometrics/likely/issues).

**[Continue Reading](?href=standard)**
