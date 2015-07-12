; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32SXY = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%u8SXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f32SXY* @covariance(%u8SXY*) {
entry:
  %1 = getelementptr inbounds %u8SXY, %u8SXY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 9504, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %u8SXY, %u8SXY* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %6 = bitcast %u0CXYT* %5 to float*
  %scevgep1 = bitcast %u0CXYT* %5 to i8*
  %7 = shl nuw nsw i64 %4, 2
  call void @llvm.memset.p0i8.i64(i8* %scevgep1, i8 0, i64 %7, i32 4, i1 false)
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
  %13 = getelementptr %u8SXY, %u8SXY* %0, i64 0, i32 6, i64 %12
  %14 = load i8, i8* %13, align 1
  %15 = uitofp i8 %14 to float
  %16 = fadd fast float %15, %11
  store float %16, float* %10, align 4
  %x_increment10 = add nuw nsw i64 %x9, 1
  %x_postcondition11 = icmp eq i64 %x_increment10, %4
  br i1 %x_postcondition11, label %x_exit8, label %x_body7

x_exit8:                                          ; preds = %x_body7
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %8
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit8
  %17 = icmp eq i32 %rows, 1
  br i1 %17, label %Flow3, label %true_entry

true_entry:                                       ; preds = %y_exit
  %18 = uitofp i32 %rows to float
  %19 = fdiv fast float 1.000000e+00, %18
  br label %x_body15

Flow3:                                            ; preds = %x_body15, %y_exit
  %20 = call %u0CXYT* @likely_new(i32 25888, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %21 = getelementptr inbounds %u0CXYT, %u0CXYT* %20, i64 1
  %22 = bitcast %u0CXYT* %21 to float*
  %23 = mul nuw nsw i64 %8, %4
  br label %y_body28

x_body15:                                         ; preds = %true_entry, %x_body15
  %x17 = phi i64 [ %x_increment18, %x_body15 ], [ 0, %true_entry ]
  %24 = getelementptr float, float* %6, i64 %x17
  %25 = load float, float* %24, align 4, !llvm.mem.parallel_loop_access !1
  %26 = fmul fast float %25, %19
  store float %26, float* %24, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment18 = add nuw nsw i64 %x17, 1
  %x_postcondition19 = icmp eq i64 %x_increment18, %4
  br i1 %x_postcondition19, label %Flow3, label %x_body15

y_body28:                                         ; preds = %y_body28, %Flow3
  %y30 = phi i64 [ 0, %Flow3 ], [ %y_increment36, %y_body28 ]
  %27 = getelementptr %u8SXY, %u8SXY* %0, i64 0, i32 6, i64 %y30
  %28 = load i8, i8* %27, align 1, !llvm.mem.parallel_loop_access !2
  %29 = getelementptr float, float* %22, i64 %y30
  %30 = uitofp i8 %28 to float
  store float %30, float* %29, align 4, !llvm.mem.parallel_loop_access !2
  %y_increment36 = add nuw nsw i64 %y30, 1
  %y_postcondition37 = icmp eq i64 %y_increment36, %23
  br i1 %y_postcondition37, label %y_body47, label %y_body28

y_body47:                                         ; preds = %y_body28, %x_exit51
  %y49 = phi i64 [ %y_increment55, %x_exit51 ], [ 0, %y_body28 ]
  %31 = mul nuw nsw i64 %y49, %4
  br label %x_body50

x_body50:                                         ; preds = %y_body47, %x_body50
  %x52 = phi i64 [ %x_increment53, %x_body50 ], [ 0, %y_body47 ]
  %32 = add nuw nsw i64 %x52, %31
  %33 = getelementptr float, float* %22, i64 %32
  %34 = load float, float* %33, align 4, !llvm.mem.parallel_loop_access !3
  %35 = getelementptr float, float* %6, i64 %x52
  %36 = load float, float* %35, align 4, !llvm.mem.parallel_loop_access !3
  %37 = fsub fast float %34, %36
  store float %37, float* %33, align 4, !llvm.mem.parallel_loop_access !3
  %x_increment53 = add nuw nsw i64 %x52, 1
  %x_postcondition54 = icmp eq i64 %x_increment53, %4
  br i1 %x_postcondition54, label %x_exit51, label %x_body50

x_exit51:                                         ; preds = %x_body50
  %y_increment55 = add nuw nsw i64 %y49, 1
  %y_postcondition56 = icmp eq i64 %y_increment55, %8
  br i1 %y_postcondition56, label %y_exit48, label %y_body47

y_exit48:                                         ; preds = %x_exit51
  %38 = call %u0CXYT* @likely_new(i32 25888, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %39 = getelementptr inbounds %u0CXYT, %u0CXYT* %38, i64 1
  %40 = bitcast %u0CXYT* %39 to float*
  %41 = ptrtoint %u0CXYT* %39 to i64
  %42 = and i64 %41, 31
  %43 = icmp eq i64 %42, 0
  call void @llvm.assume(i1 %43)
  br label %y_body68

y_body68:                                         ; preds = %x_exit72, %y_exit48
  %y70 = phi i64 [ 0, %y_exit48 ], [ %y_increment80, %x_exit72 ]
  %44 = mul nuw nsw i64 %y70, %4
  br label %x_body71

x_body71:                                         ; preds = %y_body68, %Flow
  %x73 = phi i64 [ %x_increment78, %Flow ], [ 0, %y_body68 ]
  %45 = icmp ugt i64 %y70, %x73
  br i1 %45, label %Flow, label %true_entry76

x_exit72:                                         ; preds = %Flow
  %y_increment80 = add nuw nsw i64 %y70, 1
  %y_postcondition81 = icmp eq i64 %y_increment80, %4
  br i1 %y_postcondition81, label %y_exit69, label %y_body68

y_exit69:                                         ; preds = %x_exit72
  %dst = bitcast %u0CXYT* %38 to %f32SXY*
  %46 = bitcast %u0CXYT* %2 to i8*
  call void @likely_release_mat(i8* %46)
  %47 = bitcast %u0CXYT* %20 to i8*
  call void @likely_release_mat(i8* %47)
  ret %f32SXY* %dst

true_entry76:                                     ; preds = %x_body71, %true_entry76
  %48 = phi i32 [ %62, %true_entry76 ], [ 0, %x_body71 ]
  %49 = phi double [ %61, %true_entry76 ], [ 0.000000e+00, %x_body71 ]
  %50 = sext i32 %48 to i64
  %51 = mul nuw nsw i64 %50, %4
  %52 = add nuw nsw i64 %51, %x73
  %53 = getelementptr float, float* %22, i64 %52
  %54 = load float, float* %53, align 4, !llvm.mem.parallel_loop_access !4
  %55 = fpext float %54 to double
  %56 = add nuw nsw i64 %51, %y70
  %57 = getelementptr float, float* %22, i64 %56
  %58 = load float, float* %57, align 4, !llvm.mem.parallel_loop_access !4
  %59 = fpext float %58 to double
  %60 = fmul fast double %59, %55
  %61 = fadd fast double %60, %49
  %62 = add nuw nsw i32 %48, 1
  %63 = icmp eq i32 %62, %rows
  br i1 %63, label %exit77, label %true_entry76

Flow:                                             ; preds = %x_body71, %exit77
  %x_increment78 = add nuw nsw i64 %x73, 1
  %x_postcondition79 = icmp eq i64 %x_increment78, %4
  br i1 %x_postcondition79, label %x_exit72, label %x_body71

exit77:                                           ; preds = %true_entry76
  %64 = add nuw nsw i64 %x73, %44
  %65 = getelementptr float, float* %40, i64 %64
  %66 = fptrunc double %61 to float
  store float %66, float* %65, align 4, !llvm.mem.parallel_loop_access !4
  %67 = mul nuw nsw i64 %x73, %4
  %68 = add nuw nsw i64 %67, %y70
  %69 = getelementptr float, float* %40, i64 %68
  store float %66, float* %69, align 4, !llvm.mem.parallel_loop_access !4
  br label %Flow
}

; Function Attrs: nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #1

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
!4 = distinct !{!4}
