Gabor Wavelet
-------------

    lambda = 128 ; wavelength
    psi    = 0.0 ; phase offset
    gamma  = 1.0 ; aspect ratio
    sigma  = 64  ; standard deviation
    theta  = 0.0 ; orientation

    sigma_x = sigma
    sigma_y = sigma / gamma

    n_stddev = 3
    x_max = (max (max (n_stddev * sigma_x * theta.cos).abs
                      (n_stddev * sigma_y * gamma * theta.sin).abs)
                 1).ceil
    y_max = (max (max (n_stddev * sigma_x * theta.cos).abs
                      (n_stddev * sigma_y * gamma * theta.sin).abs)
                 1).ceil

    (gabor i32 i32 f32 f32 f32 f32 f32) =
      (x_max y_max sigma_x sigma_y theta lambda psi) =>
    {
      dx = x - x_max
      dy = y - y_max
      xp =      dx * theta.cos + dy * theta.sin
      yp = -1 * dx * theta.sin + dy * theta.cos
      (-0.5 * ((xp / sigma_x).sq + (yp / sigma_y).sq)).exp * (2 * pi / lambda * xp + psi).cos
    } : ((columns 2 * x_max + 1) (rows 2 * y_max + 1) (type parallel))

    (gabor x_max y_max sigma_x sigma_y theta lambda psi)
