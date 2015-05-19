; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%u8XY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32X = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @multiply_transposed_tmp_thunk0({ %f32XY*, %u8XY*, %f32X* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32XY*, %u8XY*, %f32X* }, { %f32XY*, %u8XY*, %f32X* }* %0, i64 0, i32 0
  %4 = load %f32XY*, %f32XY** %3, align 8
  %5 = getelementptr inbounds { %f32XY*, %u8XY*, %f32X* }, { %f32XY*, %u8XY*, %f32X* }* %0, i64 0, i32 1
  %6 = load %u8XY*, %u8XY** %5, align 8
  %7 = getelementptr inbounds { %f32XY*, %u8XY*, %f32X* }, { %f32XY*, %u8XY*, %f32X* }* %0, i64 0, i32 2
  %8 = load %f32X*, %f32X** %7, align 8
  %9 = getelementptr inbounds %u8XY, %u8XY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %9, align 4, !range !0
  %centered_y_step = zext i32 %columns1 to i64
  %10 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 6, i64 0
  %11 = ptrtoint float* %10 to i64
  %12 = and i64 %11, 31
  %13 = icmp eq i64 %12, 0
  call void @llvm.assume(i1 %13)
  %14 = getelementptr inbounds %u8XY, %u8XY* %6, i64 0, i32 6, i64 0
  %15 = ptrtoint i8* %14 to i64
  %16 = and i64 %15, 31
  %17 = icmp eq i64 %16, 0
  call void @llvm.assume(i1 %17)
  %18 = getelementptr inbounds %f32X, %f32X* %8, i64 0, i32 6, i64 0
  %19 = ptrtoint float* %18 to i64
  %20 = and i64 %19, 31
  %21 = icmp eq i64 %20, 0
  call void @llvm.assume(i1 %21)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %22 = mul nuw nsw i64 %y, %centered_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %23 = add nuw nsw i64 %x, %22
  %24 = getelementptr %u8XY, %u8XY* %6, i64 0, i32 6, i64 %23
  %25 = load i8, i8* %24, align 1, !llvm.mem.parallel_loop_access !1
  %26 = getelementptr %f32X, %f32X* %8, i64 0, i32 6, i64 %x
  %27 = load float, float* %26, align 4, !llvm.mem.parallel_loop_access !1
  %28 = uitofp i8 %25 to float
  %29 = fsub fast float %28, %27
  %30 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %23
  store float %29, float* %30, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %centered_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: nounwind
define private void @multiply_transposed_tmp_thunk1({ %f32XY*, %f32XY*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %0, i64 0, i32 0
  %4 = load %f32XY*, %f32XY** %3, align 8
  %5 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %0, i64 0, i32 1
  %6 = load %f32XY*, %f32XY** %5, align 8
  %7 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %0, i64 0, i32 2
  %8 = load i32, i32* %7, align 4
  %9 = getelementptr inbounds %f32XY, %f32XY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %9, align 4, !range !0
  %dst_y_step = zext i32 %columns1 to i64
  %10 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 6, i64 0
  %11 = ptrtoint float* %10 to i64
  %12 = and i64 %11, 31
  %13 = icmp eq i64 %12, 0
  call void @llvm.assume(i1 %13)
  %14 = getelementptr inbounds %f32XY, %f32XY* %6, i64 0, i32 6, i64 0
  %15 = ptrtoint float* %14 to i64
  %16 = and i64 %15, 31
  %17 = icmp eq i64 %16, 0
  call void @llvm.assume(i1 %17)
  %18 = icmp eq i32 %8, 0
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %19 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %Flow6
  %x = phi i64 [ %x_increment, %Flow6 ], [ 0, %y_body ]
  %20 = icmp ugt i64 %y, %x
  br i1 %20, label %Flow6, label %label.preheader

label.preheader:                                  ; preds = %x_body
  br i1 %18, label %exit4, label %true_entry3

x_exit:                                           ; preds = %Flow6
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void

true_entry3:                                      ; preds = %label.preheader, %true_entry3
  %21 = phi i32 [ %35, %true_entry3 ], [ 0, %label.preheader ]
  %22 = phi double [ %34, %true_entry3 ], [ 0.000000e+00, %label.preheader ]
  %23 = sext i32 %21 to i64
  %24 = mul nuw nsw i64 %23, %dst_y_step
  %25 = add nuw nsw i64 %24, %x
  %26 = getelementptr %f32XY, %f32XY* %6, i64 0, i32 6, i64 %25
  %27 = load float, float* %26, align 4, !llvm.mem.parallel_loop_access !2
  %28 = fpext float %27 to double
  %29 = add nuw nsw i64 %24, %y
  %30 = getelementptr %f32XY, %f32XY* %6, i64 0, i32 6, i64 %29
  %31 = load float, float* %30, align 4, !llvm.mem.parallel_loop_access !2
  %32 = fpext float %31 to double
  %33 = fmul fast double %32, %28
  %34 = fadd fast double %33, %22
  %35 = add nuw nsw i32 %21, 1
  %36 = icmp eq i32 %35, %8
  br i1 %36, label %exit4, label %true_entry3

Flow6:                                            ; preds = %x_body, %exit4
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

exit4:                                            ; preds = %true_entry3, %label.preheader
  %.lcssa = phi double [ 0.000000e+00, %label.preheader ], [ %34, %true_entry3 ]
  %37 = fptrunc double %.lcssa to float
  %38 = add nuw nsw i64 %x, %19
  %39 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %38
  store float %37, float* %39, align 4, !llvm.mem.parallel_loop_access !2
  %40 = mul nuw nsw i64 %x, %dst_y_step
  %41 = add nuw nsw i64 %40, %y
  %42 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %41
  store float %37, float* %42, align 4, !llvm.mem.parallel_loop_access !2
  br label %Flow6
}

define %f32XY* @multiply_transposed(%u8XY*, %f32X*) {
entry:
  %2 = getelementptr inbounds %u8XY, %u8XY* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = getelementptr inbounds %u8XY, %u8XY* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %5 = zext i32 %rows to i64
  %6 = alloca { %f32XY*, %u8XY*, %f32X* }, align 8
  %7 = bitcast { %f32XY*, %u8XY*, %f32X* }* %6 to %u0CXYT**
  store %u0CXYT* %4, %u0CXYT** %7, align 8
  %8 = getelementptr inbounds { %f32XY*, %u8XY*, %f32X* }, { %f32XY*, %u8XY*, %f32X* }* %6, i64 0, i32 1
  store %u8XY* %0, %u8XY** %8, align 8
  %9 = getelementptr inbounds { %f32XY*, %u8XY*, %f32X* }, { %f32XY*, %u8XY*, %f32X* }* %6, i64 0, i32 2
  store %f32X* %1, %f32X** %9, align 8
  %10 = bitcast { %f32XY*, %u8XY*, %f32X* }* %6 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32XY*, %u8XY*, %f32X* }*, i64, i64)* @multiply_transposed_tmp_thunk0 to i8*), i8* %10, i64 %5)
  %11 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %12 = bitcast %u0CXYT* %11 to %f32XY*
  %13 = zext i32 %columns to i64
  %14 = alloca { %f32XY*, %f32XY*, i32 }, align 8
  %15 = bitcast { %f32XY*, %f32XY*, i32 }* %14 to %u0CXYT**
  store %u0CXYT* %11, %u0CXYT** %15, align 8
  %16 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %14, i64 0, i32 1
  %17 = bitcast %f32XY** %16 to %u0CXYT**
  store %u0CXYT* %4, %u0CXYT** %17, align 8
  %18 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %14, i64 0, i32 2
  store i32 %rows, i32* %18, align 8
  %19 = bitcast { %f32XY*, %f32XY*, i32 }* %14 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32XY*, %f32XY*, i32 }*, i64, i64)* @multiply_transposed_tmp_thunk1 to i8*), i8* %19, i64 %13)
  %20 = bitcast %u0CXYT* %4 to i8*
  call void @likely_release_mat(i8* %20)
  ret %f32XY* %12
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
