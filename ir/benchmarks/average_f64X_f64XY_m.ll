; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64X = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @average_tmp_thunk0({ %f64X* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64X* }, { %f64X* }* %0, i64 0, i32 0
  %4 = load %f64X*, %f64X** %3, align 8
  %5 = getelementptr inbounds %f64X, %f64X* %4, i64 0, i32 6, i64 0
  %6 = ptrtoint double* %5 to i64
  %7 = and i64 %6, 31
  %8 = icmp eq i64 %7, 0
  call void @llvm.assume(i1 %8)
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %9 = getelementptr %f64X, %f64X* %4, i64 0, i32 6, i64 %x
  store double 0.000000e+00, double* %9, align 8, !llvm.mem.parallel_loop_access !0
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !0

x_exit:                                           ; preds = %x_body
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: nounwind
define private void @average_tmp_thunk1({ %f64X*, float }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64X*, float }, { %f64X*, float }* %0, i64 0, i32 0
  %4 = load %f64X*, %f64X** %3, align 8
  %5 = getelementptr inbounds { %f64X*, float }, { %f64X*, float }* %0, i64 0, i32 1
  %6 = load float, float* %5, align 4
  %7 = getelementptr inbounds %f64X, %f64X* %4, i64 0, i32 6, i64 0
  %8 = ptrtoint double* %7 to i64
  %9 = and i64 %8, 31
  %10 = icmp eq i64 %9, 0
  call void @llvm.assume(i1 %10)
  %11 = fpext float %6 to double
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %12 = getelementptr %f64X, %f64X* %4, i64 0, i32 6, i64 %x
  %13 = load double, double* %12, align 8, !llvm.mem.parallel_loop_access !1
  %14 = fmul fast double %13, %11
  store double %14, double* %12, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !1

x_exit:                                           ; preds = %x_body
  ret void
}

define %f64X* @average(%f64XY*) {
entry:
  %1 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !2
  %2 = call %u0CXYT* @likely_new(i32 8512, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = zext i32 %columns to i64
  %4 = alloca { %f64X* }, align 8
  %5 = bitcast { %f64X* }* %4 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %5, align 8
  %6 = bitcast { %f64X* }* %4 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64X* }*, i64, i64)* @average_tmp_thunk0 to i8*), i8* %6, i64 %3)
  %7 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 4
  %rows = load i32, i32* %7, align 4, !range !2
  %8 = zext i32 %rows to i64
  %9 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %10 = bitcast %u0CXYT* %9 to double*
  %11 = ptrtoint %u0CXYT* %9 to i64
  %12 = and i64 %11, 31
  %13 = icmp eq i64 %12, 0
  call void @llvm.assume(i1 %13)
  %columns3 = load i32, i32* %1, align 4, !range !2
  %src_y_step = zext i32 %columns3 to i64
  %14 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 6, i64 0
  %15 = ptrtoint double* %14 to i64
  %16 = and i64 %15, 31
  %17 = icmp eq i64 %16, 0
  call void @llvm.assume(i1 %17)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %18 = mul nuw nsw i64 %y, %src_y_step
  br label %x_body

x_body:                                           ; preds = %x_body, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %x_body ]
  %19 = getelementptr double, double* %10, i64 %x
  %20 = load double, double* %19, align 8
  %21 = add nuw nsw i64 %x, %18
  %22 = getelementptr %f64XY, %f64XY* %0, i64 0, i32 6, i64 %21
  %23 = load double, double* %22, align 8
  %24 = fadd fast double %23, %20
  store double %24, double* %19, align 8
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %src_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %8
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %25 = bitcast %u0CXYT* %2 to %f64X*
  %26 = uitofp i32 %rows to float
  %27 = fdiv fast float 1.000000e+00, %26
  %28 = alloca { %f64X*, float }, align 8
  %29 = bitcast { %f64X*, float }* %28 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %29, align 8
  %30 = getelementptr inbounds { %f64X*, float }, { %f64X*, float }* %28, i64 0, i32 1
  store float %27, float* %30, align 8
  %31 = bitcast { %f64X*, float }* %28 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64X*, float }*, i64, i64)* @average_tmp_thunk1 to i8*), i8* %31, i64 %3)
  ret %f64X* %25
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = distinct !{!0}
!1 = distinct !{!1}
!2 = !{i32 1, i32 -1}
