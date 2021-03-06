; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%u8Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define noalias %f32Matrix* @covariance(%u8Matrix* noalias nocapture readonly) #1 {
entry:
  %1 = getelementptr inbounds %u8Matrix, %u8Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0Matrix* @likely_new(i32 9504, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %u8Matrix, %u8Matrix* %0, i64 0, i32 4
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
  %14 = getelementptr %u8Matrix, %u8Matrix* %0, i64 0, i32 6, i64 %13
  %15 = load i8, i8* %14, align 1
  %16 = uitofp i8 %15 to float
  %17 = fadd fast float %16, %12
  store float %17, float* %11, align 4
  %x_increment10 = add nuw nsw i64 %x9, 1
  %x_postcondition11 = icmp eq i64 %x_increment10, %4
  br i1 %x_postcondition11, label %x_exit8, label %x_body7

x_exit8:                                          ; preds = %x_body7
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %9
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit8
  %18 = icmp eq i32 %rows, 1
  br i1 %18, label %Flow2, label %true_entry

true_entry:                                       ; preds = %y_exit
  %19 = uitofp i32 %rows to float
  %20 = fdiv fast float 1.000000e+00, %19
  br label %x_body15

Flow2:                                            ; preds = %x_body15, %y_exit
  %21 = call %u0Matrix* @likely_new(i32 25888, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %22 = getelementptr inbounds %u0Matrix, %u0Matrix* %21, i64 1
  %23 = bitcast %u0Matrix* %22 to float*
  br label %y_body28

x_body15:                                         ; preds = %true_entry, %x_body15
  %x17 = phi i64 [ %x_increment18, %x_body15 ], [ 0, %true_entry ]
  %24 = getelementptr float, float* %7, i64 %x17
  %25 = load float, float* %24, align 4, !llvm.mem.parallel_loop_access !1
  %26 = fmul fast float %25, %20
  store float %26, float* %24, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment18 = add nuw nsw i64 %x17, 1
  %x_postcondition19 = icmp eq i64 %x_increment18, %4
  br i1 %x_postcondition19, label %Flow2, label %x_body15

y_body28:                                         ; preds = %x_exit32, %Flow2
  %y30 = phi i64 [ 0, %Flow2 ], [ %y_increment36, %x_exit32 ]
  %27 = mul nuw nsw i64 %y30, %4
  br label %x_body31

x_body31:                                         ; preds = %y_body28, %x_body31
  %x33 = phi i64 [ %x_increment34, %x_body31 ], [ 0, %y_body28 ]
  %28 = add nuw nsw i64 %x33, %27
  %29 = getelementptr %u8Matrix, %u8Matrix* %0, i64 0, i32 6, i64 %28
  %30 = load i8, i8* %29, align 1, !llvm.mem.parallel_loop_access !2
  %31 = getelementptr float, float* %23, i64 %28
  %32 = uitofp i8 %30 to float
  store float %32, float* %31, align 4, !llvm.mem.parallel_loop_access !2
  %x_increment34 = add nuw nsw i64 %x33, 1
  %x_postcondition35 = icmp eq i64 %x_increment34, %4
  br i1 %x_postcondition35, label %x_exit32, label %x_body31

x_exit32:                                         ; preds = %x_body31
  %y_increment36 = add nuw nsw i64 %y30, 1
  %y_postcondition37 = icmp eq i64 %y_increment36, %9
  br i1 %y_postcondition37, label %y_body47, label %y_body28

y_body47:                                         ; preds = %x_exit32, %x_exit51
  %y49 = phi i64 [ %y_increment55, %x_exit51 ], [ 0, %x_exit32 ]
  %33 = mul nuw nsw i64 %y49, %4
  br label %x_body50

x_body50:                                         ; preds = %y_body47, %x_body50
  %x52 = phi i64 [ %x_increment53, %x_body50 ], [ 0, %y_body47 ]
  %34 = add nuw nsw i64 %x52, %33
  %35 = getelementptr float, float* %23, i64 %34
  %36 = load float, float* %35, align 4, !llvm.mem.parallel_loop_access !3
  %37 = getelementptr float, float* %7, i64 %x52
  %38 = load float, float* %37, align 4, !llvm.mem.parallel_loop_access !3
  %39 = fsub fast float %36, %38
  store float %39, float* %35, align 4, !llvm.mem.parallel_loop_access !3
  %x_increment53 = add nuw nsw i64 %x52, 1
  %x_postcondition54 = icmp eq i64 %x_increment53, %4
  br i1 %x_postcondition54, label %x_exit51, label %x_body50

x_exit51:                                         ; preds = %x_body50
  %y_increment55 = add nuw nsw i64 %y49, 1
  %y_postcondition56 = icmp eq i64 %y_increment55, %9
  br i1 %y_postcondition56, label %y_exit48, label %y_body47

y_exit48:                                         ; preds = %x_exit51
  %40 = call %u0Matrix* @likely_new(i32 25888, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %41 = getelementptr inbounds %u0Matrix, %u0Matrix* %40, i64 1
  %42 = bitcast %u0Matrix* %41 to float*
  br label %y_body68

y_body68:                                         ; preds = %x_exit72, %y_exit48
  %y70 = phi i64 [ 0, %y_exit48 ], [ %y_increment80, %x_exit72 ]
  %43 = mul nuw nsw i64 %y70, %4
  br label %x_body71

x_body71:                                         ; preds = %y_body68, %exit75
  %x73 = phi i64 [ %x_increment78, %exit75 ], [ 0, %y_body68 ]
  %44 = icmp ugt i64 %y70, %x73
  br i1 %44, label %exit75, label %true_entry76

true_entry76:                                     ; preds = %x_body71, %true_entry76
  %45 = phi i32 [ %59, %true_entry76 ], [ 0, %x_body71 ]
  %46 = phi double [ %58, %true_entry76 ], [ 0.000000e+00, %x_body71 ]
  %47 = zext i32 %45 to i64
  %48 = mul nuw nsw i64 %47, %4
  %49 = add nuw nsw i64 %48, %x73
  %50 = getelementptr float, float* %23, i64 %49
  %51 = load float, float* %50, align 4, !llvm.mem.parallel_loop_access !4
  %52 = fpext float %51 to double
  %53 = add nuw nsw i64 %48, %y70
  %54 = getelementptr float, float* %23, i64 %53
  %55 = load float, float* %54, align 4, !llvm.mem.parallel_loop_access !4
  %56 = fpext float %55 to double
  %57 = fmul fast double %56, %52
  %58 = fadd fast double %57, %46
  %59 = add nuw nsw i32 %45, 1
  %60 = icmp eq i32 %59, %rows
  br i1 %60, label %exit77, label %true_entry76

exit77:                                           ; preds = %true_entry76
  %61 = add nuw nsw i64 %x73, %43
  %62 = getelementptr float, float* %42, i64 %61
  %63 = fptrunc double %58 to float
  store float %63, float* %62, align 4, !llvm.mem.parallel_loop_access !4
  %64 = mul nuw nsw i64 %x73, %4
  %65 = add nuw nsw i64 %64, %y70
  %66 = getelementptr float, float* %42, i64 %65
  store float %63, float* %66, align 4, !llvm.mem.parallel_loop_access !4
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
  %dst = bitcast %u0Matrix* %40 to %f32Matrix*
  %67 = bitcast %u0Matrix* %2 to i8*
  call void @likely_release_mat(i8* %67) #1
  %68 = bitcast %u0Matrix* %21 to i8*
  call void @likely_release_mat(i8* %68) #1
  ret %f32Matrix* %dst
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #0

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
!4 = distinct !{!4}
