; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
define private void @matrix_multiplication_tmp_thunk0({ %f64XY*, %f64XY*, %f64XY*, i32 }* noalias nocapture readonly, i64, i64) #0 {
entry:
  %3 = getelementptr inbounds { %f64XY*, %f64XY*, %f64XY*, i32 }, { %f64XY*, %f64XY*, %f64XY*, i32 }* %0, i64 0, i32 0
  %4 = load %f64XY*, %f64XY** %3, align 8
  %5 = getelementptr inbounds { %f64XY*, %f64XY*, %f64XY*, i32 }, { %f64XY*, %f64XY*, %f64XY*, i32 }* %0, i64 0, i32 1
  %6 = load %f64XY*, %f64XY** %5, align 8
  %7 = getelementptr inbounds { %f64XY*, %f64XY*, %f64XY*, i32 }, { %f64XY*, %f64XY*, %f64XY*, i32 }* %0, i64 0, i32 2
  %8 = load %f64XY*, %f64XY** %7, align 8
  %9 = getelementptr inbounds { %f64XY*, %f64XY*, %f64XY*, i32 }, { %f64XY*, %f64XY*, %f64XY*, i32 }* %0, i64 0, i32 3
  %10 = load i32, i32* %9, align 4
  %11 = getelementptr inbounds %f64XY, %f64XY* %8, i64 0, i32 3
  %columns3 = load i32, i32* %11, align 4, !range !0
  %C_y_step = zext i32 %columns3 to i64
  %12 = getelementptr inbounds %f64XY, %f64XY* %4, i64 0, i32 6, i64 0
  %13 = ptrtoint double* %12 to i64
  %14 = and i64 %13, 31
  %15 = icmp eq i64 %14, 0
  call void @llvm.assume(i1 %15)
  %16 = getelementptr inbounds %f64XY, %f64XY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %16, align 4, !range !0
  %A_y_step = zext i32 %columns1 to i64
  %17 = getelementptr inbounds %f64XY, %f64XY* %6, i64 0, i32 6, i64 0
  %18 = ptrtoint double* %17 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  call void @llvm.assume(i1 %20)
  %21 = getelementptr inbounds %f64XY, %f64XY* %8, i64 0, i32 6, i64 0
  %22 = ptrtoint double* %21 to i64
  %23 = and i64 %22, 31
  %24 = icmp eq i64 %23, 0
  call void @llvm.assume(i1 %24)
  %25 = icmp eq i32 %10, 0
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %26 = mul nuw nsw i64 %y, %C_y_step
  %27 = mul nuw nsw i64 %y, %A_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %exit
  %x = phi i64 [ %x_increment, %exit ], [ 0, %y_body ]
  br i1 %25, label %exit, label %true_entry

true_entry:                                       ; preds = %x_body, %true_entry
  %28 = phi i32 [ %40, %true_entry ], [ 0, %x_body ]
  %29 = phi double [ %39, %true_entry ], [ 0.000000e+00, %x_body ]
  %30 = sext i32 %28 to i64
  %31 = add nuw nsw i64 %30, %27
  %32 = getelementptr %f64XY, %f64XY* %6, i64 0, i32 6, i64 %31
  %33 = load double, double* %32, align 8, !llvm.mem.parallel_loop_access !1
  %34 = mul nuw nsw i64 %30, %C_y_step
  %35 = add nuw nsw i64 %34, %x
  %36 = getelementptr %f64XY, %f64XY* %8, i64 0, i32 6, i64 %35
  %37 = load double, double* %36, align 8, !llvm.mem.parallel_loop_access !1
  %38 = fmul fast double %37, %33
  %39 = fadd fast double %38, %29
  %40 = add nuw nsw i32 %28, 1
  %41 = icmp eq i32 %40, %10
  br i1 %41, label %exit, label %true_entry

exit:                                             ; preds = %true_entry, %x_body
  %.lcssa = phi double [ 0.000000e+00, %x_body ], [ %39, %true_entry ]
  %42 = add nuw nsw i64 %x, %26
  %43 = getelementptr %f64XY, %f64XY* %4, i64 0, i32 6, i64 %42
  store double %.lcssa, double* %43, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %C_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %f64XY* @matrix_multiplication(%f64XY*, %f64XY*) {
entry:
  %2 = getelementptr inbounds %f64XY, %f64XY* %1, i64 0, i32 4
  %rows = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 3
  %columns = load i32, i32* %3, align 4, !range !0
  %4 = icmp eq i32 %rows, %columns
  call void @llvm.assume(i1 %4)
  %5 = getelementptr inbounds %f64XY, %f64XY* %1, i64 0, i32 3
  %columns1 = load i32, i32* %5, align 4, !range !0
  %6 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 4
  %rows2 = load i32, i32* %6, align 4, !range !0
  %7 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %columns1, i32 %rows2, i32 1, i8* null)
  %C = bitcast %u0CXYT* %7 to %f64XY*
  %8 = zext i32 %rows2 to i64
  %9 = alloca { %f64XY*, %f64XY*, %f64XY*, i32 }, align 8
  %10 = bitcast { %f64XY*, %f64XY*, %f64XY*, i32 }* %9 to %u0CXYT**
  store %u0CXYT* %7, %u0CXYT** %10, align 8
  %11 = getelementptr inbounds { %f64XY*, %f64XY*, %f64XY*, i32 }, { %f64XY*, %f64XY*, %f64XY*, i32 }* %9, i64 0, i32 1
  store %f64XY* %0, %f64XY** %11, align 8
  %12 = getelementptr inbounds { %f64XY*, %f64XY*, %f64XY*, i32 }, { %f64XY*, %f64XY*, %f64XY*, i32 }* %9, i64 0, i32 2
  store %f64XY* %1, %f64XY** %12, align 8
  %13 = getelementptr inbounds { %f64XY*, %f64XY*, %f64XY*, i32 }, { %f64XY*, %f64XY*, %f64XY*, i32 }* %9, i64 0, i32 3
  store i32 %columns, i32* %13, align 8
  %14 = bitcast { %f64XY*, %f64XY*, %f64XY*, i32 }* %9 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64XY*, %f64XY*, %f64XY*, i32 }*, i64, i64)* @matrix_multiplication_tmp_thunk0 to i8*), i8* %14, i64 %8)
  ret %f64XY* %C
}

attributes #0 = { nounwind }
attributes #1 = { nounwind readonly }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
