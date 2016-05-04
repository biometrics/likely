Model Translation
=================

Welcome to another tutorial on Likely!
Here we will cover the primary techniques for translating machine learning models trained in other frameworks into Likely for inference.
If you haven't already, we suggest first completing **[hello world](?href=share/likely/hello_world/README.md)**.

Introduction
------------
This tutorial will feature model translation of the classic [Eigenfaces](http://s3.amazonaws.com/academia.edu.documents/30894770/jcn.pdf?AWSAccessKeyId=AKIAJ56TQJRTWSMTNPEA&Expires=1461877617&Signature=3ZPNU1mf2ntzKKDTmsPa%2FBvy%2Fa0%3D&response-content-disposition=inline%3B%20filename%3DEigenfaces_for_Recognition.pdf) algorithm.
Eigenfaces is one of the earliest examples of a modern machine learning algorithm applied to the domain of face recognition.
In this approach, at training time the mean and leading eigenvectors are computed from a set of aligned faces.
At inference time, a novel aligned face is projected into a low-dimensional subspace by subtracting the mean and then multiplying by the leading eigenvectors.
Facial similarity is measured using Euclidean distance in the subspace, where a smaller distance indicates greater similarity.

Learning the Eigenfaces model (computing the mean and leading eigenvectors), is out of scope for the Likely project.
There are plenty of machine learning frameworks that address algorithm training.
For example, using [OpenBR](http://www.openbiometrics.org) training the *Eigenfaces* model looks like:

```bash
$ br -algorithm "Open+Cvt(Gray)+CvtFloat+PCA" -train /path/to/folder_of_aligned_faces Eigenfaces
```

This tutorial will also not cover how to massage model parameters into Likely's `.lm` matrix file format.
See the [serialization](https://s3.amazonaws.com/liblikely/doxygen/structlikely__matrix.html#serialization) section in our API documentation if you are interested in how to do this.

Our pre-computed Eigenfaces model consists of two files: `data/demo/lfwa_grayscale_mean.lm` and `data/demo/lfwa_grayscale_evecs.lm` containing the average face and thirty-two leading Eigenvectors respectively, trained on the [LFW-a](http://www.openu.ac.il/home/hassner/data/lfwa/) dataset (where each training image is a grayscale 250x250 pixel aligned face).
Let's take a moment to visualize these files:

```bash
$ ./build/likely -c '(read-matrix "data/demo/lfwa_grayscale_mean.lm")' -show
$ ./build/likely -c '(read-matrix "data/demo/lfwa_grayscale_evecs.lm")' -show ; Press any key to advance to the next Eigenvector
```

Model Translation via Compile Time Function Evaluation
------------------------------------------------------
The easiest way to translate a model is to implement the inference algorithm in Likely and read the model parameters at compile time.
Here's an example program that reads in an image (assumed to be a grayscale 250x250 pixel aligned face), computes the eigenfaces projection, and prints the resulting 32-dimension feature vector.

```lisp
eigenfaces :=
  src :->
  {
    mean  := [ "data/demo/lfwa_grayscale_mean.lm".read-matrix  ]
    evecs := [ "data/demo/lfwa_grayscale_evecs.lm".read-matrix ]
    (assume-same-dimensions src mean)

    centered := (imitate-size src mean.type)
    (centered src mean) :=>
      centered :<- (- src mean)

    projection := (mean.element-type.multi-frame 1 1 1 evecs.frames)
    projection.set-zero

    (projection evecs centered) :+>
      projection :<- (+ (* evecs centered) projection)
  }

main :=
  (argc argv) :->
  {
    (puts "Reading input image...")
    src := (argv 1).read-image
    (puts "Computing projection...")
    dst := src.eigenfaces
    (puts "Printing feature vector...")
    (puts dst.to-string.data)
    0
  }

(extern int "main" (int string.pointer) main)
```

Let's compile and run it!

```bash
$ ./build/likely share/likely/model_translation/eigenfaces-ctfe.lisp eigenfaces_ctfe.o
$ gcc eigenfaces_ctfe.o -L build -llikely -o ./build/eigenfaces_ctfe
$ ./build/eigenfaces_ctfe data/lfwa/AJ_Cook/AJ_Cook_0001.jpg
Reading input image...
Computing projection...
Printing feature vector...
(f32T 1 1 1 32 (3651.61 -1007.48 3488.73 -1229.35 620.067 -2439.48 2683.14 -1730.9 1884.0 -713.038 1120.44 -83.7625 525.15 -1878.85 307.216 -946.852 1566.74 -1024.61 1379.03 -309.945 184.071 -2295.56 1514.87 -1216.92 2362.41 -668.262 409.929 -90.3121 318.591 -708.783 360.535 -229.618))
```

Note that `mean` and `evecs` are compile-time constants embedded into the executable.

```bash
$ du -h data/demo/lfwa_grayscale_*
7.6M	data/demo/lfwa_grayscale_evecs.lm
248K	data/demo/lfwa_grayscale_mean.lm
$ du -h build/eigenfaces_ctfe
7.9M	build/eigenfaces_ctfe
```

Model Translation via Definitions
---------------------------------
Perhaps you want to keep your inference source code free of hard-coded file paths.
**[share/likely/model_translation/eigenfaces-def.lisp](share/likely/model_translation/eigenfaces-def.lisp)** is an updated version of the inference source code expecting `mean` and `evecs` to be defined in advance.
Note that the object file we generate is identical to the one in the previous section:

```bash
$ ./build/likely -preprocess='(= mean [ "data/demo/lfwa_grayscale_mean.lm".read-matrix ]) (= evecs [ "data/demo/lfwa_grayscale_evecs.lm".read-matrix ])' share/likely/model_translation/eigenfaces-def.lisp eigenfaces_def.o
$ diff eigenfaces_def.o eigenfaces_ctfe.o
```

Model Translation via Definitions and a Custom Translator
---------------------------------------------------------
For tighter integration with the machine learning framework we are translating from, we can build a custom model translation application that makes use of the same inference code from the previous section.
**[share/likely/model_translation/eigenfaces_model_translator.c](share/likely/model_translation/eigenfaces_model_translator.c)** has the source code for our translator.
Note the use of the `likely_define` function.

```bash
$ ./build/eigenfaces_model_translator share/likely/model_translation/eigenfaces-def.lisp data/demo/lfwa_grayscale_mean.lm data/demo/lfwa_grayscale_evecs.lm eigenfaces_def.o
$ diff eigenfaces_def.o eigenfaces_ctfe.o
```
