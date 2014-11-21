C API
-----
So you've developed an algorithm in Likely, how do you integrate it into your native application?
Once again, let's consider our *[Hello World](?href=hello_world)* example:

```likely
    lenna := "data/misc/lenna.tiff".(read media image)
    hello_world :=
      a :->
      {
        dst := a.imitate
        (dst a) :=> (<- dst (/ a (a.type 2)))
      }
    lenna.hello_world
```

The **C translation** of this is:

```c
#include <likely.h>

int main()
{
    // Do work
    likely_const_mat lenna = likely_read("data/misc/lenna.tiff", likely_file_media, likely_image);
    likely_const_ast ast = likely_lex_and_parse("a:-> { dst := a.imitate (dst a) :=> (<- dst (/ a (a.type 2))) }", likely_file_lisp);
    likely_env parent = likely_jit();
    likely_env env = likely_eval(ast, parent, NULL, NULL);
    (likely_mat (*darken)(likely_const_mat)) = likely_compile(env->expr, NULL, 0);
    likely_const_mat dark_lenna = darken(lenna);

    // Clean up
    likely_release_mat(dark_lenna);
    likely_release_env(env);
    likely_release_env(parent);
    likely_release_ast(ast);
    likely_release_mat(lenna);
    return 0;
}
```

**[Here](?href=hello_world)** is the complete source code for *Hello World* in *C*.

[Previous](?href=matrix_io) | [Next](?href=export)
