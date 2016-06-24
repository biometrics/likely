; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64Matrix = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: norecurse nounwind
define private void @average_tmp_thunk0({ %f64Matrix*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64Matrix*, i32 }, { %f64Matrix*, i32 }* %0, i64 0, i32 0
  %4 = load %f64Matrix*, %f64Matrix** %3, align 8
  %5 = getelementptr inbounds { %f64Matrix*, i32 }, { %f64Matrix*, i32 }* %0, i64 0, i32 1
  %6 = load i32, i32* %5, align 4
  %7 = sitofp i32 %6 to double
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %8 = getelementptr %f64Matrix, %f64Matrix* %4, i64 0, i32 6, i64 %x
  store double %7, double* %8, align 8, !llvm.mem.parallel_loop_access !0
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  ret void
}

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: norecurse nounwind
define private void @average_tmp_thunk1({ %f64Matrix*, double }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f64Matrix*, double }, { %f64Matrix*, double }* %0, i64 0, i32 0
  %4 = load %f64Matrix*, %f64Matrix** %3, align 8
  %5 = getelementptr inbounds { %f64Matrix*, double }, { %f64Matrix*, double }* %0, i64 0, i32 1
  %6 = load double, double* %5, align 8
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %7 = getelementptr %f64Matrix, %f64Matrix* %4, i64 0, i32 6, i64 %x
  %8 = load double, double* %7, align 8, !llvm.mem.parallel_loop_access !1
  %9 = fmul fast double %8, %6
  store double %9, double* %7, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  ret void
}

; Function Attrs: nounwind
define noalias %f64Matrix* @average(%f64Matrix* noalias nocapture readonly) #2 {
entry:
  %1 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !2
  %2 = call %u0Matrix* @likely_new(i32 8512, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !2
  %4 = zext i32 %columns to i64
  %5 = alloca { %f64Matrix*, i32 }, align 8
  %6 = bitcast { %f64Matrix*, i32 }* %5 to %u0Matrix**
  store %u0Matrix* %2, %u0Matrix** %6, align 8
  %7 = getelementptr inbounds { %f64Matrix*, i32 }, { %f64Matrix*, i32 }* %5, i64 0, i32 1
  store i32 0, i32* %7, align 8
  %8 = bitcast { %f64Matrix*, i32 }* %5 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64Matrix*, i32 }*, i64, i64)* @average_tmp_thunk0 to i8*), i8* %8, i64 %4) #2
  %9 = zext i32 %rows to i64
  %10 = getelementptr inbounds %u0Matrix, %u0Matrix* %2, i64 1
  %11 = bitcast %u0Matrix* %10 to double*
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %12 = mul nuw nsw i64 %y, %4
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %13 = getelementptr double, double* %11, i64 %x
  %14 = load double, double* %13, align 8
  %15 = add nuw nsw i64 %x, %12
  %16 = getelementptr %f64Matrix, %f64Matrix* %0, i64 0, i32 6, i64 %15
  %17 = load double, double* %16, align 8
  %18 = fadd fast double %17, %14
  store double %18, double* %13, align 8
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %4
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %9
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %19 = bitcast %u0Matrix* %2 to %f64Matrix*
  %20 = icmp eq i32 %rows, 1
  br i1 %20, label %exit, label %true_entry

true_entry:                                       ; preds = %y_exit
  %21 = uitofp i32 %rows to double
  %22 = fdiv fast double 1.000000e+00, %21
  %23 = alloca { %f64Matrix*, double }, align 8
  %24 = bitcast { %f64Matrix*, double }* %23 to %u0Matrix**
  store %u0Matrix* %2, %u0Matrix** %24, align 8
  %25 = getelementptr inbounds { %f64Matrix*, double }, { %f64Matrix*, double }* %23, i64 0, i32 1
  store double %22, double* %25, align 8
  %26 = bitcast { %f64Matrix*, double }* %23 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64Matrix*, double }*, i64, i64)* @average_tmp_thunk1 to i8*), i8* %26, i64 %4) #2
  br label %exit

exit:                                             ; preds = %y_exit, %true_entry
  ret %f64Matrix* %19
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { norecurse nounwind }
attributes #2 = { nounwind }

!0 = distinct !{!0}
!1 = distinct !{!1}
!2 = !{i32 1, i32 -1}
