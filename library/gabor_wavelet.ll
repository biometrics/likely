Gabor Wavelet
-------------

    lambda = 5.0 ; wavelength
    psi    = 0.0 ; phase offset
    gamma  = 1 ; aspect ratio
    sigma  = 3 ; standard deviation
    theta  = 0 ; orientation
    resolution = 32

    sigma_x = sigma
    sigma_y = sigma / gamma

    nstddev = 3
    x_max = (ceil (max (max (fabs nstddev * sigma_x * (cos theta)) (fabs nstddev * sigma_y * gamma * (sin theta))) 1.f32))
    y_max = (ceil (max (max (fabs nstddev * sigma_x * (sin theta)) (fabs nstddev * sigma_y * gamma * (cos theta))) 1.f32))

    (gabor f32 f32 f32 f32 f32 f32 f32 f32) =
      (resolution x_max y_max sigma_x sigma_y theta lambda psi) =>
    {
      dx = x.f32 / resolution - x_max
      dy = y.f32 / resolution - y_max
      xp = dx * (cos theta) + dy * (sin theta)
      yp = -1 * dx * (sin theta) + dy * (cos theta)
      (exp -0.5 * ((xp * xp) / (sigma_x * sigma_x) + (yp * yp) / (sigma_y * sigma_y))) * (cos 2 * pi / lambda * xp + psi)
    } : ((columns resolution * (2 * x_max + 1)) (rows resolution * (2 * y_max + 1)) (type parallel))

    (gabor resolution x_max y_max sigma_x sigma_y theta lambda psi)
