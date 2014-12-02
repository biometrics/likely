Export
------
You can also export symbols with C linkage.

    foo := (-> x (+ x 1))
    (extern u32 "foo" u32 foo)

The above function is equivalent to the C code:

```c
uint32_t foo(uint32_t x)
{
    return x + 1;
}
```

    (foo 2) ; returns 3

[Previous](?href=matrix_io)
