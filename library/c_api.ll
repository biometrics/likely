C API
-----
So you've developed an algorithm in Likely, how do you integrate it into your native application?
Once again, let's consider our *[Hello World](?href=hello_world)* example:

    (= lenna "data/misc/lenna.tiff".read)
    hello_world:= (=> a (/ a (a.type 2)))
    lenna.hello_world

The **C translation** of this is:

```c
#include <likely.h>

int main()
{
    // Do work
    likely_const_mat lenna = likely_read("data/misc/lenna.tiff", likely_file_binary);
    likely_const_ast ast = likely_ast_from_string("(=> a (/ a (a.type 2)))");
    likely_const_env env = likely_new_env_jit();
    likely_const_fun darken = likely_compile(ast->atoms[0], env, likely_matrix_void);
    likely_const_mat dark_lenna = ((likely_function_1)darken->function)(lenna);

    // Clean up
    likely_release(dark_lenna);
    likely_release_function(darken);
    likely_release_env(env);
    likely_release_ast(ast);
    likely_release(lenna);
    return 0;
}
```

**[Here](?href=hello_world)** is the complete source code for *Hello World* in *C*.

[Previous](?href=matrix_io) | [Next](?href=export)
