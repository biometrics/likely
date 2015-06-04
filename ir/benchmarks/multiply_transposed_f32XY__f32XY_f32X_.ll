; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%f32X = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f32XY* @multiply_transposed(%f32XY*, %f32X*) {
entry:
  %2 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %5 = zext i32 %rows to i64
  %6 = getelementptr inbounds %u0CXYT, %u0CXYT* %4, i64 1
  %7 = ptrtoint %u0CXYT* %6 to i64
  %8 = and i64 %7, 31
  %9 = icmp eq i64 %8, 0
  call void @llvm.assume(i1 %9)
  %10 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 6, i64 0
  %11 = ptrtoint float* %10 to i64
  %12 = and i64 %11, 31
  %13 = icmp eq i64 %12, 0
  call void @llvm.assume(i1 %13)
  %scevgep = getelementptr %u0CXYT, %u0CXYT* %4, i64 1, i32 0
  %14 = zext i32 %columns to i64
  %scevgep3 = getelementptr %f32XY, %f32XY* %0, i64 1, i32 0
  %15 = shl nuw nsw i64 %14, 2
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %y_body ]
  %16 = mul i64 %y, %14
  %scevgep1 = getelementptr i32, i32* %scevgep, i64 %16
  %scevgep12 = bitcast i32* %scevgep1 to i8*
  %scevgep4 = getelementptr i32, i32* %scevgep3, i64 %16
  %scevgep45 = bitcast i32* %scevgep4 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep12, i8* %scevgep45, i64 %15, i32 4, i1 false)
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %5
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
  %17 = bitcast %u0CXYT* %6 to float*
  %18 = getelementptr inbounds %f32X, %f32X* %1, i64 0, i32 6, i64 0
  %19 = ptrtoint float* %18 to i64
  %20 = and i64 %19, 31
  %21 = icmp eq i64 %20, 0
  call void @llvm.assume(i1 %21)
  br label %y_body15

y_body15:                                         ; preds = %x_exit19, %y_exit
  %y17 = phi i64 [ 0, %y_exit ], [ %y_increment23, %x_exit19 ]
  %22 = mul nuw nsw i64 %y17, %14
  br label %x_body18

x_body18:                                         ; preds = %y_body15, %x_body18
  %x20 = phi i64 [ %x_increment21, %x_body18 ], [ 0, %y_body15 ]
  %23 = add nuw nsw i64 %x20, %22
  %24 = getelementptr float, float* %17, i64 %23
  %25 = load float, float* %24, align 4, !llvm.mem.parallel_loop_access !1
  %26 = getelementptr %f32X, %f32X* %1, i64 0, i32 6, i64 %x20
  %27 = load float, float* %26, align 4, !llvm.mem.parallel_loop_access !1
  %28 = fsub fast float %25, %27
  store float %28, float* %24, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment21 = add nuw nsw i64 %x20, 1
  %x_postcondition22 = icmp eq i64 %x_increment21, %14
  br i1 %x_postcondition22, label %x_exit19, label %x_body18

x_exit19:                                         ; preds = %x_body18
  %y_increment23 = add nuw nsw i64 %y17, 1
  %y_postcondition24 = icmp eq i64 %y_increment23, %5
  br i1 %y_postcondition24, label %y_exit16, label %y_body15

y_exit16:                                         ; preds = %x_exit19
  %29 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %30 = getelementptr inbounds %u0CXYT, %u0CXYT* %29, i64 1
  %31 = bitcast %u0CXYT* %30 to float*
  %32 = ptrtoint %u0CXYT* %30 to i64
  %33 = and i64 %32, 31
  %34 = icmp eq i64 %33, 0
  call void @llvm.assume(i1 %34)
  br label %y_body33

y_body33:                                         ; preds = %x_exit37, %y_exit16
  %y35 = phi i64 [ 0, %y_exit16 ], [ %y_increment43, %x_exit37 ]
  %35 = mul nuw nsw i64 %y35, %14
  br label %x_body36

x_body36:                                         ; preds = %y_body33, %Flow
  %x38 = phi i64 [ %x_increment41, %Flow ], [ 0, %y_body33 ]
  %36 = icmp ugt i64 %y35, %x38
  br i1 %36, label %Flow, label %true_entry39

x_exit37:                                         ; preds = %Flow
  %y_increment43 = add nuw nsw i64 %y35, 1
  %y_postcondition44 = icmp eq i64 %y_increment43, %14
  br i1 %y_postcondition44, label %y_exit34, label %y_body33

y_exit34:                                         ; preds = %x_exit37
  %dst = bitcast %u0CXYT* %29 to %f32XY*
  %37 = bitcast %u0CXYT* %4 to i8*
  call void @likely_release_mat(i8* %37)
  ret %f32XY* %dst

true_entry39:                                     ; preds = %x_body36, %true_entry39
  %38 = phi i32 [ %52, %true_entry39 ], [ 0, %x_body36 ]
  %39 = phi double [ %51, %true_entry39 ], [ 0.000000e+00, %x_body36 ]
  %40 = sext i32 %38 to i64
  %41 = mul nuw nsw i64 %40, %14
  %42 = add nuw nsw i64 %41, %x38
  %43 = getelementptr float, float* %17, i64 %42
  %44 = load float, float* %43, align 4, !llvm.mem.parallel_loop_access !2
  %45 = fpext float %44 to double
  %46 = add nuw nsw i64 %41, %y35
  %47 = getelementptr float, float* %17, i64 %46
  %48 = load float, float* %47, align 4, !llvm.mem.parallel_loop_access !2
  %49 = fpext float %48 to double
  %50 = fmul fast double %49, %45
  %51 = fadd fast double %50, %39
  %52 = add nuw nsw i32 %38, 1
  %53 = icmp eq i32 %52, %rows
  br i1 %53, label %exit40, label %true_entry39

Flow:                                             ; preds = %x_body36, %exit40
  %x_increment41 = add nuw nsw i64 %x38, 1
  %x_postcondition42 = icmp eq i64 %x_increment41, %14
  br i1 %x_postcondition42, label %x_exit37, label %x_body36

exit40:                                           ; preds = %true_entry39
  %54 = add nuw nsw i64 %x38, %35
  %55 = getelementptr float, float* %31, i64 %54
  %56 = fptrunc double %51 to float
  store float %56, float* %55, align 4, !llvm.mem.parallel_loop_access !2
  %57 = mul nuw nsw i64 %x38, %14
  %58 = add nuw nsw i64 %57, %y35
  %59 = getelementptr float, float* %31, i64 %58
  store float %56, float* %59, align 4, !llvm.mem.parallel_loop_access !2
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
