Demos
-----
Ways to run a demo from the command line:

```bash
$ likely <file> -show
$ likely <file> -render result.png
$ dream <file> # Likely's IDE
```

<div class="row" id="demos"></div>
<script>
var html = ""
var demos = ["Hello World", "Mandelbrot Set", "Gabor Wavelet", "Average Face"]
demos.forEach(function(demo) {
 var fileName = demo.toLowerCase().replace(" ", "_");
 html = html + '<div class="col-sm-6 col-md-4">'
  + '  <div class="thumbnail">'
  + '    <a href="?href=' + fileName + '">'
  + '      <img src="https://github.com/biometrics/likely/releases/download/v0.1/' + fileName + '.jpg">'
  + '    </a>'
  + '    <div class="caption">'
  + '      <h3>' + demo + '</h3>'
  + '      <dl class="dl-horizontal">'
  + '        <dt>Likely Source</dt>'
  + '        <dd><a href="http://raw.github.com/biometrics/likely/gh-pages/library/"' + fileName + '.md">library/' + fileName + '.md</a></dd>'
  + '        <dt>Generated LLVM IR</dt>'
  + '        <dd><a href="https://s3.amazonaws.com/liblikely/ir/' + fileName + '.ll">serial</a> &'
  + '            <a href="https://s3.amazonaws.com/liblikely/ir/' + fileName + '-p.ll">parallel</a></dd>'
  + '      </dl>'
  + '    </div>'
  + '  </div>'
  + '</div>'
})
document.getElementById("demos").innerHTML = html;
</script>

### Static Compilation
Likely source code can also be statically compiled to object files with a C ABI.
For example:

```bash
$ likely -O3 -c '"library/<algorithm>.md".(import this) (extern <return_type> "likely_test_function" <parameter_types> <algorithm> true)' <algorithm>.o
$ clang src/main.c <algorithm>.o -llikely -o <algorithm> # specify -I and -L as needed
$ ./<algorithm> - <arguments>
```

#### Enabling/Disabling Static Compilation & Testing
When building Likely, demo compilation and testing is controlled by the *CMake* variable **LIKELY_DEMOS** which defaults to **Default**.
Demo compilation and testing can be disabled with **LIKELY_DEMOS=None**.
Certain computationally demanding demos must be explicitly enabled with **LIKELY_DEMOS=All**.
