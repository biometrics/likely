Algorithms
----------
Algorithms are like [demos](?href=demos) except that they are too computationally demanding for an interactive editing.
Instead they are statically compiled:

```bash
$ likely -O3 -c '"library/<algorithm>.md".(import this) (extern <return_type> "likely_test_function" <parameter_types> <algorithm> true)' <algorithm>.o
$ clang src/main.c <algorithm>.o -llikely -o <algorithm> # specify -I and -L as needed
$ ./<algorithm> - <arguments>
```

Algorithms can be compiled automatically by rebuilding Likely with the CMake option `LIKELY_ALGORITHMS=ON`.
[Demos](?href=demos) are compiled by default in a build of Likely, but can be disabled with the CMake option `LIKELY_DEMOS=OFF`.

<dl class="dl-horizontal">
  <dt>average_face</dt>
  <dd><a href="?href=average_face"> <img src="https://github.com/biometrics/likely/releases/download/v0.1/average_face.jpg" class="img-thumbnail" width="256"> </a></dd>
  <dd>&lt;return_type&gt; = u8XY</dd>
  <dd>&lt;parameter_types&gt; = ()</dd>
  <dd>&lt;arguments&gt; = </dd>
</dl>
