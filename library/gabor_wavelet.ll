Gabor Wavelet
-------------

Wavelet parameters

    lambda = 128 ; wavelength
    psi    = 0.0 ; phase offset
    gamma  = 1.0 ; aspect ratio
    sigma  = 64  ; standard deviation
    theta  = 0   ; orientation
    n_stddev = 3 ; bounding box size

Interaction

    theta_rad = (theta + gabor_wavelet_angle ?? 0) * pi / 180
    lambda_norm = lambda * gabor_wavelet_scale ?? 1

Derived values

    sigma_x = sigma
    sigma_y = sigma / gamma
    x_max = (max (max (n_stddev * sigma_x * theta_rad.cos).abs
                      (n_stddev * sigma_y * gamma * theta_rad.sin).abs)
                 1).ceil
    y_max = (max (max (n_stddev * sigma_x * theta_rad.cos).abs
                      (n_stddev * sigma_y * gamma * theta_rad.sin).abs)
                 1).ceil

Definition

    (gabor_wavelet i32 i32 f32 f32 f32 f32 f32) =
      (x_max y_max sigma_x sigma_y theta lambda psi) =>
    {
      dx = x - x_max
      dy = y - y_max
      xp =      dx * theta.cos + dy * theta.sin
      yp = -1 * dx * theta.sin + dy * theta.cos
      (-0.5 * ((xp / sigma_x).sq + (yp / sigma_y).sq)).exp * (2 * pi / lambda * xp + psi).cos
    } : ((columns 2 * x_max + 1) (rows 2 * y_max + 1) (type parallel))

Execution

    (gabor_wavelet x_max y_max sigma_x sigma_y theta_rad lambda_norm psi)
