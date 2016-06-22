; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64Matrix = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define noalias %f64Matrix* @multiply_transposed(%f64Matrix* noalias nocapture readonly, %f64Matrix* noalias nocapture readonly) #1 {
entry:
  %2 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = call %u0Matrix* @likely_new(i32 24896, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %5 = zext i32 %rows to i64
  %mat_y_step = zext i32 %columns to i64
  %6 = getelementptr inbounds %u0Matrix, %u0Matrix* %4, i64 1
  %scevgep = getelementptr %u0Matrix, %u0Matrix* %4, i64 1, i32 0
  %7 = shl nuw nsw i64 %mat_y_step, 1
  %scevgep3 = getelementptr %f64Matrix, %f64Matrix* %0, i64 1, i32 0
  %8 = shl nuw nsw i64 %mat_y_step, 3
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %9 = mul i64 %y, %7
  %scevgep1 = getelementptr i32, i32* %scevgep, i64 %9
  %scevgep12 = bitcast i32* %scevgep1 to i8*
  %scevgep4 = getelementptr i32, i32* %scevgep3, i64 %9
  %scevgep45 = bitcast i32* %scevgep4 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep12, i8* %scevgep45, i64 %8, i32 8, i1 false)
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %5
  br i1 %y_postcondition, label %y_body15.preheader, label %y_body

y_body15.preheader:                               ; preds = %y_body
  %10 = bitcast %u0Matrix* %6 to double*
  br label %y_body15

y_body15:                                         ; preds = %x_exit19, %y_body15.preheader
  %y17 = phi i64 [ 0, %y_body15.preheader ], [ %y_increment23, %x_exit19 ]
  %11 = mul nuw nsw i64 %y17, %mat_y_step
  br label %x_body18

x_body18:                                         ; preds = %y_body15, %x_body18
  %x20 = phi i64 [ %x_increment21, %x_body18 ], [ 0, %y_body15 ]
  %12 = add nuw nsw i64 %x20, %11
  %13 = getelementptr double, double* %10, i64 %12
  %14 = load double, double* %13, align 8, !llvm.mem.parallel_loop_access !1
  %15 = getelementptr %f64Matrix, %f64Matrix* %1, i64 0, i32 6, i64 %x20
  %16 = load double, double* %15, align 8, !llvm.mem.parallel_loop_access !1
  %17 = fsub fast double %14, %16
  store double %17, double* %13, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment21 = add nuw nsw i64 %x20, 1
  %x_postcondition22 = icmp eq i64 %x_increment21, %mat_y_step
  br i1 %x_postcondition22, label %x_exit19, label %x_body18

x_exit19:                                         ; preds = %x_body18
  %y_increment23 = add nuw nsw i64 %y17, 1
  %y_postcondition24 = icmp eq i64 %y_increment23, %5
  br i1 %y_postcondition24, label %y_exit16, label %y_body15

y_exit16:                                         ; preds = %x_exit19
  %18 = call %u0Matrix* @likely_new(i32 24896, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %19 = getelementptr inbounds %u0Matrix, %u0Matrix* %18, i64 1
  %20 = bitcast %u0Matrix* %19 to double*
  br label %y_body33

y_body33:                                         ; preds = %x_exit37, %y_exit16
  %y35 = phi i64 [ 0, %y_exit16 ], [ %y_increment43, %x_exit37 ]
  %21 = mul nuw nsw i64 %y35, %mat_y_step
  br label %x_body36

x_body36:                                         ; preds = %y_body33, %exit
  %x38 = phi i64 [ %x_increment41, %exit ], [ 0, %y_body33 ]
  %22 = icmp ugt i64 %y35, %x38
  br i1 %22, label %exit, label %true_entry39

true_entry39:                                     ; preds = %x_body36, %true_entry39
  %23 = phi i32 [ %35, %true_entry39 ], [ 0, %x_body36 ]
  %24 = phi double [ %34, %true_entry39 ], [ 0.000000e+00, %x_body36 ]
  %25 = zext i32 %23 to i64
  %26 = mul nuw nsw i64 %25, %mat_y_step
  %27 = add nuw nsw i64 %26, %x38
  %28 = getelementptr double, double* %10, i64 %27
  %29 = load double, double* %28, align 8, !llvm.mem.parallel_loop_access !2
  %30 = add nuw nsw i64 %26, %y35
  %31 = getelementptr double, double* %10, i64 %30
  %32 = load double, double* %31, align 8, !llvm.mem.parallel_loop_access !2
  %33 = fmul fast double %32, %29
  %34 = fadd fast double %33, %24
  %35 = add nuw nsw i32 %23, 1
  %36 = icmp eq i32 %35, %rows
  br i1 %36, label %exit40, label %true_entry39

exit40:                                           ; preds = %true_entry39
  %37 = add nuw nsw i64 %x38, %21
  %38 = getelementptr double, double* %20, i64 %37
  store double %34, double* %38, align 8, !llvm.mem.parallel_loop_access !2
  %39 = mul nuw nsw i64 %x38, %mat_y_step
  %40 = add nuw nsw i64 %39, %y35
  %41 = getelementptr double, double* %20, i64 %40
  store double %34, double* %41, align 8, !llvm.mem.parallel_loop_access !2
  br label %exit

exit:                                             ; preds = %exit40, %x_body36
  %x_increment41 = add nuw nsw i64 %x38, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %mat_y_step
  br i1 %x_postcondition42, label %x_exit37, label %x_body36

x_exit37:                                         ; preds = %exit
  %y_increment43 = add nuw nsw i64 %y35, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %mat_y_step
  br i1 %y_postcondition44, label %y_exit34, label %y_body33

y_exit34:                                         ; preds = %x_exit37
  %dst = bitcast %u0Matrix* %18 to %f64Matrix*
  %42 = bitcast %u0Matrix* %4 to i8*
  call void @likely_release_mat(i8* %42) #1
  ret %f64Matrix* %dst
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture readonly, i64, i32, i1) #0

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
