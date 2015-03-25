main :=
  (argc argv) :->
  {
    (puts (argv 0))
    0
  }

(extern int "main" (int string.pointer) main)
