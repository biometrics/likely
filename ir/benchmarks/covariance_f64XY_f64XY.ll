; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind readonly
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
  %7 = ptrtoint %u0CXYT* %5 to i64
  %8 = and i64 %7, 31
  %9 = icmp eq i64 %8, 0
  call void @llvm.assume(i1 %9)
  %scevgep1 = bitcast %u0CXYT* %5 to i8*
  %10 = shl nuw nsw i64 %4, 3
  call void @llvm.memset.p0i8.i64(i8* %scevgep1, i8 0, i64 %10, i32 8, i1 false)
  %11 = zext i32 %rows to i64
  %12 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 6, i64 0
  %13 = ptrtoint double* %12 to i64
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
  %17 = getelementptr double, double* %6, i64 %x9
  %18 = load double, double* %17, align 8
  %19 = add nuw nsw i64 %x9, %16
  %20 = getelementptr %f64XY, %f64XY* %0, i64 0, i32 6, i64 %19
  %21 = load double, double* %20, align 8
  %22 = fadd fast double %21, %18
  store double %22, double* %17, align 8
  %x_increment10 = add nuw nsw i64 %x9, 1
  %x_postcondition11 = icmp eq i64 %x_increment10, %4
  br i1 %x_postcondition11, label %x_exit8, label %x_body7

x_exit8:                                          ; preds = %x_body7
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %11
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit8
  %23 = icmp eq i32 %rows, 1
  br i1 %23, label %Flow3, label %true_entry

true_entry:                                       ; preds = %y_exit
  %24 = uitofp i32 %rows to double
  %25 = fdiv fast double 1.000000e+00, %24
  br label %x_body15

Flow3:                                            ; preds = %x_body15, %y_exit
  %26 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %27 = getelementptr inbounds %u0CXYT, %u0CXYT* %26, i64 1
  %28 = bitcast %u0CXYT* %27 to double*
  %29 = ptrtoint %u0CXYT* %27 to i64
  %30 = and i64 %29, 31
  %31 = icmp eq i64 %30, 0
  call void @llvm.assume(i1 %31)
  br label %y_body31

x_body15:                                         ; preds = %true_entry, %x_body15
  %x17 = phi i64 [ %x_increment18, %x_body15 ], [ 0, %true_entry ]
  %32 = getelementptr double, double* %6, i64 %x17
  %33 = load double, double* %32, align 8, !llvm.mem.parallel_loop_access !1
  %34 = fmul fast double %33, %25
  store double %34, double* %32, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment18 = add nuw nsw i64 %x17, 1
  %x_postcondition19 = icmp eq i64 %x_increment18, %4
  br i1 %x_postcondition19, label %Flow3, label %x_body15

y_body31:                                         ; preds = %x_exit35, %Flow3
  %y33 = phi i64 [ 0, %Flow3 ], [ %y_increment39, %x_exit35 ]
  %35 = mul nuw nsw i64 %y33, %4
  br label %x_body34

x_body34:                                         ; preds = %y_body31, %x_body34
  %x36 = phi i64 [ %x_increment37, %x_body34 ], [ 0, %y_body31 ]
  %36 = add nuw nsw i64 %x36, %35
  %37 = getelementptr %f64XY, %f64XY* %0, i64 0, i32 6, i64 %36
  %38 = load double, double* %37, align 8, !llvm.mem.parallel_loop_access !2
  %39 = getelementptr double, double* %6, i64 %x36
  %40 = load double, double* %39, align 8, !llvm.mem.parallel_loop_access !2
  %41 = fsub fast double %38, %40
  %42 = getelementptr double, double* %28, i64 %36
  store double %41, double* %42, align 8, !llvm.mem.parallel_loop_access !2
  %x_increment37 = add nuw nsw i64 %x36, 1
  %x_postcondition38 = icmp eq i64 %x_increment37, %4
  br i1 %x_postcondition38, label %x_exit35, label %x_body34

x_exit35:                                         ; preds = %x_body34
  %y_increment39 = add nuw nsw i64 %y33, 1
  %y_postcondition40 = icmp eq i64 %y_increment39, %11
  br i1 %y_postcondition40, label %y_exit32, label %y_body31

y_exit32:                                         ; preds = %x_exit35
  %43 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %44 = getelementptr inbounds %u0CXYT, %u0CXYT* %43, i64 1
  %45 = bitcast %u0CXYT* %44 to double*
  %46 = ptrtoint %u0CXYT* %44 to i64
  %47 = and i64 %46, 31
  %48 = icmp eq i64 %47, 0
  call void @llvm.assume(i1 %48)
  br label %y_body52

y_body52:                                         ; preds = %x_exit56, %y_exit32
  %y54 = phi i64 [ 0, %y_exit32 ], [ %y_increment64, %x_exit56 ]
  %49 = mul nuw nsw i64 %y54, %4
  br label %x_body55

x_body55:                                         ; preds = %y_body52, %Flow
  %x57 = phi i64 [ %x_increment62, %Flow ], [ 0, %y_body52 ]
  %50 = icmp ugt i64 %y54, %x57
  br i1 %50, label %Flow, label %true_entry60

x_exit56:                                         ; preds = %Flow
  %y_increment64 = add nuw nsw i64 %y54, 1
  %y_postcondition65 = icmp eq i64 %y_increment64, %4
  br i1 %y_postcondition65, label %y_exit53, label %y_body52

y_exit53:                                         ; preds = %x_exit56
  %dst = bitcast %u0CXYT* %43 to %f64XY*
  %51 = bitcast %u0CXYT* %2 to i8*
  call void @likely_release_mat(i8* %51)
  %52 = bitcast %u0CXYT* %26 to i8*
  call void @likely_release_mat(i8* %52)
  ret %f64XY* %dst

true_entry60:                                     ; preds = %x_body55, %true_entry60
  %53 = phi i32 [ %65, %true_entry60 ], [ 0, %x_body55 ]
  %54 = phi double [ %64, %true_entry60 ], [ 0.000000e+00, %x_body55 ]
  %55 = sext i32 %53 to i64
  %56 = mul nuw nsw i64 %55, %4
  %57 = add nuw nsw i64 %56, %x57
  %58 = getelementptr double, double* %28, i64 %57
  %59 = load double, double* %58, align 8, !llvm.mem.parallel_loop_access !3
  %60 = add nuw nsw i64 %56, %y54
  %61 = getelementptr double, double* %28, i64 %60
  %62 = load double, double* %61, align 8, !llvm.mem.parallel_loop_access !3
  %63 = fmul fast double %62, %59
  %64 = fadd fast double %63, %54
  %65 = add nuw nsw i32 %53, 1
  %66 = icmp eq i32 %65, %rows
  br i1 %66, label %exit61, label %true_entry60

Flow:                                             ; preds = %x_body55, %exit61
  %x_increment62 = add nuw nsw i64 %x57, 1
  %x_postcondition63 = icmp eq i64 %x_increment62, %4
  br i1 %x_postcondition63, label %x_exit56, label %x_body55

exit61:                                           ; preds = %true_entry60
  %67 = add nuw nsw i64 %x57, %49
  %68 = getelementptr double, double* %45, i64 %67
  store double %64, double* %68, align 8, !llvm.mem.parallel_loop_access !3
  %69 = mul nuw nsw i64 %x57, %4
  %70 = add nuw nsw i64 %69, %y54
  %71 = getelementptr double, double* %45, i64 %70
  store double %64, double* %71, align 8, !llvm.mem.parallel_loop_access !3
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
