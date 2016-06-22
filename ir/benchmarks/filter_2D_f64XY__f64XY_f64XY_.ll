; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64Matrix = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
define noalias %f64Matrix* @filter_2D(%f64Matrix* noalias nocapture readonly, %f64Matrix* noalias nocapture readonly) #0 {
entry:
  %2 = getelementptr inbounds %f64Matrix, %f64Matrix* %1, i64 0, i32 3
  %width = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f64Matrix, %f64Matrix* %1, i64 0, i32 4
  %height = load i32, i32* %3, align 4, !range !0
  %4 = srem i32 %width, 2
  %5 = icmp eq i32 %4, 1
  call void @llvm.assume(i1 %5)
  %6 = srem i32 %height, 2
  %7 = icmp eq i32 %6, 1
  call void @llvm.assume(i1 %7)
  %8 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %8, align 4, !range !0
  %9 = add i32 %width, -1
  %10 = add nuw nsw i32 %columns, %9
  %11 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %11, align 4, !range !0
  %12 = add i32 %height, -1
  %13 = add nuw nsw i32 %rows, %12
  %14 = call %u0Matrix* @likely_new(i32 24896, i32 1, i32 %10, i32 %13, i32 1, i8* null)
  %15 = zext i32 %13 to i64
  %mat_y_step = zext i32 %10 to i64
  %16 = getelementptr inbounds %u0Matrix, %u0Matrix* %14, i64 1
  %scevgep7 = getelementptr %u0Matrix, %u0Matrix* %14, i64 1, i32 0
  %17 = shl nuw nsw i64 %mat_y_step, 1
  %18 = shl nuw nsw i64 %mat_y_step, 3
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %19 = mul i64 %y, %17
  %scevgep8 = getelementptr i32, i32* %scevgep7, i64 %19
  %scevgep89 = bitcast i32* %scevgep8 to i8*
  call void @llvm.memset.p0i8.i64(i8* %scevgep89, i8 0, i64 %18, i32 8, i1 false)
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %15
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %20 = bitcast %u0Matrix* %16 to double*
  %pad-columns = sdiv i32 %9, 2
  %pad-rows = sdiv i32 %12, 2
  %21 = zext i32 %rows to i64
  %src_y_step = zext i32 %columns to i64
  %22 = sext i32 %pad-rows to i64
  %23 = mul nsw i64 %mat_y_step, %22
  %24 = sext i32 %pad-columns to i64
  %25 = add i64 %23, %24
  %26 = mul i64 %25, 2
  %scevgep4 = getelementptr %f64Matrix, %f64Matrix* %0, i64 1, i32 0
  %27 = mul nuw nsw i64 %src_y_step, 2
  %28 = shl nuw nsw i64 %src_y_step, 3
  br label %y_body9

y_body9:                                          ; preds = %y_body9, %y_exit
  %y11 = phi i64 [ 0, %y_exit ], [ %y_increment17, %y_body9 ]
  %29 = mul i64 %y11, %17
  %30 = add i64 %29, %26
  %scevgep2 = getelementptr i32, i32* %scevgep7, i64 %30
  %scevgep23 = bitcast i32* %scevgep2 to i8*
  %31 = mul i64 %27, %y11
  %scevgep5 = getelementptr i32, i32* %scevgep4, i64 %31
  %scevgep56 = bitcast i32* %scevgep5 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep23, i8* %scevgep56, i64 %28, i32 8, i1 false)
  %y_increment17 = add nuw nsw i64 %y11, 1
  %y_postcondition18 = icmp eq i64 %y_increment17, %21
  br i1 %y_postcondition18, label %y_exit10, label %y_body9

y_exit10:                                         ; preds = %y_body9
  %32 = call %u0Matrix* @likely_new(i32 24896, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %33 = getelementptr inbounds %u0Matrix, %u0Matrix* %32, i64 1
  %34 = bitcast %u0Matrix* %33 to double*
  %kernel_y_step = zext i32 %width to i64
  br label %y_body31

y_body31:                                         ; preds = %x_exit35, %y_exit10
  %y33 = phi i64 [ 0, %y_exit10 ], [ %y_increment43, %x_exit35 ]
  %35 = mul nuw nsw i64 %y33, %src_y_step
  br label %x_body34

x_body34:                                         ; preds = %y_body31, %exit
  %x36 = phi i64 [ %x_increment41, %exit ], [ 0, %y_body31 ]
  %36 = add nuw nsw i64 %x36, %35
  br label %loop38.preheader

loop38.preheader:                                 ; preds = %x_body34, %exit40
  %37 = phi i32 [ %59, %exit40 ], [ 0, %x_body34 ]
  %38 = phi double [ %56, %exit40 ], [ 0.000000e+00, %x_body34 ]
  %39 = zext i32 %37 to i64
  %40 = add nuw nsw i64 %39, %y33
  %41 = mul nuw nsw i64 %40, %mat_y_step
  %42 = add i64 %41, %x36
  %43 = mul nuw nsw i64 %39, %kernel_y_step
  br label %true_entry39

exit:                                             ; preds = %exit40
  %44 = getelementptr double, double* %34, i64 %36
  store double %56, double* %44, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment41 = add nuw nsw i64 %x36, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %src_y_step
  br i1 %x_postcondition42, label %x_exit35, label %x_body34

x_exit35:                                         ; preds = %exit
  %y_increment43 = add nuw nsw i64 %y33, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %21
  br i1 %y_postcondition44, label %y_exit32, label %y_body31

y_exit32:                                         ; preds = %x_exit35
  %dst = bitcast %u0Matrix* %32 to %f64Matrix*
  %45 = bitcast %u0Matrix* %14 to i8*
  call void @likely_release_mat(i8* %45) #0
  ret %f64Matrix* %dst

true_entry39:                                     ; preds = %loop38.preheader, %true_entry39
  %46 = phi double [ %56, %true_entry39 ], [ %38, %loop38.preheader ]
  %47 = phi i32 [ %57, %true_entry39 ], [ 0, %loop38.preheader ]
  %48 = zext i32 %47 to i64
  %49 = add i64 %42, %48
  %50 = getelementptr double, double* %20, i64 %49
  %51 = load double, double* %50, align 8, !llvm.mem.parallel_loop_access !1
  %52 = add nuw nsw i64 %48, %43
  %53 = getelementptr %f64Matrix, %f64Matrix* %1, i64 0, i32 6, i64 %52
  %54 = load double, double* %53, align 8, !llvm.mem.parallel_loop_access !1
  %55 = fmul fast double %54, %51
  %56 = fadd fast double %55, %46
  %57 = add nuw nsw i32 %47, 1
  %58 = icmp eq i32 %57, %width
  br i1 %58, label %exit40, label %true_entry39

exit40:                                           ; preds = %true_entry39
  %59 = add nuw nsw i32 %37, 1
  %60 = icmp eq i32 %59, %height
  br i1 %60, label %exit, label %loop38.preheader
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture readonly, i64, i32, i1) #1

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #1

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind }
attributes #1 = { argmemonly nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
