; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%f32CXY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #1

; Function Attrs: nounwind
define private void @convert_grayscale_tmp_thunk0({ %f32XY*, %f32CXY* }* noalias nocapture readonly, i64, i64) #0 {
entry:
  %3 = getelementptr inbounds { %f32XY*, %f32CXY* }, { %f32XY*, %f32CXY* }* %0, i64 0, i32 0
  %4 = load %f32XY*, %f32XY** %3, align 8
  %5 = getelementptr inbounds { %f32XY*, %f32CXY* }, { %f32XY*, %f32CXY* }* %0, i64 0, i32 1
  %6 = load %f32CXY*, %f32CXY** %5, align 8
  %7 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !0
  %dst_y_step = zext i32 %columns to i64
  %8 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 6, i64 0
  %9 = ptrtoint float* %8 to i64
  %10 = and i64 %9, 31
  %11 = icmp eq i64 %10, 0
  call void @llvm.assume(i1 %11)
  %12 = getelementptr inbounds %f32CXY, %f32CXY* %6, i64 0, i32 2
  %channels = load i32, i32* %12, align 4, !range !0
  %src_c = zext i32 %channels to i64
  %13 = getelementptr inbounds %f32CXY, %f32CXY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %13, align 4, !range !0
  %src_x = zext i32 %columns1 to i64
  %src_y_step = mul nuw nsw i64 %src_x, %src_c
  %14 = getelementptr inbounds %f32CXY, %f32CXY* %6, i64 0, i32 6, i64 0
  %15 = ptrtoint float* %14 to i64
  %16 = and i64 %15, 31
  %17 = icmp eq i64 %16, 0
  call void @llvm.assume(i1 %17)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %18 = mul nuw nsw i64 %src_y_step, %y
  %19 = add i64 %18, 1
  %20 = add i64 %18, 2
  %21 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %x_body, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %x_body ]
  %22 = mul nuw nsw i64 %x, %src_c
  %23 = add nuw nsw i64 %22, %18
  %24 = getelementptr %f32CXY, %f32CXY* %6, i64 0, i32 6, i64 %23
  %25 = load float, float* %24, align 4, !llvm.mem.parallel_loop_access !1
  %26 = add i64 %19, %22
  %27 = getelementptr %f32CXY, %f32CXY* %6, i64 0, i32 6, i64 %26
  %28 = load float, float* %27, align 4, !llvm.mem.parallel_loop_access !1
  %29 = add i64 %20, %22
  %30 = getelementptr %f32CXY, %f32CXY* %6, i64 0, i32 6, i64 %29
  %31 = load float, float* %30, align 4, !llvm.mem.parallel_loop_access !1
  %32 = fmul float %25, 0x3FBD2F1AA0000000
  %33 = fmul float %28, 0x3FE2C8B440000000
  %34 = fadd float %32, %33
  %35 = fmul float %31, 0x3FD322D0E0000000
  %36 = fadd float %34, %35
  %37 = add nuw nsw i64 %x, %21
  %38 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %37
  store float %36, float* %38, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !1

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %f32XY* @convert_grayscale(%f32CXY*) {
entry:
  %1 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 2
  %channels = load i32, i32* %1, align 4, !range !0
  %2 = icmp eq i32 %channels, 3
  call void @llvm.assume(i1 %2)
  %3 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 3
  %columns = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %f32CXY, %f32CXY* %0, i64 0, i32 4
  %rows = load i32, i32* %4, align 4, !range !0
  %5 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %6 = bitcast %u0CXYT* %5 to %f32XY*
  %7 = zext i32 %rows to i64
  %8 = alloca { %f32XY*, %f32CXY* }, align 8
  %9 = bitcast { %f32XY*, %f32CXY* }* %8 to %u0CXYT**
  store %u0CXYT* %5, %u0CXYT** %9, align 8
  %10 = getelementptr inbounds { %f32XY*, %f32CXY* }, { %f32XY*, %f32CXY* }* %8, i64 0, i32 1
  store %f32CXY* %0, %f32CXY** %10, align 8
  %11 = bitcast { %f32XY*, %f32CXY* }* %8 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32XY*, %f32CXY* }*, i64, i64)* @convert_grayscale_tmp_thunk0 to i8*), i8* %11, i64 %7)
  ret %f32XY* %6
}

attributes #0 = { nounwind }
attributes #1 = { nounwind readonly }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
