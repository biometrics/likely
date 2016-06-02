; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

define noalias %f32XY* @covariance(%f32XY* nocapture readonly) {
entry:
  %1 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 8480, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %6 = bitcast %u0CXYT* %5 to float*
  %scevgep67 = bitcast %u0CXYT* %5 to i8*
  %7 = shl nuw nsw i64 %4, 2
  call void @llvm.memset.p0i8.i64(i8* %scevgep67, i8 0, i64 %7, i32 4, i1 false)
  %8 = zext i32 %rows to i64
  br label %y_body

y_body:                                           ; preds = %x_exit8, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit8 ]
  %9 = mul nuw nsw i64 %y, %4
  br label %x_body7

x_body7:                                          ; preds = %y_body, %x_body7
  %x9 = phi i64 [ %x_increment10, %x_body7 ], [ 0, %y_body ]
  %10 = getelementptr float, float* %6, i64 %x9
  %11 = load float, float* %10, align 4
  %12 = add nuw nsw i64 %x9, %9
  %13 = getelementptr %f32XY, %f32XY* %0, i64 0, i32 6, i64 %12
  %14 = load float, float* %13, align 4
  %15 = fadd fast float %14, %11
  store float %15, float* %10, align 4
  %x_increment10 = add nuw nsw i64 %x9, 1
  %x_postcondition11 = icmp eq i64 %x_increment10, %4
  br i1 %x_postcondition11, label %x_exit8, label %x_body7

x_exit8:                                          ; preds = %x_body7
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %8
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit8
  %16 = icmp eq i32 %rows, 1
  br i1 %16, label %Flow9, label %true_entry

true_entry:                                       ; preds = %y_exit
  %17 = uitofp i32 %rows to float
  %18 = fdiv fast float 1.000000e+00, %17
  br label %x_body15

Flow9:                                            ; preds = %x_body15, %y_exit
  %19 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %20 = getelementptr inbounds %u0CXYT, %u0CXYT* %19, i64 1
  %21 = bitcast %u0CXYT* %20 to float*
  %scevgep = getelementptr %u0CXYT, %u0CXYT* %19, i64 1, i32 0
  %scevgep3 = getelementptr %f32XY, %f32XY* %0, i64 1, i32 0
  br label %y_body28

x_body15:                                         ; preds = %true_entry, %x_body15
  %x17 = phi i64 [ %x_increment18, %x_body15 ], [ 0, %true_entry ]
  %22 = getelementptr float, float* %6, i64 %x17
  %23 = load float, float* %22, align 4, !llvm.mem.parallel_loop_access !1
  %24 = fmul fast float %23, %18
  store float %24, float* %22, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment18 = add nuw nsw i64 %x17, 1
  %x_postcondition19 = icmp eq i64 %x_increment18, %4
  br i1 %x_postcondition19, label %Flow9, label %x_body15

y_body28:                                         ; preds = %y_body28, %Flow9
  %y30 = phi i64 [ 0, %Flow9 ], [ %y_increment36, %y_body28 ]
  %25 = mul i64 %y30, %4
  %scevgep1 = getelementptr i32, i32* %scevgep, i64 %25
  %scevgep12 = bitcast i32* %scevgep1 to i8*
  %scevgep4 = getelementptr i32, i32* %scevgep3, i64 %25
  %scevgep45 = bitcast i32* %scevgep4 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep12, i8* %scevgep45, i64 %7, i32 4, i1 false)
  %y_increment36 = add nuw nsw i64 %y30, 1
  %y_postcondition37 = icmp eq i64 %y_increment36, %8
  br i1 %y_postcondition37, label %y_body47, label %y_body28

y_body47:                                         ; preds = %y_body28, %x_exit51
  %y49 = phi i64 [ %y_increment55, %x_exit51 ], [ 0, %y_body28 ]
  %26 = mul nuw nsw i64 %y49, %4
  br label %x_body50

x_body50:                                         ; preds = %y_body47, %x_body50
  %x52 = phi i64 [ %x_increment53, %x_body50 ], [ 0, %y_body47 ]
  %27 = add nuw nsw i64 %x52, %26
  %28 = getelementptr float, float* %21, i64 %27
  %29 = load float, float* %28, align 4, !llvm.mem.parallel_loop_access !2
  %30 = getelementptr float, float* %6, i64 %x52
  %31 = load float, float* %30, align 4, !llvm.mem.parallel_loop_access !2
  %32 = fsub fast float %29, %31
  store float %32, float* %28, align 4, !llvm.mem.parallel_loop_access !2
  %x_increment53 = add nuw nsw i64 %x52, 1
  %x_postcondition54 = icmp eq i64 %x_increment53, %4
  br i1 %x_postcondition54, label %x_exit51, label %x_body50

x_exit51:                                         ; preds = %x_body50
  %y_increment55 = add nuw nsw i64 %y49, 1
  %y_postcondition56 = icmp eq i64 %y_increment55, %8
  br i1 %y_postcondition56, label %y_exit48, label %y_body47

y_exit48:                                         ; preds = %x_exit51
  %33 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %34 = getelementptr inbounds %u0CXYT, %u0CXYT* %33, i64 1
  %35 = bitcast %u0CXYT* %34 to float*
  br label %y_body68

y_body68:                                         ; preds = %x_exit72, %y_exit48
  %y70 = phi i64 [ 0, %y_exit48 ], [ %y_increment80, %x_exit72 ]
  %36 = mul nuw nsw i64 %y70, %4
  br label %x_body71

x_body71:                                         ; preds = %y_body68, %Flow
  %x73 = phi i64 [ %x_increment78, %Flow ], [ 0, %y_body68 ]
  %37 = icmp ugt i64 %y70, %x73
  br i1 %37, label %Flow, label %true_entry76

x_exit72:                                         ; preds = %Flow
  %y_increment80 = add nuw nsw i64 %y70, 1
  %y_postcondition81 = icmp eq i64 %y_increment80, %4
  br i1 %y_postcondition81, label %y_exit69, label %y_body68

y_exit69:                                         ; preds = %x_exit72
  %dst = bitcast %u0CXYT* %33 to %f32XY*
  %38 = bitcast %u0CXYT* %2 to i8*
  call void @likely_release_mat(i8* %38)
  %39 = bitcast %u0CXYT* %19 to i8*
  call void @likely_release_mat(i8* %39)
  ret %f32XY* %dst

true_entry76:                                     ; preds = %x_body71, %true_entry76
  %40 = phi i32 [ %54, %true_entry76 ], [ 0, %x_body71 ]
  %41 = phi double [ %53, %true_entry76 ], [ 0.000000e+00, %x_body71 ]
  %42 = sext i32 %40 to i64
  %43 = mul nuw nsw i64 %42, %4
  %44 = add nuw nsw i64 %43, %x73
  %45 = getelementptr float, float* %21, i64 %44
  %46 = load float, float* %45, align 4, !llvm.mem.parallel_loop_access !3
  %47 = fpext float %46 to double
  %48 = add nuw nsw i64 %43, %y70
  %49 = getelementptr float, float* %21, i64 %48
  %50 = load float, float* %49, align 4, !llvm.mem.parallel_loop_access !3
  %51 = fpext float %50 to double
  %52 = fmul fast double %51, %47
  %53 = fadd fast double %52, %41
  %54 = add nuw nsw i32 %40, 1
  %55 = icmp eq i32 %54, %rows
  br i1 %55, label %exit77, label %true_entry76

Flow:                                             ; preds = %x_body71, %exit77
  %x_increment78 = add nuw nsw i64 %x73, 1
  %x_postcondition79 = icmp eq i64 %x_increment78, %4
  br i1 %x_postcondition79, label %x_exit72, label %x_body71

exit77:                                           ; preds = %true_entry76
  %56 = add nuw nsw i64 %x73, %36
  %57 = getelementptr float, float* %35, i64 %56
  %58 = fptrunc double %53 to float
  store float %58, float* %57, align 4, !llvm.mem.parallel_loop_access !3
  %59 = mul nuw nsw i64 %x73, %4
  %60 = add nuw nsw i64 %59, %y70
  %61 = getelementptr float, float* %35, i64 %60
  store float %58, float* %61, align 4, !llvm.mem.parallel_loop_access !3
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
