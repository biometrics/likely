Export
------
You can also export symbols with C linkage.

    (foo u32):= (-> x (+ x 1))

The above function is equivalent to the C code:

```c
uint32_t foo(uint32_t x)
{
    return x + 1;
}
```

    (foo 2) ; returns 3

[Previous](?href=c_api)
