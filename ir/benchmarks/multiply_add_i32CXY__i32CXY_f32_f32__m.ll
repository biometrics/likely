; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%i32CXY = type { i32, i32, i32, i32, i32, i32, [0 x i32] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @multiply_add_tmp_thunk0({ %i32CXY*, %i32CXY*, float, float }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %i32CXY*, %i32CXY*, float, float }, { %i32CXY*, %i32CXY*, float, float }* %0, i64 0, i32 0
  %4 = load %i32CXY*, %i32CXY** %3, align 8
  %5 = getelementptr inbounds { %i32CXY*, %i32CXY*, float, float }, { %i32CXY*, %i32CXY*, float, float }* %0, i64 0, i32 1
  %6 = load %i32CXY*, %i32CXY** %5, align 8
  %7 = getelementptr inbounds { %i32CXY*, %i32CXY*, float, float }, { %i32CXY*, %i32CXY*, float, float }* %0, i64 0, i32 2
  %8 = load float, float* %7, align 4
  %9 = getelementptr inbounds { %i32CXY*, %i32CXY*, float, float }, { %i32CXY*, %i32CXY*, float, float }* %0, i64 0, i32 3
  %10 = load float, float* %9, align 4
  %11 = getelementptr inbounds %i32CXY, %i32CXY* %6, i64 0, i32 2
  %channels1 = load i32, i32* %11, align 4, !range !0
  %dst_c = zext i32 %channels1 to i64
  %12 = getelementptr inbounds %i32CXY, %i32CXY* %6, i64 0, i32 3
  %columns2 = load i32, i32* %12, align 4, !range !0
  %dst_x = zext i32 %columns2 to i64
  %13 = getelementptr inbounds %i32CXY, %i32CXY* %4, i64 0, i32 6, i64 0
  %14 = ptrtoint i32* %13 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  call void @llvm.assume(i1 %16)
  %17 = getelementptr inbounds %i32CXY, %i32CXY* %6, i64 0, i32 6, i64 0
  %18 = ptrtoint i32* %17 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  call void @llvm.assume(i1 %20)
  %21 = mul i64 %dst_c, %2
  %22 = mul i64 %21, %dst_x
  br label %y_body

y_body:                                           ; preds = %y_body, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %y_body ]
  %23 = getelementptr %i32CXY, %i32CXY* %6, i64 0, i32 6, i64 %y
  %24 = load i32, i32* %23, align 4, !llvm.mem.parallel_loop_access !1
  %25 = sitofp i32 %24 to float
  %26 = fmul float %8, %25
  %27 = fadd float %10, %26
  %28 = getelementptr %i32CXY, %i32CXY* %4, i64 0, i32 6, i64 %y
  %29 = fcmp olt float %27, 0.000000e+00
  %30 = select i1 %29, float -5.000000e-01, float 5.000000e-01
  %31 = fadd float %27, %30
  %32 = fptosi float %31 to i32
  store i32 %32, i32* %28, align 4, !llvm.mem.parallel_loop_access !1
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %22
  br i1 %y_postcondition, label %y_exit, label %y_body, !llvm.loop !1

y_exit:                                           ; preds = %y_body
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %i32CXY* @multiply_add(%i32CXY*, float, float) {
entry:
  %3 = getelementptr inbounds %i32CXY, %i32CXY* %0, i64 0, i32 2
  %channels = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %i32CXY, %i32CXY* %0, i64 0, i32 3
  %columns = load i32, i32* %4, align 4, !range !0
  %5 = getelementptr inbounds %i32CXY, %i32CXY* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = call %u0CXYT* @likely_new(i32 29216, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %7 = bitcast %u0CXYT* %6 to %i32CXY*
  %8 = zext i32 %rows to i64
  %9 = alloca { %i32CXY*, %i32CXY*, float, float }, align 8
  %10 = bitcast { %i32CXY*, %i32CXY*, float, float }* %9 to %u0CXYT**
  store %u0CXYT* %6, %u0CXYT** %10, align 8
  %11 = getelementptr inbounds { %i32CXY*, %i32CXY*, float, float }, { %i32CXY*, %i32CXY*, float, float }* %9, i64 0, i32 1
  store %i32CXY* %0, %i32CXY** %11, align 8
  %12 = getelementptr inbounds { %i32CXY*, %i32CXY*, float, float }, { %i32CXY*, %i32CXY*, float, float }* %9, i64 0, i32 2
  store float %1, float* %12, align 8
  %13 = getelementptr inbounds { %i32CXY*, %i32CXY*, float, float }, { %i32CXY*, %i32CXY*, float, float }* %9, i64 0, i32 3
  store float %2, float* %13, align 4
  %14 = bitcast { %i32CXY*, %i32CXY*, float, float }* %9 to i8*
  call void @likely_fork(i8* bitcast (void ({ %i32CXY*, %i32CXY*, float, float }*, i64, i64)* @multiply_add_tmp_thunk0 to i8*), i8* %14, i64 %8)
  ret %i32CXY* %7
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
