; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f32XY* @covariance(%f32XY*) {
entry:
  %1 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 8480, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %6 = bitcast %u0CXYT* %5 to float*
  %7 = ptrtoint %u0CXYT* %5 to i64
  %8 = and i64 %7, 31
  %9 = icmp eq i64 %8, 0
  call void @llvm.assume(i1 %9)
  %scevgep67 = bitcast %u0CXYT* %5 to i8*
  %10 = shl nuw nsw i64 %4, 2
  call void @llvm.memset.p0i8.i64(i8* %scevgep67, i8 0, i64 %10, i32 32, i1 false)
  %11 = zext i32 %rows to i64
  %12 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 6, i64 0
  %13 = ptrtoint float* %12 to i64
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
  %20 = getelementptr %f32XY, %f32XY* %0, i64 0, i32 6, i64 %19
  %21 = load float, float* %20, align 4
  %22 = fadd fast float %21, %18
  store float %22, float* %17, align 4
  %x_increment10 = add nuw nsw i64 %x9, 1
  %x_postcondition11 = icmp eq i64 %x_increment10, %4
  br i1 %x_postcondition11, label %x_exit8, label %x_body7

x_exit8:                                          ; preds = %x_body7
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %11
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit8
  %23 = icmp eq i32 %rows, 1
  br i1 %23, label %Flow9, label %true_entry

true_entry:                                       ; preds = %y_exit
  %24 = uitofp i32 %rows to float
  %25 = fdiv fast float 1.000000e+00, %24
  br label %x_body15

Flow9:                                            ; preds = %x_body15, %y_exit
  %26 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %27 = getelementptr inbounds %u0CXYT, %u0CXYT* %26, i64 1
  %28 = bitcast %u0CXYT* %27 to float*
  %29 = ptrtoint %u0CXYT* %27 to i64
  %30 = and i64 %29, 31
  %31 = icmp eq i64 %30, 0
  call void @llvm.assume(i1 %31)
  %scevgep = getelementptr %u0CXYT, %u0CXYT* %26, i64 1, i32 0
  %scevgep3 = getelementptr %f32XY, %f32XY* %0, i64 1, i32 0
  br label %y_body28

x_body15:                                         ; preds = %true_entry, %x_body15
  %x17 = phi i64 [ %x_increment18, %x_body15 ], [ 0, %true_entry ]
  %32 = getelementptr float, float* %6, i64 %x17
  %33 = load float, float* %32, align 4, !llvm.mem.parallel_loop_access !1
  %34 = fmul fast float %33, %25
  store float %34, float* %32, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment18 = add nuw nsw i64 %x17, 1
  %x_postcondition19 = icmp eq i64 %x_increment18, %4
  br i1 %x_postcondition19, label %Flow9, label %x_body15

y_body28:                                         ; preds = %y_body28, %Flow9
  %y30 = phi i64 [ 0, %Flow9 ], [ %y_increment36, %y_body28 ]
  %35 = mul i64 %y30, %4
  %scevgep1 = getelementptr i32, i32* %scevgep, i64 %35
  %scevgep12 = bitcast i32* %scevgep1 to i8*
  %scevgep4 = getelementptr i32, i32* %scevgep3, i64 %35
  %scevgep45 = bitcast i32* %scevgep4 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep12, i8* %scevgep45, i64 %10, i32 4, i1 false)
  %y_increment36 = add nuw nsw i64 %y30, 1
  %y_postcondition37 = icmp eq i64 %y_increment36, %11
  br i1 %y_postcondition37, label %y_body47, label %y_body28

y_body47:                                         ; preds = %y_body28, %x_exit51
  %y49 = phi i64 [ %y_increment55, %x_exit51 ], [ 0, %y_body28 ]
  %36 = mul nuw nsw i64 %y49, %4
  br label %x_body50

x_body50:                                         ; preds = %y_body47, %x_body50
  %x52 = phi i64 [ %x_increment53, %x_body50 ], [ 0, %y_body47 ]
  %37 = add nuw nsw i64 %x52, %36
  %38 = getelementptr float, float* %28, i64 %37
  %39 = load float, float* %38, align 4, !llvm.mem.parallel_loop_access !2
  %40 = getelementptr float, float* %6, i64 %x52
  %41 = load float, float* %40, align 4, !llvm.mem.parallel_loop_access !2
  %42 = fsub fast float %39, %41
  store float %42, float* %38, align 4, !llvm.mem.parallel_loop_access !2
  %x_increment53 = add nuw nsw i64 %x52, 1
  %x_postcondition54 = icmp eq i64 %x_increment53, %4
  br i1 %x_postcondition54, label %x_exit51, label %x_body50

x_exit51:                                         ; preds = %x_body50
  %y_increment55 = add nuw nsw i64 %y49, 1
  %y_postcondition56 = icmp eq i64 %y_increment55, %11
  br i1 %y_postcondition56, label %y_exit48, label %y_body47

y_exit48:                                         ; preds = %x_exit51
  %43 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %44 = getelementptr inbounds %u0CXYT, %u0CXYT* %43, i64 1
  %45 = bitcast %u0CXYT* %44 to float*
  %46 = ptrtoint %u0CXYT* %44 to i64
  %47 = and i64 %46, 31
  %48 = icmp eq i64 %47, 0
  call void @llvm.assume(i1 %48)
  br label %y_body68

y_body68:                                         ; preds = %x_exit72, %y_exit48
  %y70 = phi i64 [ 0, %y_exit48 ], [ %y_increment80, %x_exit72 ]
  %49 = mul nuw nsw i64 %y70, %4
  br label %x_body71

x_body71:                                         ; preds = %y_body68, %Flow
  %x73 = phi i64 [ %x_increment78, %Flow ], [ 0, %y_body68 ]
  %50 = icmp ugt i64 %y70, %x73
  br i1 %50, label %Flow, label %true_entry76

x_exit72:                                         ; preds = %Flow
  %y_increment80 = add nuw nsw i64 %y70, 1
  %y_postcondition81 = icmp eq i64 %y_increment80, %4
  br i1 %y_postcondition81, label %y_exit69, label %y_body68

y_exit69:                                         ; preds = %x_exit72
  %dst = bitcast %u0CXYT* %43 to %f32XY*
  %51 = bitcast %u0CXYT* %2 to i8*
  call void @likely_release_mat(i8* %51)
  %52 = bitcast %u0CXYT* %26 to i8*
  call void @likely_release_mat(i8* %52)
  ret %f32XY* %dst

true_entry76:                                     ; preds = %x_body71, %true_entry76
  %53 = phi i32 [ %67, %true_entry76 ], [ 0, %x_body71 ]
  %54 = phi double [ %66, %true_entry76 ], [ 0.000000e+00, %x_body71 ]
  %55 = sext i32 %53 to i64
  %56 = mul nuw nsw i64 %55, %4
  %57 = add nuw nsw i64 %56, %x73
  %58 = getelementptr float, float* %28, i64 %57
  %59 = load float, float* %58, align 4, !llvm.mem.parallel_loop_access !3
  %60 = fpext float %59 to double
  %61 = add nuw nsw i64 %56, %y70
  %62 = getelementptr float, float* %28, i64 %61
  %63 = load float, float* %62, align 4, !llvm.mem.parallel_loop_access !3
  %64 = fpext float %63 to double
  %65 = fmul fast double %64, %60
  %66 = fadd fast double %65, %54
  %67 = add nuw nsw i32 %53, 1
  %68 = icmp eq i32 %67, %rows
  br i1 %68, label %exit77, label %true_entry76

Flow:                                             ; preds = %x_body71, %exit77
  %x_increment78 = add nuw nsw i64 %x73, 1
  %x_postcondition79 = icmp eq i64 %x_increment78, %4
  br i1 %x_postcondition79, label %x_exit72, label %x_body71

exit77:                                           ; preds = %true_entry76
  %69 = add nuw nsw i64 %x73, %49
  %70 = getelementptr float, float* %45, i64 %69
  %71 = fptrunc double %66 to float
  store float %71, float* %70, align 4, !llvm.mem.parallel_loop_access !3
  %72 = mul nuw nsw i64 %x73, %4
  %73 = add nuw nsw i64 %72, %y70
  %74 = getelementptr float, float* %45, i64 %73
  store float %71, float* %74, align 4, !llvm.mem.parallel_loop_access !3
  br label %Flow
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
