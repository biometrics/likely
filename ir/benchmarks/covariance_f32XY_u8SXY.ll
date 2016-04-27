; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32SXY = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%u8SXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: nounwind argmemonly
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
  %7 = ptrtoint %u0CXYT* %5 to i64
  %8 = and i64 %7, 31
  %9 = icmp eq i64 %8, 0
  call void @llvm.assume(i1 %9)
  %scevgep1 = bitcast %u0CXYT* %5 to i8*
  %10 = shl nuw nsw i64 %4, 2
  call void @llvm.memset.p0i8.i64(i8* %scevgep1, i8 0, i64 %10, i32 32, i1 false)
  %11 = zext i32 %rows to i64
  %12 = getelementptr inbounds %u8SXY, %u8SXY* %0, i64 0, i32 6, i64 0
  %13 = ptrtoint i8* %12 to i64
  %14 = and i64 %13, 31
  %15 = icmp eq i64 %14, 0
  call void @llvm.assume(i1 %15)
  br label %y_body

y_body:                                           ; preds = %x_exit8, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit8 ]
  %16 = mul nuw nsw i64 %y, %4
  br label %x_body7

x_body7:                                          ; preds = %y_body, %x_body7
  %x9 = phi i64 [ %x_increment10, %x_body7 ], [ 0, %y_body ]
  %17 = getelementptr float, float* %6, i64 %x9
  %18 = load float, float* %17, align 4
  %19 = add nuw nsw i64 %x9, %16
  %20 = getelementptr %u8SXY, %u8SXY* %0, i64 0, i32 6, i64 %19
  %21 = load i8, i8* %20, align 1
  %22 = uitofp i8 %21 to float
  %23 = fadd fast float %22, %18
  store float %23, float* %17, align 4
  %x_increment10 = add nuw nsw i64 %x9, 1
  %x_postcondition11 = icmp eq i64 %x_increment10, %4
  br i1 %x_postcondition11, label %x_exit8, label %x_body7

x_exit8:                                          ; preds = %x_body7
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %11
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit8
  %24 = icmp eq i32 %rows, 1
  br i1 %24, label %Flow3, label %true_entry

true_entry:                                       ; preds = %y_exit
  %25 = uitofp i32 %rows to float
  %26 = fdiv fast float 1.000000e+00, %25
  br label %x_body15

Flow3:                                            ; preds = %x_body15, %y_exit
  %27 = call %u0CXYT* @likely_new(i32 25888, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %28 = getelementptr inbounds %u0CXYT, %u0CXYT* %27, i64 1
  %29 = bitcast %u0CXYT* %28 to float*
  %30 = ptrtoint %u0CXYT* %28 to i64
  %31 = and i64 %30, 31
  %32 = icmp eq i64 %31, 0
  call void @llvm.assume(i1 %32)
  %33 = mul nuw nsw i64 %11, %4
  br label %y_body28

x_body15:                                         ; preds = %true_entry, %x_body15
  %x17 = phi i64 [ %x_increment18, %x_body15 ], [ 0, %true_entry ]
  %34 = getelementptr float, float* %6, i64 %x17
  %35 = load float, float* %34, align 4, !llvm.mem.parallel_loop_access !1
  %36 = fmul fast float %35, %26
  store float %36, float* %34, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment18 = add nuw nsw i64 %x17, 1
  %x_postcondition19 = icmp eq i64 %x_increment18, %4
  br i1 %x_postcondition19, label %Flow3, label %x_body15

y_body28:                                         ; preds = %y_body28, %Flow3
  %y30 = phi i64 [ 0, %Flow3 ], [ %y_increment36, %y_body28 ]
  %37 = getelementptr %u8SXY, %u8SXY* %0, i64 0, i32 6, i64 %y30
  %38 = load i8, i8* %37, align 1, !llvm.mem.parallel_loop_access !2
  %39 = getelementptr float, float* %29, i64 %y30
  %40 = uitofp i8 %38 to float
  store float %40, float* %39, align 4, !llvm.mem.parallel_loop_access !2
  %y_increment36 = add nuw nsw i64 %y30, 1
  %y_postcondition37 = icmp eq i64 %y_increment36, %33
  br i1 %y_postcondition37, label %y_body47, label %y_body28

y_body47:                                         ; preds = %y_body28, %x_exit51
  %y49 = phi i64 [ %y_increment55, %x_exit51 ], [ 0, %y_body28 ]
  %41 = mul nuw nsw i64 %y49, %4
  br label %x_body50

x_body50:                                         ; preds = %y_body47, %x_body50
  %x52 = phi i64 [ %x_increment53, %x_body50 ], [ 0, %y_body47 ]
  %42 = add nuw nsw i64 %x52, %41
  %43 = getelementptr float, float* %29, i64 %42
  %44 = load float, float* %43, align 4, !llvm.mem.parallel_loop_access !3
  %45 = getelementptr float, float* %6, i64 %x52
  %46 = load float, float* %45, align 4, !llvm.mem.parallel_loop_access !3
  %47 = fsub fast float %44, %46
  store float %47, float* %43, align 4, !llvm.mem.parallel_loop_access !3
  %x_increment53 = add nuw nsw i64 %x52, 1
  %x_postcondition54 = icmp eq i64 %x_increment53, %4
  br i1 %x_postcondition54, label %x_exit51, label %x_body50

x_exit51:                                         ; preds = %x_body50
  %y_increment55 = add nuw nsw i64 %y49, 1
  %y_postcondition56 = icmp eq i64 %y_increment55, %11
  br i1 %y_postcondition56, label %y_exit48, label %y_body47

y_exit48:                                         ; preds = %x_exit51
  %48 = call %u0CXYT* @likely_new(i32 25888, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %49 = getelementptr inbounds %u0CXYT, %u0CXYT* %48, i64 1
  %50 = bitcast %u0CXYT* %49 to float*
  %51 = ptrtoint %u0CXYT* %49 to i64
  %52 = and i64 %51, 31
  %53 = icmp eq i64 %52, 0
  call void @llvm.assume(i1 %53)
  br label %y_body68

y_body68:                                         ; preds = %x_exit72, %y_exit48
  %y70 = phi i64 [ 0, %y_exit48 ], [ %y_increment80, %x_exit72 ]
  %54 = mul nuw nsw i64 %y70, %4
  br label %x_body71

x_body71:                                         ; preds = %y_body68, %Flow
  %x73 = phi i64 [ %x_increment78, %Flow ], [ 0, %y_body68 ]
  %55 = icmp ugt i64 %y70, %x73
  br i1 %55, label %Flow, label %true_entry76

x_exit72:                                         ; preds = %Flow
  %y_increment80 = add nuw nsw i64 %y70, 1
  %y_postcondition81 = icmp eq i64 %y_increment80, %4
  br i1 %y_postcondition81, label %y_exit69, label %y_body68

y_exit69:                                         ; preds = %x_exit72
  %dst = bitcast %u0CXYT* %48 to %f32SXY*
  %56 = bitcast %u0CXYT* %2 to i8*
  call void @likely_release_mat(i8* %56)
  %57 = bitcast %u0CXYT* %27 to i8*
  call void @likely_release_mat(i8* %57)
  ret %f32SXY* %dst

true_entry76:                                     ; preds = %x_body71, %true_entry76
  %58 = phi i32 [ %72, %true_entry76 ], [ 0, %x_body71 ]
  %59 = phi double [ %71, %true_entry76 ], [ 0.000000e+00, %x_body71 ]
  %60 = sext i32 %58 to i64
  %61 = mul nuw nsw i64 %60, %4
  %62 = add nuw nsw i64 %61, %x73
  %63 = getelementptr float, float* %29, i64 %62
  %64 = load float, float* %63, align 4, !llvm.mem.parallel_loop_access !4
  %65 = fpext float %64 to double
  %66 = add nuw nsw i64 %61, %y70
  %67 = getelementptr float, float* %29, i64 %66
  %68 = load float, float* %67, align 4, !llvm.mem.parallel_loop_access !4
  %69 = fpext float %68 to double
  %70 = fmul fast double %69, %65
  %71 = fadd fast double %70, %59
  %72 = add nuw nsw i32 %58, 1
  %73 = icmp eq i32 %72, %rows
  br i1 %73, label %exit77, label %true_entry76

Flow:                                             ; preds = %x_body71, %exit77
  %x_increment78 = add nuw nsw i64 %x73, 1
  %x_postcondition79 = icmp eq i64 %x_increment78, %4
  br i1 %x_postcondition79, label %x_exit72, label %x_body71

exit77:                                           ; preds = %true_entry76
  %74 = add nuw nsw i64 %x73, %54
  %75 = getelementptr float, float* %50, i64 %74
  %76 = fptrunc double %71 to float
  store float %76, float* %75, align 4, !llvm.mem.parallel_loop_access !4
  %77 = mul nuw nsw i64 %x73, %4
  %78 = add nuw nsw i64 %77, %y70
  %79 = getelementptr float, float* %50, i64 %78
  store float %76, float* %79, align 4, !llvm.mem.parallel_loop_access !4
  br label %Flow
}

; Function Attrs: nounwind argmemonly
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #0

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind argmemonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
!4 = distinct !{!4}
