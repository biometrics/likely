Gabor Wavelet
-------------
Wavelet parameters

    lambda:= 128 ; wavelength
    psi   := 0.0 ; phase offset
    gamma := 1.0 ; aspect ratio
    sigma := 64  ; standard deviation
    theta := 0   ; orientation
    n_stddev:= 3 ; bounding box size

Interaction

    theta_rad:= theta:+ (try gabor_wavelet_angle 0).(* pi).(/ 180)
    lambda_norm:= lambda:* (try gabor_wavelet_scale 1)
    radius:= (try gabor_wavelet_width 385).(- 1).(/ 2)

Definition

    (gabor_wavelet i32 i32 f32 f32 f32 f32 f32):=
      (=> (x_max y_max sigma_x sigma_y theta lambda psi)
    {
      dx:= (- x.i32 x_max)
      dy:= (- y.i32 y_max)
      xp:= (* dx theta.cos):+ (* dy theta.sin)
      yp:= (* -1:* dx theta.sin):+ (* dy theta.cos)
      (* -0.5 (/ xp sigma_x).sq:+ (/ yp sigma_y).sq).exp:* (+ (* (/ (* 2 pi) lambda) xp) psi).cos
    } ((+ 2:* x_max 1).columns (+ 2:* y_max 1).rows) )

Execution

    (gabor_wavelet radius radius sigma (/ sigma gamma) theta_rad lambda_norm psi)
