; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%u16Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

define noalias %f32Matrix* @covariance(%u16Matrix* nocapture readonly) {
entry:
  %1 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0Matrix* @likely_new(i32 9504, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %u16Matrix, %u16Matrix* %0, i64 0, i32 4
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
  %14 = getelementptr %u16Matrix, %u16Matrix* %0, i64 0, i32 6, i64 %13
  %15 = load i16, i16* %14, align 2
  %16 = sitofp i16 %15 to float
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
  %24 = mul nuw nsw i64 %9, %4
  br label %y_body28

x_body15:                                         ; preds = %true_entry, %x_body15
  %x17 = phi i64 [ %x_increment18, %x_body15 ], [ 0, %true_entry ]
  %25 = getelementptr float, float* %7, i64 %x17
  %26 = load float, float* %25, align 4, !llvm.mem.parallel_loop_access !1
  %27 = fmul fast float %26, %20
  store float %27, float* %25, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment18 = add nuw nsw i64 %x17, 1
  %x_postcondition19 = icmp eq i64 %x_increment18, %4
  br i1 %x_postcondition19, label %Flow2, label %x_body15

y_body28:                                         ; preds = %y_body28, %Flow2
  %y30 = phi i64 [ 0, %Flow2 ], [ %y_increment36, %y_body28 ]
  %28 = getelementptr %u16Matrix, %u16Matrix* %0, i64 0, i32 6, i64 %y30
  %29 = load i16, i16* %28, align 2, !llvm.mem.parallel_loop_access !2
  %30 = getelementptr float, float* %23, i64 %y30
  %31 = sitofp i16 %29 to float
  store float %31, float* %30, align 4, !llvm.mem.parallel_loop_access !2
  %y_increment36 = add nuw nsw i64 %y30, 1
  %y_postcondition37 = icmp eq i64 %y_increment36, %24
  br i1 %y_postcondition37, label %y_body47, label %y_body28

y_body47:                                         ; preds = %y_body28, %x_exit51
  %y49 = phi i64 [ %y_increment55, %x_exit51 ], [ 0, %y_body28 ]
  %32 = mul nuw nsw i64 %y49, %4
  br label %x_body50

x_body50:                                         ; preds = %y_body47, %x_body50
  %x52 = phi i64 [ %x_increment53, %x_body50 ], [ 0, %y_body47 ]
  %33 = add nuw nsw i64 %x52, %32
  %34 = getelementptr float, float* %23, i64 %33
  %35 = load float, float* %34, align 4, !llvm.mem.parallel_loop_access !3
  %36 = getelementptr float, float* %7, i64 %x52
  %37 = load float, float* %36, align 4, !llvm.mem.parallel_loop_access !3
  %38 = fsub fast float %35, %37
  store float %38, float* %34, align 4, !llvm.mem.parallel_loop_access !3
  %x_increment53 = add nuw nsw i64 %x52, 1
  %x_postcondition54 = icmp eq i64 %x_increment53, %4
  br i1 %x_postcondition54, label %x_exit51, label %x_body50

x_exit51:                                         ; preds = %x_body50
  %y_increment55 = add nuw nsw i64 %y49, 1
  %y_postcondition56 = icmp eq i64 %y_increment55, %9
  br i1 %y_postcondition56, label %y_exit48, label %y_body47

y_exit48:                                         ; preds = %x_exit51
  %39 = call %u0Matrix* @likely_new(i32 25888, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %40 = getelementptr inbounds %u0Matrix, %u0Matrix* %39, i64 1
  %41 = bitcast %u0Matrix* %40 to float*
  br label %y_body68

y_body68:                                         ; preds = %x_exit72, %y_exit48
  %y70 = phi i64 [ 0, %y_exit48 ], [ %y_increment80, %x_exit72 ]
  %42 = mul nuw nsw i64 %y70, %4
  br label %x_body71

x_body71:                                         ; preds = %y_body68, %Flow
  %x73 = phi i64 [ %x_increment78, %Flow ], [ 0, %y_body68 ]
  %43 = icmp ugt i64 %y70, %x73
  br i1 %43, label %Flow, label %true_entry76

x_exit72:                                         ; preds = %Flow
  %y_increment80 = add nuw nsw i64 %y70, 1
  %y_postcondition81 = icmp eq i64 %y_increment80, %4
  br i1 %y_postcondition81, label %y_exit69, label %y_body68

y_exit69:                                         ; preds = %x_exit72
  %dst = bitcast %u0Matrix* %39 to %f32Matrix*
  %44 = bitcast %u0Matrix* %2 to i8*
  call void @likely_release_mat(i8* %44)
  %45 = bitcast %u0Matrix* %21 to i8*
  call void @likely_release_mat(i8* %45)
  ret %f32Matrix* %dst

true_entry76:                                     ; preds = %x_body71, %true_entry76
  %46 = phi i32 [ %60, %true_entry76 ], [ 0, %x_body71 ]
  %47 = phi double [ %59, %true_entry76 ], [ 0.000000e+00, %x_body71 ]
  %48 = sext i32 %46 to i64
  %49 = mul nuw nsw i64 %48, %4
  %50 = add nuw nsw i64 %49, %x73
  %51 = getelementptr float, float* %23, i64 %50
  %52 = load float, float* %51, align 4, !llvm.mem.parallel_loop_access !4
  %53 = fpext float %52 to double
  %54 = add nuw nsw i64 %49, %y70
  %55 = getelementptr float, float* %23, i64 %54
  %56 = load float, float* %55, align 4, !llvm.mem.parallel_loop_access !4
  %57 = fpext float %56 to double
  %58 = fmul fast double %57, %53
  %59 = fadd fast double %58, %47
  %60 = add nuw nsw i32 %46, 1
  %61 = icmp eq i32 %60, %rows
  br i1 %61, label %exit77, label %true_entry76

Flow:                                             ; preds = %x_body71, %exit77
  %x_increment78 = add nuw nsw i64 %x73, 1
  %x_postcondition79 = icmp eq i64 %x_increment78, %4
  br i1 %x_postcondition79, label %x_exit72, label %x_body71

exit77:                                           ; preds = %true_entry76
  %62 = add nuw nsw i64 %x73, %42
  %63 = getelementptr float, float* %41, i64 %62
  %64 = fptrunc double %59 to float
  store float %64, float* %63, align 4, !llvm.mem.parallel_loop_access !4
  %65 = mul nuw nsw i64 %x73, %4
  %66 = add nuw nsw i64 %65, %y70
  %67 = getelementptr float, float* %41, i64 %66
  store float %64, float* %67, align 4, !llvm.mem.parallel_loop_access !4
  br label %Flow
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #0

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { argmemonly nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
!4 = distinct !{!4}
