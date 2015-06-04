; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64X = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @average_tmp_thunk0({ %f64X*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64X*, i32 }, { %f64X*, i32 }* %0, i64 0, i32 0
  %4 = load %f64X*, %f64X** %3, align 8
  %5 = getelementptr inbounds { %f64X*, i32 }, { %f64X*, i32 }* %0, i64 0, i32 1
  %6 = load i32, i32* %5, align 4
  %7 = getelementptr inbounds %f64X, %f64X* %4, i64 0, i32 6, i64 0
  %8 = ptrtoint double* %7 to i64
  %9 = and i64 %8, 31
  %10 = icmp eq i64 %9, 0
  call void @llvm.assume(i1 %10)
  %11 = sitofp i32 %6 to double
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %12 = getelementptr %f64X, %f64X* %4, i64 0, i32 6, i64 %x
  store double %11, double* %12, align 8, !llvm.mem.parallel_loop_access !0
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: nounwind
define private void @average_tmp_thunk1({ %f64X*, double }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64X*, double }, { %f64X*, double }* %0, i64 0, i32 0
  %4 = load %f64X*, %f64X** %3, align 8
  %5 = getelementptr inbounds { %f64X*, double }, { %f64X*, double }* %0, i64 0, i32 1
  %6 = load double, double* %5, align 8
  %7 = getelementptr inbounds %f64X, %f64X* %4, i64 0, i32 6, i64 0
  %8 = ptrtoint double* %7 to i64
  %9 = and i64 %8, 31
  %10 = icmp eq i64 %9, 0
  call void @llvm.assume(i1 %10)
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %11 = getelementptr %f64X, %f64X* %4, i64 0, i32 6, i64 %x
  %12 = load double, double* %11, align 8, !llvm.mem.parallel_loop_access !1
  %13 = fmul fast double %12, %6
  store double %13, double* %11, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  ret void
}

define %f64X* @average(%f64XY*) {
entry:
  %1 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !2
  %2 = call %u0CXYT* @likely_new(i32 8512, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !2
  %4 = zext i32 %columns to i64
  %5 = alloca { %f64X*, i32 }, align 8
  %6 = bitcast { %f64X*, i32 }* %5 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %6, align 8
  %7 = getelementptr inbounds { %f64X*, i32 }, { %f64X*, i32 }* %5, i64 0, i32 1
  store i32 0, i32* %7, align 8
  %8 = bitcast { %f64X*, i32 }* %5 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64X*, i32 }*, i64, i64)* @average_tmp_thunk0 to i8*), i8* %8, i64 %4)
  %rows2 = load i32, i32* %3, align 4, !range !2
  %9 = zext i32 %rows2 to i64
  %10 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %11 = bitcast %u0CXYT* %10 to double*
  %12 = ptrtoint %u0CXYT* %10 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  call void @llvm.assume(i1 %14)
  %columns4 = load i32, i32* %1, align 4, !range !2
  %src_y_step = zext i32 %columns4 to i64
  %15 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 6, i64 0
  %16 = ptrtoint double* %15 to i64
  %17 = and i64 %16, 31
  %18 = icmp eq i64 %17, 0
  call void @llvm.assume(i1 %18)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %19 = mul nuw nsw i64 %y, %src_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %20 = getelementptr double, double* %11, i64 %x
  %21 = load double, double* %20, align 8
  %22 = add nuw nsw i64 %x, %19
  %23 = getelementptr %f64XY, %f64XY* %0, i64 0, i32 6, i64 %22
  %24 = load double, double* %23, align 8
  %25 = fadd fast double %24, %21
  store double %25, double* %20, align 8
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %src_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %9
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %26 = bitcast %u0CXYT* %2 to %f64X*
  %27 = icmp eq i32 %rows, 1
  br i1 %27, label %exit, label %true_entry

true_entry:                                       ; preds = %y_exit
  %28 = uitofp i32 %rows to double
  %29 = fdiv fast double 1.000000e+00, %28
  %30 = alloca { %f64X*, double }, align 8
  %31 = bitcast { %f64X*, double }* %30 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %31, align 8
  %32 = getelementptr inbounds { %f64X*, double }, { %f64X*, double }* %30, i64 0, i32 1
  store double %29, double* %32, align 8
  %33 = bitcast { %f64X*, double }* %30 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64X*, double }*, i64, i64)* @average_tmp_thunk1 to i8*), i8* %33, i64 %4)
  br label %exit

exit:                                             ; preds = %y_exit, %true_entry
  ret %f64X* %26
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = distinct !{!0}
!1 = distinct !{!1}
!2 = !{i32 1, i32 -1}
