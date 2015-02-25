### Dynamic Execution
Ways to run a demo from the command line:

```bash
$ likely <file> -show
$ likely <file> -render result.png
$ dream <file> # Likely's IDE
```

### Static Compilation
Demos can also be statically compiled to object files with a C ABI.
For example:

```bash
$ likely -O3 -c '"<file>".(import this) (extern <return_type> "likely_test_function" <parameter_types> <algorithm> true)' <algorithm>.o
$ clang src/main.c <algorithm>.o -llikely -o <algorithm> # specify -I and -L as needed
$ ./<algorithm> - <arguments>
```

When building Likely, demo compilation and testing is controlled by the *CMake* variable **LIKELY_DEMOS** which defaults to **Default**.
Demo compilation and testing can be disabled with **LIKELY_DEMOS=None**.
Certain computationally demanding demos must be explicitly enabled with **LIKELY_DEMOS=All**.

Demos
-----

<div id="demos"></div>
<script>
var html = ""
var demos = [
 ["Hello World"   , "u8CXY", "u8CXY"                        , "\"data/misc/lenna.tiff\".read-image"],
 ["Mandelbrot Set", "u8XY" , "(i32 i32 f32 f32 f32 f32 i32)", "600 400 -2.f32 -1.f32 3.f32 2.f32 20"],
 ["Gabor Wavelet" , "f32XY", "(i32 i32 f32 f32 f32 f32 f32)", "192 192 64.f32 64.f32 0.f32 128.f32 0.f32"],
 ["Average Face"  , "u8XY" , "()"                           , ""]]
var index = 0
demos.forEach(function(demo) {
 var fileName = demo[0].toLowerCase().replace(" ", "_");
 if (index % 2 == 0)
  html = html + '<div class="row">'
 html = html
  + '<div class="col-sm-12 col-md-6">'
  + '  <div class="thumbnail">'
  + '    <a href="?href=' + fileName + '">'
  + '      <img src="https://github.com/biometrics/likely/releases/download/v0.1/' + fileName + '.jpg" width=384 height=384>'
  + '    </a>'
  + '    <div class="caption">'
  + '      <h3>' + demo[0] + '</h3>'
  + '      <dl class="dl-horizontal">'
  + '        <dt>Likely Source File</dt>'
  + '        <dd><a href="http://raw.github.com/biometrics/likely/gh-pages/library/' + fileName + '.md">library/' + fileName + '.md</a></dd>'
  + '        <dt>Generated LLVM IR</dt>'
  + '        <dd><a href="https://s3.amazonaws.com/liblikely/ir/' + fileName + '.ll">serial</a> &'
  + '            <a href="https://s3.amazonaws.com/liblikely/ir/' + fileName + '-p.ll">parallel</a></dd>'
  + '        <dt>Algorithm</dt>'
  + '        <dd>' + fileName + '</dd>'
  + '        <dt>Return Type</dt>'
  + '        <dd>' + demo[1] + '</dd>'
  + '        <dt>Parameter Types</dt>'
  + '        <dd>' + demo[2] + '</dd>'
  + '        <dt>Example Arguments</dt>'
  + '        <dd>' + demo[3] + '</dd>'
  + '      </dl>'
  + '    </div>'
  + '  </div>'
  + '</div>'
 if (index % 2 == 1)
  html = html + '</div> <br>'
 index = index + 1
})
document.getElementById("demos").innerHTML = html;
</script>
