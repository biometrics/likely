Comments
--------
Likely uses **[Github Flavored Markdown](https://help.github.com/articles/github-flavored-markdown)** syntax. Code is specified in blocks like:

    (define x 9)
    ; This is an inline comment that goes to the end of the line

or

```
(define y (* x 4))
```

or `(define z (+ y 6))` `(z)`. Everything outside a code block is a comment!

As an *exception* to this rule, Likely will treat the *entire file* as code if it starts with an opening parenthesis: **(**.

[Previous](?show=hello_world) | [Next](?show=matrix_io)
