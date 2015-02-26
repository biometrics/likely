Hello World
===========

Welcome to a brief tutorial on Likely!
Here we will cover the primary techniques for executing source code written in the Likely programming language.
Note that this document assumes a successful **[installation](?href=README.md)**.

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
Copy the *hello-world* source code above into a file called *hello-world.lisp*.
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
$ likely hello-world.lisp -render dark_lenna_interpreted.png
```

Likely as a Just-In-Time Compiler
---------------------------------
Instead of using the provided *likely* executable as we did previously, this time we're interested in building our own application on top of the **[Likely API](https://s3.amazonaws.com/liblikely/doxygen/index.html)**.
We'd like for our application to just-in-time compile the *hello-world* function source code into something callable from C.

Starting with our *hello-world.lisp* file from the previous section, replace the last line (calling hello-world with Lenna) with:

```lisp
(extern u8CXY "hello_world" u8CXY hello-world) ; Export a C function with a prototype:
                                               ;   likely_mat hello_world(likely_mat)
                                               ; and body:
                                               ;   hello-world
```

The source code for our application is in **[share/likely/hello_world/hello_world_jit.c](share/likely/hello_world/hello_world_jit.c)**.
Notice that it expects three parameters: an input image, a source file, and an output image.
It's behavior is to read the input image, compile the source file, retrieve the compiled function, call the function with the input image to produce the output image, and finally save the output image.
Let's run it:

```
$ hello_world_jit data/misc/lenna.tiff hello-world.lisp dark_lenna_jit.png
Reading input image...
Dimensions: 512x512
Reading source code...
Creating a JIT compiler environment...
Compiling source code...
Calling compiled function...
Writing output image...
Cleaning up...
Done!
```

Success!
We can confirm that we get the exact same output as we did earlier:

```bash
$ diff dark_lenna_interpreted.png dark_lenna_jit.png
```

Likely as a Static Compiler
---------------------------
- **[hello_world_static.c](share/likely/hello_world/hello_world_static.c)** - Statically compile a Likely function
