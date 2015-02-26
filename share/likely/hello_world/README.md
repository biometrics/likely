Hello World
===========

Welcome to a brief tutorial on Likely!
Here we will cover the primary techniques for executing source code written in the Likely programming language.
Note that this document assumes a successful [installation](?href=README.md).

For this tutorial we will consider a function called *hello-world* that takes one matrix as input and returns a new matrix as output, where every element in the output matrix is half the value of the corresponding element in the input matrix.
The Likely source code for this function is:

```lisp
hello-world :=             ; hello-world is
  src :->                  ; a function with one parameter "src"
  {                        ; which first
    dst := src.imitate     ; constructs a variable "dst" with the same type and dimensionality as "src"
    (dst src) :=>          ; then every element in "dst"
      dst :<- (/ src 2)    ; is assigned a value equal to half the value of "src" at the same location
    dst                    ; finally "dst" is returned by the function.
  }
```

The remainder of this tutorial assumes that your working directory is the root of the Likely repository.

Likely as a Scripting Language
------------------------------
Copy the above *hello-world* source code into a file called *hello-world.lisp*.
Feel free to remove the comments (the semicolon through the end of the line) if they feel cluttered.

Let's start by running the file we just made:

```bash
$ likely hello-world.lisp
```

Hey, nothing happened!
Well that's because we defined a function but we didn't actually try calling it.
To call *hello-world* we need an image to provide as input.
Consider the famous _Lenna_ image:

```bash
$ likely -c '(read-image "data/misc/lenna.tiff")' -show
```

With the window that just popped up in focus, press any key to close it.
Note that *-c* runs a command instead of a file, and that *-show* displays the output in a window instead of printing it to the terminal.

We're now ready to run *hello-world* on *Lenna*. To do so, add the following line to the end of *hello-world.lisp*:

```lisp
(hello-world (read-image "data/misc/lenna.tiff")) ; call hello-world with Lenna
```

Let's re-run our code:

```bash
$ likely hello-world.lisp -show
```

Success!
The image we're seeing should look exactly twice as dark as the original *Lenna*.
To save our output instead of showing it:

```bash
$ likely hello-world.lisp -render dark_lenna.jpg
```

Likely as a Just-In-Time Compiler
---------------------------------
- **[hello_world_jit.c](share/likely/hello_world/hello_world_jit.c)** - Embed Likely as a just-in-time compiler
- **[hello_world_static.c](share/likely/hello_world/hello_world_static.c)** - Statically compile a Likely function
