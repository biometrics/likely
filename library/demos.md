Demos
-----
Ways to run a demo from the command line:

```bash
$ dream <file>
$ likely <file> -show
$ likely <file> -render result.png
```

Be sure to also check out [algorithms](?href=algorithms).

<div class="row" id="demos"></div>

<script>
    var html = ""
    var demos = ["Hello World", "Gabor Wavelet", "Mandelbrot Set"]
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
