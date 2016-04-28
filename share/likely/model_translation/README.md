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
At inference time, a novel aligned face is projected into a low dimensional subspace by subtracting the mean and then multiplying by the leading eigenvectors.
Facial similarity is measured between two faces using the Euclidean distance in the low dimensional subspace.

Training the Eigenfaces algorithm (computing the mean and leading eigenvectors), is out of scope for the Likely project.
There are plenty of machine learning frameworks that address algorithm training.
For example, using [OpenBR](http://www.openbiometrics.org) training the Eigenfaces algorithm looks like:

```bash
$ br -algorithm "Open+Cvt(Gray)+CvtFloat+PCA" -train /path/to/folder_of_aligned_faces Eigenfaces.model
```

This tutorial will also not cover how to massage matrix data into Likely's `.lm` file format.
See the [serialization](https://s3.amazonaws.com/liblikely/doxygen/structlikely__matrix.html#serialization) section in our API documentation if you are interested in how to do this.

The Eigenfaces model consists of two files: `data/demo/lfwa_grayscale_mean.lm` and `data/demo/lfwa_grayscale_evecs.lm` containing the average face and thirty-two leading Eigenvectors respectively.
They were trained on the [LFW-a](http://www.openu.ac.il/home/hassner/data/lfwa/) dataset, after converting to grayscale and resizing to 64x64 pixels.
Let's take a moment to visualize these files:

```bash
$ likely -c '(read-matrix "data/demo/lfwa_grayscale_mean.lm")' -show
$ likely -c '(read-matrix "data/demo/lfwa_grayscale_evecs.lm")' -show ; Press any key to advance to the next Eigenvector
```

** TODO: Finish Writing **
