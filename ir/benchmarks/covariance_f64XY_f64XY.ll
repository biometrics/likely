; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64Matrix = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

define noalias %f64Matrix* @covariance(%f64Matrix* nocapture readonly) {
entry:
  %1 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0Matrix* @likely_new(i32 8512, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %f64Matrix, %f64Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0Matrix, %u0Matrix* %2, i64 1
  %6 = bitcast %u0Matrix* %5 to i8*
  %7 = bitcast %u0Matrix* %5 to double*
  %8 = shl nuw nsw i64 %4, 3
  call void @llvm.memset.p0i8.i64(i8* %6, i8 0, i64 %8, i32 8, i1 false)
  %9 = zext i32 %rows to i64
  br label %y_body

y_body:                                           ; preds = %x_exit8, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit8 ]
  %10 = mul nuw nsw i64 %y, %4
  br label %x_body7

x_body7:                                          ; preds = %y_body, %x_body7
  %x9 = phi i64 [ %x_increment10, %x_body7 ], [ 0, %y_body ]
  %11 = getelementptr double, double* %7, i64 %x9
  %12 = load double, double* %11, align 8
  %13 = add nuw nsw i64 %x9, %10
  %14 = getelementptr %f64Matrix, %f64Matrix* %0, i64 0, i32 6, i64 %13
  %15 = load double, double* %14, align 8
  %16 = fadd fast double %15, %12
  store double %16, double* %11, align 8
  %x_increment10 = add nuw nsw i64 %x9, 1
  %x_postcondition11 = icmp eq i64 %x_increment10, %4
  br i1 %x_postcondition11, label %x_exit8, label %x_body7

x_exit8:                                          ; preds = %x_body7
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %9
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit8
  %17 = icmp eq i32 %rows, 1
  br i1 %17, label %Flow7, label %true_entry

true_entry:                                       ; preds = %y_exit
  %18 = uitofp i32 %rows to double
  %19 = fdiv fast double 1.000000e+00, %18
  br label %x_body15

Flow7:                                            ; preds = %x_body15, %y_exit
  %20 = call %u0Matrix* @likely_new(i32 24896, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %21 = getelementptr inbounds %u0Matrix, %u0Matrix* %20, i64 1
  %scevgep = getelementptr %u0Matrix, %u0Matrix* %20, i64 1, i32 0
  %22 = shl nuw nsw i64 %4, 1
  %scevgep3 = getelementptr %f64Matrix, %f64Matrix* %0, i64 1, i32 0
  br label %y_body28

x_body15:                                         ; preds = %true_entry, %x_body15
  %x17 = phi i64 [ %x_increment18, %x_body15 ], [ 0, %true_entry ]
  %23 = getelementptr double, double* %7, i64 %x17
  %24 = load double, double* %23, align 8, !llvm.mem.parallel_loop_access !1
  %25 = fmul fast double %24, %19
  store double %25, double* %23, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment18 = add nuw nsw i64 %x17, 1
  %x_postcondition19 = icmp eq i64 %x_increment18, %4
  br i1 %x_postcondition19, label %Flow7, label %x_body15

y_body28:                                         ; preds = %y_body28, %Flow7
  %y30 = phi i64 [ 0, %Flow7 ], [ %y_increment36, %y_body28 ]
  %26 = mul i64 %y30, %22
  %scevgep1 = getelementptr i32, i32* %scevgep, i64 %26
  %scevgep12 = bitcast i32* %scevgep1 to i8*
  %scevgep4 = getelementptr i32, i32* %scevgep3, i64 %26
  %scevgep45 = bitcast i32* %scevgep4 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep12, i8* %scevgep45, i64 %8, i32 8, i1 false)
  %y_increment36 = add nuw nsw i64 %y30, 1
  %y_postcondition37 = icmp eq i64 %y_increment36, %9
  br i1 %y_postcondition37, label %y_body47.preheader, label %y_body28

y_body47.preheader:                               ; preds = %y_body28
  %27 = bitcast %u0Matrix* %21 to double*
  br label %y_body47

y_body47:                                         ; preds = %x_exit51, %y_body47.preheader
  %y49 = phi i64 [ 0, %y_body47.preheader ], [ %y_increment55, %x_exit51 ]
  %28 = mul nuw nsw i64 %y49, %4
  br label %x_body50

x_body50:                                         ; preds = %y_body47, %x_body50
  %x52 = phi i64 [ %x_increment53, %x_body50 ], [ 0, %y_body47 ]
  %29 = add nuw nsw i64 %x52, %28
  %30 = getelementptr double, double* %27, i64 %29
  %31 = load double, double* %30, align 8, !llvm.mem.parallel_loop_access !2
  %32 = getelementptr double, double* %7, i64 %x52
  %33 = load double, double* %32, align 8, !llvm.mem.parallel_loop_access !2
  %34 = fsub fast double %31, %33
  store double %34, double* %30, align 8, !llvm.mem.parallel_loop_access !2
  %x_increment53 = add nuw nsw i64 %x52, 1
  %x_postcondition54 = icmp eq i64 %x_increment53, %4
  br i1 %x_postcondition54, label %x_exit51, label %x_body50

x_exit51:                                         ; preds = %x_body50
  %y_increment55 = add nuw nsw i64 %y49, 1
  %y_postcondition56 = icmp eq i64 %y_increment55, %9
  br i1 %y_postcondition56, label %y_exit48, label %y_body47

y_exit48:                                         ; preds = %x_exit51
  %35 = call %u0Matrix* @likely_new(i32 24896, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %36 = getelementptr inbounds %u0Matrix, %u0Matrix* %35, i64 1
  %37 = bitcast %u0Matrix* %36 to double*
  br label %y_body68

y_body68:                                         ; preds = %x_exit72, %y_exit48
  %y70 = phi i64 [ 0, %y_exit48 ], [ %y_increment80, %x_exit72 ]
  %38 = mul nuw nsw i64 %y70, %4
  br label %x_body71

x_body71:                                         ; preds = %y_body68, %Flow
  %x73 = phi i64 [ %x_increment78, %Flow ], [ 0, %y_body68 ]
  %39 = icmp ugt i64 %y70, %x73
  br i1 %39, label %Flow, label %true_entry76

x_exit72:                                         ; preds = %Flow
  %y_increment80 = add nuw nsw i64 %y70, 1
  %y_postcondition81 = icmp eq i64 %y_increment80, %4
  br i1 %y_postcondition81, label %y_exit69, label %y_body68

y_exit69:                                         ; preds = %x_exit72
  %dst = bitcast %u0Matrix* %35 to %f64Matrix*
  %40 = bitcast %u0Matrix* %2 to i8*
  call void @likely_release_mat(i8* %40)
  %41 = bitcast %u0Matrix* %20 to i8*
  call void @likely_release_mat(i8* %41)
  ret %f64Matrix* %dst

true_entry76:                                     ; preds = %x_body71, %true_entry76
  %42 = phi i32 [ %54, %true_entry76 ], [ 0, %x_body71 ]
  %43 = phi double [ %53, %true_entry76 ], [ 0.000000e+00, %x_body71 ]
  %44 = sext i32 %42 to i64
  %45 = mul nuw nsw i64 %44, %4
  %46 = add nuw nsw i64 %45, %x73
  %47 = getelementptr double, double* %27, i64 %46
  %48 = load double, double* %47, align 8, !llvm.mem.parallel_loop_access !3
  %49 = add nuw nsw i64 %45, %y70
  %50 = getelementptr double, double* %27, i64 %49
  %51 = load double, double* %50, align 8, !llvm.mem.parallel_loop_access !3
  %52 = fmul fast double %51, %48
  %53 = fadd fast double %52, %43
  %54 = add nuw nsw i32 %42, 1
  %55 = icmp eq i32 %54, %rows
  br i1 %55, label %exit77, label %true_entry76

Flow:                                             ; preds = %x_body71, %exit77
  %x_increment78 = add nuw nsw i64 %x73, 1
  %x_postcondition79 = icmp eq i64 %x_increment78, %4
  br i1 %x_postcondition79, label %x_exit72, label %x_body71

exit77:                                           ; preds = %true_entry76
  %56 = add nuw nsw i64 %x73, %38
  %57 = getelementptr double, double* %37, i64 %56
  store double %53, double* %57, align 8, !llvm.mem.parallel_loop_access !3
  %58 = mul nuw nsw i64 %x73, %4
  %59 = add nuw nsw i64 %58, %y70
  %60 = getelementptr double, double* %37, i64 %59
  store double %53, double* %60, align 8, !llvm.mem.parallel_loop_access !3
  br label %Flow
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture readonly, i64, i32, i1) #0

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #0

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { argmemonly nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
