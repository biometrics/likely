main :=
  () :->
  {
    (puts "hello world!")
    0
  }

(extern int "main" () main)
