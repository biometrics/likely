; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind argmemonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f64XY* @covariance(%f64XY*) {
entry:
  %1 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 8512, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %6 = bitcast %u0CXYT* %5 to double*
  %scevgep67 = bitcast %u0CXYT* %5 to i8*
  %7 = shl nuw nsw i64 %4, 3
  call void @llvm.memset.p0i8.i64(i8* %scevgep67, i8 0, i64 %7, i32 8, i1 false)
  %8 = zext i32 %rows to i64
  br label %y_body

y_body:                                           ; preds = %x_exit8, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit8 ]
  %9 = mul nuw nsw i64 %y, %4
  br label %x_body7

x_body7:                                          ; preds = %y_body, %x_body7
  %x9 = phi i64 [ %x_increment10, %x_body7 ], [ 0, %y_body ]
  %10 = getelementptr double, double* %6, i64 %x9
  %11 = load double, double* %10, align 8
  %12 = add nuw nsw i64 %x9, %9
  %13 = getelementptr %f64XY, %f64XY* %0, i64 0, i32 6, i64 %12
  %14 = load double, double* %13, align 8
  %15 = fadd fast double %14, %11
  store double %15, double* %10, align 8
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
  %17 = uitofp i32 %rows to double
  %18 = fdiv fast double 1.000000e+00, %17
  br label %x_body15

Flow9:                                            ; preds = %x_body15, %y_exit
  %19 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %20 = getelementptr inbounds %u0CXYT, %u0CXYT* %19, i64 1
  %scevgep = getelementptr %u0CXYT, %u0CXYT* %19, i64 1, i32 0
  %21 = shl nuw nsw i64 %4, 1
  %scevgep3 = getelementptr %f64XY, %f64XY* %0, i64 1, i32 0
  br label %y_body28

x_body15:                                         ; preds = %true_entry, %x_body15
  %x17 = phi i64 [ %x_increment18, %x_body15 ], [ 0, %true_entry ]
  %22 = getelementptr double, double* %6, i64 %x17
  %23 = load double, double* %22, align 8, !llvm.mem.parallel_loop_access !1
  %24 = fmul fast double %23, %18
  store double %24, double* %22, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment18 = add nuw nsw i64 %x17, 1
  %x_postcondition19 = icmp eq i64 %x_increment18, %4
  br i1 %x_postcondition19, label %Flow9, label %x_body15

y_body28:                                         ; preds = %y_body28, %Flow9
  %y30 = phi i64 [ 0, %Flow9 ], [ %y_increment36, %y_body28 ]
  %25 = mul i64 %y30, %21
  %scevgep1 = getelementptr i32, i32* %scevgep, i64 %25
  %scevgep12 = bitcast i32* %scevgep1 to i8*
  %scevgep4 = getelementptr i32, i32* %scevgep3, i64 %25
  %scevgep45 = bitcast i32* %scevgep4 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep12, i8* %scevgep45, i64 %7, i32 8, i1 false)
  %y_increment36 = add nuw nsw i64 %y30, 1
  %y_postcondition37 = icmp eq i64 %y_increment36, %8
  br i1 %y_postcondition37, label %y_body47.preheader, label %y_body28

y_body47.preheader:                               ; preds = %y_body28
  %26 = bitcast %u0CXYT* %20 to double*
  br label %y_body47

y_body47:                                         ; preds = %x_exit51, %y_body47.preheader
  %y49 = phi i64 [ 0, %y_body47.preheader ], [ %y_increment55, %x_exit51 ]
  %27 = mul nuw nsw i64 %y49, %4
  br label %x_body50

x_body50:                                         ; preds = %y_body47, %x_body50
  %x52 = phi i64 [ %x_increment53, %x_body50 ], [ 0, %y_body47 ]
  %28 = add nuw nsw i64 %x52, %27
  %29 = getelementptr double, double* %26, i64 %28
  %30 = load double, double* %29, align 8, !llvm.mem.parallel_loop_access !2
  %31 = getelementptr double, double* %6, i64 %x52
  %32 = load double, double* %31, align 8, !llvm.mem.parallel_loop_access !2
  %33 = fsub fast double %30, %32
  store double %33, double* %29, align 8, !llvm.mem.parallel_loop_access !2
  %x_increment53 = add nuw nsw i64 %x52, 1
  %x_postcondition54 = icmp eq i64 %x_increment53, %4
  br i1 %x_postcondition54, label %x_exit51, label %x_body50

x_exit51:                                         ; preds = %x_body50
  %y_increment55 = add nuw nsw i64 %y49, 1
  %y_postcondition56 = icmp eq i64 %y_increment55, %8
  br i1 %y_postcondition56, label %y_exit48, label %y_body47

y_exit48:                                         ; preds = %x_exit51
  %34 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %35 = getelementptr inbounds %u0CXYT, %u0CXYT* %34, i64 1
  %36 = bitcast %u0CXYT* %35 to double*
  %37 = ptrtoint %u0CXYT* %35 to i64
  %38 = and i64 %37, 31
  %39 = icmp eq i64 %38, 0
  call void @llvm.assume(i1 %39)
  br label %y_body68

y_body68:                                         ; preds = %x_exit72, %y_exit48
  %y70 = phi i64 [ 0, %y_exit48 ], [ %y_increment80, %x_exit72 ]
  %40 = mul nuw nsw i64 %y70, %4
  br label %x_body71

x_body71:                                         ; preds = %y_body68, %Flow
  %x73 = phi i64 [ %x_increment78, %Flow ], [ 0, %y_body68 ]
  %41 = icmp ugt i64 %y70, %x73
  br i1 %41, label %Flow, label %true_entry76

x_exit72:                                         ; preds = %Flow
  %y_increment80 = add nuw nsw i64 %y70, 1
  %y_postcondition81 = icmp eq i64 %y_increment80, %4
  br i1 %y_postcondition81, label %y_exit69, label %y_body68

y_exit69:                                         ; preds = %x_exit72
  %dst = bitcast %u0CXYT* %34 to %f64XY*
  %42 = bitcast %u0CXYT* %2 to i8*
  call void @likely_release_mat(i8* %42)
  %43 = bitcast %u0CXYT* %19 to i8*
  call void @likely_release_mat(i8* %43)
  ret %f64XY* %dst

true_entry76:                                     ; preds = %x_body71, %true_entry76
  %44 = phi i32 [ %56, %true_entry76 ], [ 0, %x_body71 ]
  %45 = phi double [ %55, %true_entry76 ], [ 0.000000e+00, %x_body71 ]
  %46 = sext i32 %44 to i64
  %47 = mul nuw nsw i64 %46, %4
  %48 = add nuw nsw i64 %47, %x73
  %49 = getelementptr double, double* %26, i64 %48
  %50 = load double, double* %49, align 8, !llvm.mem.parallel_loop_access !3
  %51 = add nuw nsw i64 %47, %y70
  %52 = getelementptr double, double* %26, i64 %51
  %53 = load double, double* %52, align 8, !llvm.mem.parallel_loop_access !3
  %54 = fmul fast double %53, %50
  %55 = fadd fast double %54, %45
  %56 = add nuw nsw i32 %44, 1
  %57 = icmp eq i32 %56, %rows
  br i1 %57, label %exit77, label %true_entry76

Flow:                                             ; preds = %x_body71, %exit77
  %x_increment78 = add nuw nsw i64 %x73, 1
  %x_postcondition79 = icmp eq i64 %x_increment78, %4
  br i1 %x_postcondition79, label %x_exit72, label %x_body71

exit77:                                           ; preds = %true_entry76
  %58 = add nuw nsw i64 %x73, %40
  %59 = getelementptr double, double* %36, i64 %58
  store double %55, double* %59, align 8, !llvm.mem.parallel_loop_access !3
  %60 = mul nuw nsw i64 %x73, %4
  %61 = add nuw nsw i64 %60, %y70
  %62 = getelementptr double, double* %36, i64 %61
  store double %55, double* %62, align 8, !llvm.mem.parallel_loop_access !3
  br label %Flow
}

; Function Attrs: nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture readonly, i64, i32, i1) #1

; Function Attrs: nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #1

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind argmemonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
