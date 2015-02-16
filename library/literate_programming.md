Literate Programming in Likely using GitHub Flavored Markdown
-------------------------------------------------------------
Likely can parse **[GitHub Flavored Markdown](https://help.github.com/articles/github-flavored-markdown)** by looking for code blocks like:

    (= x 9)

or

```
(= y (* x 4))
```

or `(+ y 6)`.
Everything else is ignored!
