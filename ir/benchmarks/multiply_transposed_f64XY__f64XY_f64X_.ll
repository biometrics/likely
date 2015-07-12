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
  %7 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 6, i64 0
  %8 = ptrtoint double* %7 to i64
  %9 = and i64 %8, 31
  %10 = icmp eq i64 %9, 0
  call void @llvm.assume(i1 %10)
  %scevgep = getelementptr %u0CXYT, %u0CXYT* %4, i64 1, i32 0
  %11 = zext i32 %columns to i64
  %12 = shl nuw nsw i64 %11, 1
  %scevgep3 = getelementptr %f64XY, %f64XY* %0, i64 1, i32 0
  %13 = shl nuw nsw i64 %11, 3
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %14 = mul i64 %y, %12
  %scevgep1 = getelementptr i32, i32* %scevgep, i64 %14
  %scevgep12 = bitcast i32* %scevgep1 to i8*
  %scevgep4 = getelementptr i32, i32* %scevgep3, i64 %14
  %scevgep45 = bitcast i32* %scevgep4 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep12, i8* %scevgep45, i64 %13, i32 8, i1 false)
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %5
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %15 = bitcast %u0CXYT* %6 to double*
  %16 = getelementptr inbounds %f64X, %f64X* %1, i64 0, i32 6, i64 0
  %17 = ptrtoint double* %16 to i64
  %18 = and i64 %17, 31
  %19 = icmp eq i64 %18, 0
  call void @llvm.assume(i1 %19)
  br label %y_body15

y_body15:                                         ; preds = %x_exit19, %y_exit
  %y17 = phi i64 [ 0, %y_exit ], [ %y_increment23, %x_exit19 ]
  %20 = mul nuw nsw i64 %y17, %11
  br label %x_body18

x_body18:                                         ; preds = %y_body15, %x_body18
  %x20 = phi i64 [ %x_increment21, %x_body18 ], [ 0, %y_body15 ]
  %21 = add nuw nsw i64 %x20, %20
  %22 = getelementptr double, double* %15, i64 %21
  %23 = load double, double* %22, align 8, !llvm.mem.parallel_loop_access !1
  %24 = getelementptr %f64X, %f64X* %1, i64 0, i32 6, i64 %x20
  %25 = load double, double* %24, align 8, !llvm.mem.parallel_loop_access !1
  %26 = fsub fast double %23, %25
  store double %26, double* %22, align 8, !llvm.mem.parallel_loop_access !1
  %x_increment21 = add nuw nsw i64 %x20, 1
  %x_postcondition22 = icmp eq i64 %x_increment21, %11
  br i1 %x_postcondition22, label %x_exit19, label %x_body18

x_exit19:                                         ; preds = %x_body18
  %y_increment23 = add nuw nsw i64 %y17, 1
  %y_postcondition24 = icmp eq i64 %y_increment23, %5
  br i1 %y_postcondition24, label %y_exit16, label %y_body15

y_exit16:                                         ; preds = %x_exit19
  %27 = call %u0CXYT* @likely_new(i32 24896, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %28 = getelementptr inbounds %u0CXYT, %u0CXYT* %27, i64 1
  %29 = bitcast %u0CXYT* %28 to double*
  %30 = ptrtoint %u0CXYT* %28 to i64
  %31 = and i64 %30, 31
  %32 = icmp eq i64 %31, 0
  call void @llvm.assume(i1 %32)
  br label %y_body33

y_body33:                                         ; preds = %x_exit37, %y_exit16
  %y35 = phi i64 [ 0, %y_exit16 ], [ %y_increment43, %x_exit37 ]
  %33 = mul nuw nsw i64 %y35, %11
  br label %x_body36

x_body36:                                         ; preds = %y_body33, %Flow
  %x38 = phi i64 [ %x_increment41, %Flow ], [ 0, %y_body33 ]
  %34 = icmp ugt i64 %y35, %x38
  br i1 %34, label %Flow, label %true_entry39

x_exit37:                                         ; preds = %Flow
  %y_increment43 = add nuw nsw i64 %y35, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %11
  br i1 %y_postcondition44, label %y_exit34, label %y_body33

y_exit34:                                         ; preds = %x_exit37
  %dst = bitcast %u0CXYT* %27 to %f64XY*
  %35 = bitcast %u0CXYT* %4 to i8*
  call void @likely_release_mat(i8* %35)
  ret %f64XY* %dst

true_entry39:                                     ; preds = %x_body36, %true_entry39
  %36 = phi i32 [ %48, %true_entry39 ], [ 0, %x_body36 ]
  %37 = phi double [ %47, %true_entry39 ], [ 0.000000e+00, %x_body36 ]
  %38 = sext i32 %36 to i64
  %39 = mul nuw nsw i64 %38, %11
  %40 = add nuw nsw i64 %39, %x38
  %41 = getelementptr double, double* %15, i64 %40
  %42 = load double, double* %41, align 8, !llvm.mem.parallel_loop_access !2
  %43 = add nuw nsw i64 %39, %y35
  %44 = getelementptr double, double* %15, i64 %43
  %45 = load double, double* %44, align 8, !llvm.mem.parallel_loop_access !2
  %46 = fmul fast double %45, %42
  %47 = fadd fast double %46, %37
  %48 = add nuw nsw i32 %36, 1
  %49 = icmp eq i32 %48, %rows
  br i1 %49, label %exit40, label %true_entry39

Flow:                                             ; preds = %x_body36, %exit40
  %x_increment41 = add nuw nsw i64 %x38, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %11
  br i1 %x_postcondition42, label %x_exit37, label %x_body36

exit40:                                           ; preds = %true_entry39
  %50 = add nuw nsw i64 %x38, %33
  %51 = getelementptr double, double* %29, i64 %50
  store double %47, double* %51, align 8, !llvm.mem.parallel_loop_access !2
  %52 = mul nuw nsw i64 %x38, %11
  %53 = add nuw nsw i64 %52, %y35
  %54 = getelementptr double, double* %29, i64 %53
  store double %47, double* %54, align 8, !llvm.mem.parallel_loop_access !2
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
