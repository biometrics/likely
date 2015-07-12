; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64X = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f64X* @average(%f64XY*) {
entry:
  %1 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 8512, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %6 = bitcast %u0CXYT* %5 to double*
  %scevgep1 = bitcast %u0CXYT* %5 to i8*
  %7 = shl nuw nsw i64 %4, 3
  call void @llvm.memset.p0i8.i64(i8* %scevgep1, i8 0, i64 %7, i32 8, i1 false)
  %8 = bitcast %u0CXYT* %2 to %f64X*
  %9 = zext i32 %rows to i64
  %10 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 6, i64 0
  %11 = ptrtoint double* %10 to i64
  %12 = and i64 %11, 31
  %13 = icmp eq i64 %12, 0
  call void @llvm.assume(i1 %13)
  br label %y_body

y_body:                                           ; preds = %x_exit8, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit8 ]
  %14 = mul nuw nsw i64 %y, %4
  br label %x_body7

x_body7:                                          ; preds = %y_body, %x_body7
  %x9 = phi i64 [ %x_increment10, %x_body7 ], [ 0, %y_body ]
  %15 = getelementptr double, double* %6, i64 %x9
  %16 = load double, double* %15, align 8
  %17 = add nuw nsw i64 %x9, %14
  %18 = getelementptr %f64XY, %f64XY* %0, i64 0, i32 6, i64 %17
  %19 = load double, double* %18, align 8
  %20 = fadd fast double %19, %16
  store double %20, double* %15, align 8
  %x_increment10 = add nuw nsw i64 %x9, 1
  %x_postcondition11 = icmp eq i64 %x_increment10, %4
  br i1 %x_postcondition11, label %x_exit8, label %x_body7

x_exit8:                                          ; preds = %x_body7
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %9
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit8
  %21 = icmp eq i32 %rows, 1
  br i1 %21, label %Flow2, label %true_entry

true_entry:                                       ; preds = %y_exit
  %22 = uitofp i32 %rows to double
  %23 = fdiv fast double 1.000000e+00, %22
  br label %x_body15

Flow2:                                            ; preds = %x_body15, %y_exit
  ret %f64X* %8

x_body15:                                         ; preds = %true_entry, %x_body15
  %x17 = phi i64 [ %x_increment18, %x_body15 ], [ 0, %true_entry ]
  %24 = getelementptr double, double* %6, i64 %x17
  %25 = load double, double* %24, align 8, !llvm.mem.parallel_loop_access !1
  %26 = fmul fast double %25, %23
  store double %26, double* %24, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment18 = add nuw nsw i64 %x17, 1
  %x_postcondition19 = icmp eq i64 %x_increment18, %4
  br i1 %x_postcondition19, label %Flow2, label %x_body15
}

; Function Attrs: nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #1

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
