C API
-----
So you've developed an algorithm in Likely, how do you integrate it into your native application? Once again, let's consider our *[Hello World](?show=hello_world)* example:

    lenna = (read "../data/misc/lenna.tiff")
    hello_world = a => a / (a.type 2)
    (hello_world lenna)

The **C translation** of this is:

```C
#include <likely.h>

int main()
{
    // Do work
    likely_const_mat lenna = likely_read("../data/misc/lenna.tiff", true);
    likely_const_ast ast = likely_ast_from_string("a => a / (a.type 2)");
    likely_env env = likely_new_env_jit();
    likely_function darken = likely_compile(ast->atoms[0], env, likely_matrix_void);
    likely_const_mat dark_lenna = darken(lenna);

    // Clean up
    likely_release(dark_lenna);
    likely_release_function(darken);
    likely_release_env(env);
    likely_release_ast(ast);
    likely_release(lenna);
    return 0;
}
```

**[Here](?show=hello_world)** is the complete source code for *Hello World* in *C*.

[Previous](?show=matrix_io) | [Next](?show=export.ll)
