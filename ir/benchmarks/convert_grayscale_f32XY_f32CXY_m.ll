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
  %7 = getelementptr inbounds %f32CXY, %f32CXY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %7, align 4, !range !0
  %dst_y_step = zext i32 %columns1 to i64
  %8 = getelementptr inbounds %f32XY, %f32XY* %4, i64 0, i32 6, i64 0
  %9 = ptrtoint float* %8 to i64
  %10 = and i64 %9, 31
  %11 = icmp eq i64 %10, 0
  call void @llvm.assume(i1 %11)
  %12 = getelementptr inbounds %f32CXY, %f32CXY* %6, i64 0, i32 2
  %channels = load i32, i32* %12, align 4, !range !0
  %src_c = zext i32 %channels to i64
  %13 = getelementptr inbounds %f32CXY, %f32CXY* %6, i64 0, i32 6, i64 0
  %14 = ptrtoint float* %13 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  call void @llvm.assume(i1 %16)
  %17 = mul nuw nsw i64 %dst_y_step, %2
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %y_body ]
  %18 = mul nuw nsw i64 %y, %src_c
  %19 = getelementptr %f32CXY, %f32CXY* %6, i64 0, i32 6, i64 %18
  %20 = load float, float* %19, align 4, !llvm.mem.parallel_loop_access !1
  %21 = add nuw nsw i64 %18, 1
  %22 = getelementptr %f32CXY, %f32CXY* %6, i64 0, i32 6, i64 %21
  %23 = load float, float* %22, align 4, !llvm.mem.parallel_loop_access !1
  %24 = add nuw nsw i64 %18, 2
  %25 = getelementptr %f32CXY, %f32CXY* %6, i64 0, i32 6, i64 %24
  %26 = load float, float* %25, align 4, !llvm.mem.parallel_loop_access !1
  %27 = fmul fast float %20, 0x3FBD2F1AA0000000
  %28 = fmul fast float %23, 0x3FE2C8B440000000
  %29 = fadd fast float %28, %27
  %30 = fmul fast float %26, 0x3FD322D0E0000000
  %31 = fadd fast float %29, %30
  %32 = getelementptr %f32XY, %f32XY* %4, i64 0, i32 6, i64 %y
  store float %31, float* %32, align 4, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %17
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %y_body
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
  %dst = bitcast %u0CXYT* %5 to %f32XY*
  %6 = zext i32 %rows to i64
  %7 = alloca { %f32XY*, %f32CXY* }, align 8
  %8 = bitcast { %f32XY*, %f32CXY* }* %7 to %u0CXYT**
  store %u0CXYT* %5, %u0CXYT** %8, align 8
  %9 = getelementptr inbounds { %f32XY*, %f32CXY* }, { %f32XY*, %f32CXY* }* %7, i64 0, i32 1
  store %f32CXY* %0, %f32CXY** %9, align 8
  %10 = bitcast { %f32XY*, %f32CXY* }* %7 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f32XY*, %f32CXY* }*, i64, i64)* @convert_grayscale_tmp_thunk0 to i8*), i8* %10, i64 %6)
  ret %f32XY* %dst
}

attributes #0 = { nounwind }
attributes #1 = { nounwind readonly }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
