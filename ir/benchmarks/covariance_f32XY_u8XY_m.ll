; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32X = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%u8XY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @covariance_tmp_thunk0({ %f32X*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32X*, i32 }, { %f32X*, i32 }* %0, i64 0, i32 0
  %4 = load %f32X*, %f32X** %3, align 8
  %5 = getelementptr inbounds { %f32X*, i32 }, { %f32X*, i32 }* %0, i64 0, i32 1
  %6 = load i32, i32* %5, align 4
  %7 = getelementptr inbounds %f32X, %f32X* %4, i64 0, i32 6, i64 0
  %8 = ptrtoint float* %7 to i64
  %9 = and i64 %8, 31
  %10 = icmp eq i64 %9, 0
  call void @llvm.assume(i1 %10)
  %11 = sitofp i32 %6 to float
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %12 = getelementptr %f32X, %f32X* %4, i64 0, i32 6, i64 %x
  store float %11, float* %12, align 4, !llvm.mem.parallel_loop_access !0
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

; Function Attrs: nounwind
define private void @covariance_tmp_thunk1({ %f32X*, float }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32X*, float }, { %f32X*, float }* %0, i64 0, i32 0
  %4 = load %f32X*, %f32X** %3, align 8
  %5 = getelementptr inbounds { %f32X*, float }, { %f32X*, float }* %0, i64 0, i32 1
  %6 = load float, float* %5, align 4
  %7 = getelementptr inbounds %f32X, %f32X* %4, i64 0, i32 6, i64 0
  %8 = ptrtoint float* %7 to i64
  %9 = and i64 %8, 31
  %10 = icmp eq i64 %9, 0
  call void @llvm.assume(i1 %10)
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ %1, %entry ], [ %x_increment, %x_body ]
  %11 = getelementptr %f32X, %f32X* %4, i64 0, i32 6, i64 %x
  %12 = load float, float* %11, align 4, !llvm.mem.parallel_loop_access !1
  %13 = fmul fast float %12, %6
  store float %13, float* %11, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %2
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  ret void
}

; Function Attrs: nounwind
define private void @covariance_tmp_thunk2({ %f32XY*, %u8XY*, %f32X* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32XY*, %u8XY*, %f32X* }, { %f32XY*, %u8XY*, %f32X* }* %0, i64 0, i32 0
  %4 = load %f32XY*, %f32XY** %3, align 8
  %5 = getelementptr inbounds { %f32XY*, %u8XY*, %f32X* }, { %f32XY*, %u8XY*, %f32X* }* %0, i64 0, i32 1
  %6 = load %u8XY*, %u8XY** %5, align 8
  %7 = getelementptr inbounds { %f32XY*, %u8XY*, %f32X* }, { %f32XY*, %u8XY*, %f32X* }* %0, i64 0, i32 2
  %8 = load %f32X*, %f32X** %7, align 8
  %9 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 3
  %columns = load i32, i32* %9, align 4, !range !2
  %centered_y_step = zext i32 %columns to i64
  %10 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 6, i64 0
  %11 = ptrtoint float* %10 to i64
  %12 = and i64 %11, 31
  %13 = icmp eq i64 %12, 0
  call void @llvm.assume(i1 %13)
  %14 = getelementptr inbounds %u8XY, %u8XY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %14, align 4, !range !2
  %src_y_step = zext i32 %columns1 to i64
  %15 = getelementptr inbounds %u8XY, %u8XY* %6, i64 0, i32 6, i64 0
  %16 = ptrtoint i8* %15 to i64
  %17 = and i64 %16, 31
  %18 = icmp eq i64 %17, 0
  call void @llvm.assume(i1 %18)
  %19 = getelementptr inbounds %f32X, %f32X* %8, i64 0, i32 6, i64 0
  %20 = ptrtoint float* %19 to i64
  %21 = and i64 %20, 31
  %22 = icmp eq i64 %21, 0
  call void @llvm.assume(i1 %22)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %23 = mul nuw nsw i64 %y, %src_y_step
  %24 = mul nuw nsw i64 %y, %centered_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %25 = add nuw nsw i64 %x, %23
  %26 = getelementptr %u8XY, %u8XY* %6, i64 0, i32 6, i64 %25
  %27 = load i8, i8* %26, align 1, !llvm.mem.parallel_loop_access !3
  %28 = getelementptr %f32X, %f32X* %8, i64 0, i32 6, i64 %x
  %29 = load float, float* %28, align 4, !llvm.mem.parallel_loop_access !3
  %30 = uitofp i8 %27 to float
  %31 = fsub fast float %30, %29
  %32 = add nuw nsw i64 %x, %24
  %33 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %32
  store float %31, float* %33, align 4, !llvm.mem.parallel_loop_access !3
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
define private void @covariance_tmp_thunk3({ %f32XY*, %f32XY*, i32 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %0, i64 0, i32 0
  %4 = load %f32XY*, %f32XY** %3, align 8
  %5 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %0, i64 0, i32 1
  %6 = load %f32XY*, %f32XY** %5, align 8
  %7 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %0, i64 0, i32 2
  %8 = load i32, i32* %7, align 4
  %9 = getelementptr inbounds %f32XY, %f32XY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %9, align 4, !range !2
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
  br i1 %20, label %Flow6, label %loop.preheader

loop.preheader:                                   ; preds = %x_body
  br i1 %18, label %exit4, label %true_entry3

x_exit:                                           ; preds = %Flow6
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void

true_entry3:                                      ; preds = %loop.preheader, %true_entry3
  %21 = phi i32 [ %35, %true_entry3 ], [ 0, %loop.preheader ]
  %22 = phi double [ %34, %true_entry3 ], [ 0.000000e+00, %loop.preheader ]
  %23 = sext i32 %21 to i64
  %24 = mul nuw nsw i64 %23, %dst_y_step
  %25 = add nuw nsw i64 %24, %x
  %26 = getelementptr %f32XY, %f32XY* %6, i64 0, i32 6, i64 %25
  %27 = load float, float* %26, align 4, !llvm.mem.parallel_loop_access !4
  %28 = fpext float %27 to double
  %29 = add nuw nsw i64 %24, %y
  %30 = getelementptr %f32XY, %f32XY* %6, i64 0, i32 6, i64 %29
  %31 = load float, float* %30, align 4, !llvm.mem.parallel_loop_access !4
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

exit4:                                            ; preds = %true_entry3, %loop.preheader
  %.lcssa = phi double [ 0.000000e+00, %loop.preheader ], [ %34, %true_entry3 ]
  %37 = add nuw nsw i64 %x, %19
  %38 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %37
  %39 = fptrunc double %.lcssa to float
  store float %39, float* %38, align 4, !llvm.mem.parallel_loop_access !4
  %40 = mul nuw nsw i64 %x, %dst_y_step
  %41 = add nuw nsw i64 %40, %y
  %42 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %41
  store float %39, float* %42, align 4, !llvm.mem.parallel_loop_access !4
  br label %Flow6
}

define %f32XY* @covariance(%u8XY*) {
entry:
  %1 = getelementptr inbounds %u8XY, %u8XY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !2
  %2 = call %u0CXYT* @likely_new(i32 8480, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %u8XY, %u8XY* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !2
  %4 = zext i32 %columns to i64
  %5 = alloca { %f32X*, i32 }, align 8
  %6 = bitcast { %f32X*, i32 }* %5 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %6, align 8
  %7 = getelementptr inbounds { %f32X*, i32 }, { %f32X*, i32 }* %5, i64 0, i32 1
  store i32 0, i32* %7, align 8
  %8 = bitcast { %f32X*, i32 }* %5 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32X*, i32 }*, i64, i64)* @covariance_tmp_thunk0 to i8*), i8* %8, i64 %4)
  %rows2 = load i32, i32* %3, align 4, !range !2
  %9 = zext i32 %rows2 to i64
  %10 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %11 = bitcast %u0CXYT* %10 to float*
  %12 = ptrtoint %u0CXYT* %10 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  call void @llvm.assume(i1 %14)
  %columns4 = load i32, i32* %1, align 4, !range !2
  %src_y_step = zext i32 %columns4 to i64
  %15 = getelementptr inbounds %u8XY, %u8XY* %0, i64 0, i32 6, i64 0
  %16 = ptrtoint i8* %15 to i64
  %17 = and i64 %16, 31
  %18 = icmp eq i64 %17, 0
  call void @llvm.assume(i1 %18)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %19 = mul nuw nsw i64 %y, %src_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %20 = getelementptr float, float* %11, i64 %x
  %21 = load float, float* %20, align 4
  %22 = add nuw nsw i64 %x, %19
  %23 = getelementptr %u8XY, %u8XY* %0, i64 0, i32 6, i64 %22
  %24 = load i8, i8* %23, align 1
  %25 = uitofp i8 %24 to float
  %26 = fadd fast float %25, %21
  store float %26, float* %20, align 4
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %src_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %9
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %27 = icmp eq i32 %rows, 1
  br i1 %27, label %exit, label %true_entry

true_entry:                                       ; preds = %y_exit
  %28 = uitofp i32 %rows to float
  %29 = fdiv fast float 1.000000e+00, %28
  %30 = alloca { %f32X*, float }, align 8
  %31 = bitcast { %f32X*, float }* %30 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %31, align 8
  %32 = getelementptr inbounds { %f32X*, float }, { %f32X*, float }* %30, i64 0, i32 1
  store float %29, float* %32, align 8
  %33 = bitcast { %f32X*, float }* %30 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32X*, float }*, i64, i64)* @covariance_tmp_thunk1 to i8*), i8* %33, i64 %4)
  %columns7.pre = load i32, i32* %1, align 4
  %rows8.pre = load i32, i32* %3, align 4
  br label %exit

exit:                                             ; preds = %y_exit, %true_entry
  %rows8 = phi i32 [ %rows2, %y_exit ], [ %rows8.pre, %true_entry ]
  %columns7 = phi i32 [ %columns4, %y_exit ], [ %columns7.pre, %true_entry ]
  %34 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns7, i32 %rows8, i32 1, i8* null)
  %35 = zext i32 %rows8 to i64
  %36 = alloca { %f32XY*, %u8XY*, %f32X* }, align 8
  %37 = bitcast { %f32XY*, %u8XY*, %f32X* }* %36 to %u0CXYT**
  store %u0CXYT* %34, %u0CXYT** %37, align 8
  %38 = getelementptr inbounds { %f32XY*, %u8XY*, %f32X* }, { %f32XY*, %u8XY*, %f32X* }* %36, i64 0, i32 1
  store %u8XY* %0, %u8XY** %38, align 8
  %39 = getelementptr inbounds { %f32XY*, %u8XY*, %f32X* }, { %f32XY*, %u8XY*, %f32X* }* %36, i64 0, i32 2
  %40 = bitcast %f32X** %39 to %u0CXYT**
  store %u0CXYT* %2, %u0CXYT** %40, align 8
  %41 = bitcast { %f32XY*, %u8XY*, %f32X* }* %36 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32XY*, %u8XY*, %f32X* }*, i64, i64)* @covariance_tmp_thunk2 to i8*), i8* %41, i64 %35)
  %42 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns7, i32 %columns7, i32 1, i8* null)
  %dst = bitcast %u0CXYT* %42 to %f32XY*
  %43 = zext i32 %columns7 to i64
  %44 = alloca { %f32XY*, %f32XY*, i32 }, align 8
  %45 = bitcast { %f32XY*, %f32XY*, i32 }* %44 to %u0CXYT**
  store %u0CXYT* %42, %u0CXYT** %45, align 8
  %46 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %44, i64 0, i32 1
  %47 = bitcast %f32XY** %46 to %u0CXYT**
  store %u0CXYT* %34, %u0CXYT** %47, align 8
  %48 = getelementptr inbounds { %f32XY*, %f32XY*, i32 }, { %f32XY*, %f32XY*, i32 }* %44, i64 0, i32 2
  store i32 %rows8, i32* %48, align 8
  %49 = bitcast { %f32XY*, %f32XY*, i32 }* %44 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32XY*, %f32XY*, i32 }*, i64, i64)* @covariance_tmp_thunk3 to i8*), i8* %49, i64 %43)
  %50 = bitcast %u0CXYT* %2 to i8*
  call void @likely_release_mat(i8* %50)
  %51 = bitcast %u0CXYT* %34 to i8*
  call void @likely_release_mat(i8* %51)
  ret %f32XY* %dst
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = distinct !{!0}
!1 = distinct !{!1}
!2 = !{i32 1, i32 -1}
!3 = distinct !{!3}
!4 = distinct !{!4}
