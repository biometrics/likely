; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64X = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f64X* @average(%f64XY*) {
entry:
  %1 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 8512, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = zext i32 %columns to i64
  %4 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %5 = bitcast %u0CXYT* %4 to double*
  %6 = ptrtoint %u0CXYT* %4 to i64
  %7 = and i64 %6, 31
  %8 = icmp eq i64 %7, 0
  call void @llvm.assume(i1 %8)
  %scevgep1 = bitcast %u0CXYT* %4 to i8*
  %9 = shl nuw nsw i64 %3, 3
  call void @llvm.memset.p0i8.i64(i8* %scevgep1, i8 0, i64 %9, i32 8, i1 false)
  %10 = bitcast %u0CXYT* %2 to %f64X*
  %11 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 4
  %rows = load i32, i32* %11, align 4, !range !0
  %12 = zext i32 %rows to i64
  %13 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 6, i64 0
  %14 = ptrtoint double* %13 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  call void @llvm.assume(i1 %16)
  br label %y_body

y_body:                                           ; preds = %x_exit8, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit8 ]
  %17 = mul nuw nsw i64 %y, %3
  br label %x_body7

x_body7:                                          ; preds = %x_body7, %y_body
  %x9 = phi i64 [ 0, %y_body ], [ %x_increment10, %x_body7 ]
  %18 = getelementptr double, double* %5, i64 %x9
  %19 = load double, double* %18, align 8
  %20 = add nuw nsw i64 %x9, %17
  %21 = getelementptr %f64XY, %f64XY* %0, i64 0, i32 6, i64 %20
  %22 = load double, double* %21, align 8
  %23 = fadd fast double %22, %19
  store double %23, double* %18, align 8
  %x_increment10 = add nuw nsw i64 %x9, 1
  %x_postcondition11 = icmp eq i64 %x_increment10, %3
  br i1 %x_postcondition11, label %x_exit8, label %x_body7

x_exit8:                                          ; preds = %x_body7
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %12
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit8
  %24 = uitofp i32 %rows to float
  %25 = fdiv fast float 1.000000e+00, %24
  %26 = fpext float %25 to double
  br label %x_body16

x_body16:                                         ; preds = %x_body16, %y_exit
  %x18 = phi i64 [ 0, %y_exit ], [ %x_increment19, %x_body16 ]
  %27 = getelementptr double, double* %5, i64 %x18
  %28 = load double, double* %27, align 8, !llvm.mem.parallel_loop_access !1
  %29 = fmul fast double %28, %26
  store double %29, double* %27, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment19 = add nuw nsw i64 %x18, 1
  %x_postcondition20 = icmp eq i64 %x_increment19, %3
  br i1 %x_postcondition20, label %x_exit17, label %x_body16, !llvm.loop !1

x_exit17:                                         ; preds = %x_body16
  ret %f64X* %10
}

; Function Attrs: nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #1

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
