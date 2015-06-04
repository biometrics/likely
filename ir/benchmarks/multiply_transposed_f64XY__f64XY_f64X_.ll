; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }
%f64X = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f64XY* @multiply_transposed(%f64XY*, %f64X*) {
entry:
  %2 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %5 = zext i32 %rows to i64
  %6 = getelementptr inbounds %u0CXYT, %u0CXYT* %4, i64 1
  %7 = ptrtoint %u0CXYT* %6 to i64
  %8 = and i64 %7, 31
  %9 = icmp eq i64 %8, 0
  call void @llvm.assume(i1 %9)
  %10 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 6, i64 0
  %11 = ptrtoint double* %10 to i64
  %12 = and i64 %11, 31
  %13 = icmp eq i64 %12, 0
  call void @llvm.assume(i1 %13)
  %scevgep = getelementptr %u0CXYT, %u0CXYT* %4, i64 1, i32 0
  %14 = zext i32 %columns to i64
  %15 = shl nuw nsw i64 %14, 1
  %scevgep3 = getelementptr %f64XY, %f64XY* %0, i64 1, i32 0
  %16 = shl nuw nsw i64 %14, 3
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %17 = mul i64 %y, %15
  %scevgep1 = getelementptr i32, i32* %scevgep, i64 %17
  %scevgep12 = bitcast i32* %scevgep1 to i8*
  %scevgep4 = getelementptr i32, i32* %scevgep3, i64 %17
  %scevgep45 = bitcast i32* %scevgep4 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep12, i8* %scevgep45, i64 %16, i32 8, i1 false)
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %5
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %18 = bitcast %u0CXYT* %6 to double*
  %19 = getelementptr inbounds %f64X, %f64X* %1, i64 0, i32 6, i64 0
  %20 = ptrtoint double* %19 to i64
  %21 = and i64 %20, 31
  %22 = icmp eq i64 %21, 0
  call void @llvm.assume(i1 %22)
  br label %y_body15

y_body15:                                         ; preds = %x_exit19, %y_exit
  %y17 = phi i64 [ 0, %y_exit ], [ %y_increment23, %x_exit19 ]
  %23 = mul nuw nsw i64 %y17, %14
  br label %x_body18

x_body18:                                         ; preds = %y_body15, %x_body18
  %x20 = phi i64 [ %x_increment21, %x_body18 ], [ 0, %y_body15 ]
  %24 = add nuw nsw i64 %x20, %23
  %25 = getelementptr double, double* %18, i64 %24
  %26 = load double, double* %25, align 8, !llvm.mem.parallel_loop_access !1
  %27 = getelementptr %f64X, %f64X* %1, i64 0, i32 6, i64 %x20
  %28 = load double, double* %27, align 8, !llvm.mem.parallel_loop_access !1
  %29 = fsub fast double %26, %28
  store double %29, double* %25, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment21 = add nuw nsw i64 %x20, 1
  %x_postcondition22 = icmp eq i64 %x_increment21, %14
  br i1 %x_postcondition22, label %x_exit19, label %x_body18

x_exit19:                                         ; preds = %x_body18
  %y_increment23 = add nuw nsw i64 %y17, 1
  %y_postcondition24 = icmp eq i64 %y_increment23, %5
  br i1 %y_postcondition24, label %y_exit16, label %y_body15

y_exit16:                                         ; preds = %x_exit19
  %30 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %31 = getelementptr inbounds %u0CXYT, %u0CXYT* %30, i64 1
  %32 = bitcast %u0CXYT* %31 to double*
  %33 = ptrtoint %u0CXYT* %31 to i64
  %34 = and i64 %33, 31
  %35 = icmp eq i64 %34, 0
  call void @llvm.assume(i1 %35)
  br label %y_body33

y_body33:                                         ; preds = %x_exit37, %y_exit16
  %y35 = phi i64 [ 0, %y_exit16 ], [ %y_increment43, %x_exit37 ]
  %36 = mul nuw nsw i64 %y35, %14
  br label %x_body36

x_body36:                                         ; preds = %y_body33, %Flow
  %x38 = phi i64 [ %x_increment41, %Flow ], [ 0, %y_body33 ]
  %37 = icmp ugt i64 %y35, %x38
  br i1 %37, label %Flow, label %true_entry39

x_exit37:                                         ; preds = %Flow
  %y_increment43 = add nuw nsw i64 %y35, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %14
  br i1 %y_postcondition44, label %y_exit34, label %y_body33

y_exit34:                                         ; preds = %x_exit37
  %dst = bitcast %u0CXYT* %30 to %f64XY*
  %38 = bitcast %u0CXYT* %4 to i8*
  call void @likely_release_mat(i8* %38)
  ret %f64XY* %dst

true_entry39:                                     ; preds = %x_body36, %true_entry39
  %39 = phi i32 [ %51, %true_entry39 ], [ 0, %x_body36 ]
  %40 = phi double [ %50, %true_entry39 ], [ 0.000000e+00, %x_body36 ]
  %41 = sext i32 %39 to i64
  %42 = mul nuw nsw i64 %41, %14
  %43 = add nuw nsw i64 %42, %x38
  %44 = getelementptr double, double* %18, i64 %43
  %45 = load double, double* %44, align 8, !llvm.mem.parallel_loop_access !2
  %46 = add nuw nsw i64 %42, %y35
  %47 = getelementptr double, double* %18, i64 %46
  %48 = load double, double* %47, align 8, !llvm.mem.parallel_loop_access !2
  %49 = fmul fast double %48, %45
  %50 = fadd fast double %49, %40
  %51 = add nuw nsw i32 %39, 1
  %52 = icmp eq i32 %51, %rows
  br i1 %52, label %exit40, label %true_entry39

Flow:                                             ; preds = %x_body36, %exit40
  %x_increment41 = add nuw nsw i64 %x38, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %14
  br i1 %x_postcondition42, label %x_exit37, label %x_body36

exit40:                                           ; preds = %true_entry39
  %53 = add nuw nsw i64 %x38, %36
  %54 = getelementptr double, double* %32, i64 %53
  store double %50, double* %54, align 8, !llvm.mem.parallel_loop_access !2
  %55 = mul nuw nsw i64 %x38, %14
  %56 = add nuw nsw i64 %55, %y35
  %57 = getelementptr double, double* %32, i64 %56
  store double %50, double* %57, align 8, !llvm.mem.parallel_loop_access !2
  br label %Flow
}

; Function Attrs: nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture readonly, i64, i32, i1) #1

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
