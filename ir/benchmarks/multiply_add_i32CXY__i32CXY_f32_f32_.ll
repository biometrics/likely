; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%i32CXY = type { i32, i32, i32, i32, i32, i32, [0 x i32] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

; Function Attrs: nounwind
define %i32CXY* @multiply_add(%i32CXY*, float, float) #1 {
entry:
  %3 = getelementptr inbounds %i32CXY, %i32CXY* %0, i64 0, i32 2
  %channels = load i32, i32* %3, align 4, !range !0
  %4 = getelementptr inbounds %i32CXY, %i32CXY* %0, i64 0, i32 3
  %columns = load i32, i32* %4, align 4, !range !0
  %5 = getelementptr inbounds %i32CXY, %i32CXY* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !0
  %6 = tail call %u0CXYT* @likely_new(i32 29216, i32 %channels, i32 %columns, i32 %rows, i32 1, i8* null)
  %7 = zext i32 %rows to i64
  %dst_c = zext i32 %channels to i64
  %dst_x = zext i32 %columns to i64
  %8 = getelementptr inbounds %u0CXYT, %u0CXYT* %6, i64 1, i32 0
  %9 = ptrtoint i32* %8 to i64
  %10 = and i64 %9, 31
  %11 = icmp eq i64 %10, 0
  tail call void @llvm.assume(i1 %11)
  %12 = getelementptr inbounds %i32CXY, %i32CXY* %0, i64 0, i32 6, i64 0
  %13 = ptrtoint i32* %12 to i64
  %14 = and i64 %13, 31
  %15 = icmp eq i64 %14, 0
  tail call void @llvm.assume(i1 %15)
  %16 = mul nuw i64 %dst_x, %dst_c
  br label %x_body

x_body:                                           ; preds = %entry, %x_exit
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %17 = mul i64 %y, %dst_x
  %tmp1 = mul i64 %17, %dst_c
  br label %c_body

c_body:                                           ; preds = %c_body, %x_body
  %c = phi i64 [ 0, %x_body ], [ %c_increment, %c_body ]
  %18 = add i64 %tmp1, %c
  %19 = getelementptr %i32CXY, %i32CXY* %0, i64 0, i32 6, i64 %18
  %20 = load i32, i32* %19, align 4, !llvm.mem.parallel_loop_access !1
  %21 = sitofp i32 %20 to float
  %22 = fmul float %21, %1
  %23 = fadd float %22, %2
  %24 = fcmp olt float %23, 0.000000e+00
  %25 = select i1 %24, float -5.000000e-01, float 5.000000e-01
  %26 = fadd float %23, %25
  %27 = fptosi float %26 to i32
  %28 = getelementptr i32, i32* %8, i64 %18
  store i32 %27, i32* %28, align 4, !llvm.mem.parallel_loop_access !1
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %16
  br i1 %c_postcondition, label %x_exit, label %c_body, !llvm.loop !1

x_exit:                                           ; preds = %c_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %7
  br i1 %y_postcondition, label %y_exit, label %x_body

y_exit:                                           ; preds = %x_exit
  %29 = bitcast %u0CXYT* %6 to %i32CXY*
  ret %i32CXY* %29
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
