; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define noalias %f32Matrix* @covariance(%f32Matrix* noalias nocapture readonly) #1 {
entry:
  %1 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0Matrix* @likely_new(i32 8480, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %f32Matrix, %f32Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0Matrix, %u0Matrix* %2, i64 1
  %6 = bitcast %u0Matrix* %5 to i8*
  %7 = bitcast %u0Matrix* %5 to float*
  %8 = shl nuw nsw i64 %4, 2
  call void @llvm.memset.p0i8.i64(i8* %6, i8 0, i64 %8, i32 4, i1 false)
  %9 = zext i32 %rows to i64
  br label %y_body

y_body:                                           ; preds = %x_exit8, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit8 ]
  %10 = mul nuw nsw i64 %y, %4
  br label %x_body7

x_body7:                                          ; preds = %y_body, %x_body7
  %x9 = phi i64 [ %x_increment10, %x_body7 ], [ 0, %y_body ]
  %11 = getelementptr float, float* %7, i64 %x9
  %12 = load float, float* %11, align 4
  %13 = add nuw nsw i64 %x9, %10
  %14 = getelementptr %f32Matrix, %f32Matrix* %0, i64 0, i32 6, i64 %13
  %15 = load float, float* %14, align 4
  %16 = fadd fast float %15, %12
  store float %16, float* %11, align 4
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
  %18 = uitofp i32 %rows to float
  %19 = fdiv fast float 1.000000e+00, %18
  br label %x_body15

Flow7:                                            ; preds = %x_body15, %y_exit
  %20 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %21 = getelementptr inbounds %u0Matrix, %u0Matrix* %20, i64 1
  %scevgep = getelementptr %u0Matrix, %u0Matrix* %20, i64 1, i32 0
  %scevgep3 = getelementptr %f32Matrix, %f32Matrix* %0, i64 1, i32 0
  br label %y_body28

x_body15:                                         ; preds = %true_entry, %x_body15
  %x17 = phi i64 [ %x_increment18, %x_body15 ], [ 0, %true_entry ]
  %22 = getelementptr float, float* %7, i64 %x17
  %23 = load float, float* %22, align 4, !llvm.mem.parallel_loop_access !1
  %24 = fmul fast float %23, %19
  store float %24, float* %22, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment18 = add nuw nsw i64 %x17, 1
  %x_postcondition19 = icmp eq i64 %x_increment18, %4
  br i1 %x_postcondition19, label %Flow7, label %x_body15

y_body28:                                         ; preds = %y_body28, %Flow7
  %y30 = phi i64 [ 0, %Flow7 ], [ %y_increment36, %y_body28 ]
  %25 = mul i64 %y30, %4
  %scevgep1 = getelementptr i32, i32* %scevgep, i64 %25
  %scevgep12 = bitcast i32* %scevgep1 to i8*
  %scevgep4 = getelementptr i32, i32* %scevgep3, i64 %25
  %scevgep45 = bitcast i32* %scevgep4 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep12, i8* %scevgep45, i64 %8, i32 4, i1 false)
  %y_increment36 = add nuw nsw i64 %y30, 1
  %y_postcondition37 = icmp eq i64 %y_increment36, %9
  br i1 %y_postcondition37, label %y_body47.preheader, label %y_body28

y_body47.preheader:                               ; preds = %y_body28
  %26 = bitcast %u0Matrix* %21 to float*
  br label %y_body47

y_body47:                                         ; preds = %x_exit51, %y_body47.preheader
  %y49 = phi i64 [ 0, %y_body47.preheader ], [ %y_increment55, %x_exit51 ]
  %27 = mul nuw nsw i64 %y49, %4
  br label %x_body50

x_body50:                                         ; preds = %y_body47, %x_body50
  %x52 = phi i64 [ %x_increment53, %x_body50 ], [ 0, %y_body47 ]
  %28 = add nuw nsw i64 %x52, %27
  %29 = getelementptr float, float* %26, i64 %28
  %30 = load float, float* %29, align 4, !llvm.mem.parallel_loop_access !2
  %31 = getelementptr float, float* %7, i64 %x52
  %32 = load float, float* %31, align 4, !llvm.mem.parallel_loop_access !2
  %33 = fsub fast float %30, %32
  store float %33, float* %29, align 4, !llvm.mem.parallel_loop_access !2
  %x_increment53 = add nuw nsw i64 %x52, 1
  %x_postcondition54 = icmp eq i64 %x_increment53, %4
  br i1 %x_postcondition54, label %x_exit51, label %x_body50

x_exit51:                                         ; preds = %x_body50
  %y_increment55 = add nuw nsw i64 %y49, 1
  %y_postcondition56 = icmp eq i64 %y_increment55, %9
  br i1 %y_postcondition56, label %y_exit48, label %y_body47

y_exit48:                                         ; preds = %x_exit51
  %34 = call %u0Matrix* @likely_new(i32 24864, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %35 = getelementptr inbounds %u0Matrix, %u0Matrix* %34, i64 1
  %36 = bitcast %u0Matrix* %35 to float*
  br label %y_body68

y_body68:                                         ; preds = %x_exit72, %y_exit48
  %y70 = phi i64 [ 0, %y_exit48 ], [ %y_increment80, %x_exit72 ]
  %37 = mul nuw nsw i64 %y70, %4
  br label %x_body71

x_body71:                                         ; preds = %y_body68, %exit75
  %x73 = phi i64 [ %x_increment78, %exit75 ], [ 0, %y_body68 ]
  %38 = icmp ugt i64 %y70, %x73
  br i1 %38, label %exit75, label %true_entry76

true_entry76:                                     ; preds = %x_body71, %true_entry76
  %39 = phi i32 [ %53, %true_entry76 ], [ 0, %x_body71 ]
  %40 = phi double [ %52, %true_entry76 ], [ 0.000000e+00, %x_body71 ]
  %41 = zext i32 %39 to i64
  %42 = mul nuw nsw i64 %41, %4
  %43 = add nuw nsw i64 %42, %x73
  %44 = getelementptr float, float* %26, i64 %43
  %45 = load float, float* %44, align 4, !llvm.mem.parallel_loop_access !3
  %46 = fpext float %45 to double
  %47 = add nuw nsw i64 %42, %y70
  %48 = getelementptr float, float* %26, i64 %47
  %49 = load float, float* %48, align 4, !llvm.mem.parallel_loop_access !3
  %50 = fpext float %49 to double
  %51 = fmul fast double %50, %46
  %52 = fadd fast double %51, %40
  %53 = add nuw nsw i32 %39, 1
  %54 = icmp eq i32 %53, %rows
  br i1 %54, label %exit77, label %true_entry76

exit77:                                           ; preds = %true_entry76
  %55 = add nuw nsw i64 %x73, %37
  %56 = getelementptr float, float* %36, i64 %55
  %57 = fptrunc double %52 to float
  store float %57, float* %56, align 4, !llvm.mem.parallel_loop_access !3
  %58 = mul nuw nsw i64 %x73, %4
  %59 = add nuw nsw i64 %58, %y70
  %60 = getelementptr float, float* %36, i64 %59
  store float %57, float* %60, align 4, !llvm.mem.parallel_loop_access !3
  br label %exit75

exit75:                                           ; preds = %exit77, %x_body71
  %x_increment78 = add nuw nsw i64 %x73, 1
  %x_postcondition79 = icmp eq i64 %x_increment78, %4
  br i1 %x_postcondition79, label %x_exit72, label %x_body71

x_exit72:                                         ; preds = %exit75
  %y_increment80 = add nuw nsw i64 %y70, 1
  %y_postcondition81 = icmp eq i64 %y_increment80, %4
  br i1 %y_postcondition81, label %y_exit69, label %y_body68

y_exit69:                                         ; preds = %x_exit72
  %dst = bitcast %u0Matrix* %34 to %f32Matrix*
  %61 = bitcast %u0Matrix* %2 to i8*
  call void @likely_release_mat(i8* %61) #1
  %62 = bitcast %u0Matrix* %20 to i8*
  call void @likely_release_mat(i8* %62) #1
  ret %f32Matrix* %dst
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture readonly, i64, i32, i1) #0

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #0

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
