; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32X = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%u8XY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f32X* @average(%u8XY*) {
entry:
  %1 = getelementptr inbounds %u8XY, %u8XY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 8480, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = zext i32 %columns to i64
  %4 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %5 = bitcast %u0CXYT* %4 to float*
  %6 = ptrtoint %u0CXYT* %4 to i64
  %7 = and i64 %6, 31
  %8 = icmp eq i64 %7, 0
  call void @llvm.assume(i1 %8)
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ 0, %entry ], [ %x_increment, %x_body ]
  %9 = getelementptr float, float* %5, i64 %x
  store float 0.000000e+00, float* %9, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %3
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !1

x_exit:                                           ; preds = %x_body
  %10 = bitcast %u0CXYT* %2 to %f32X*
  %11 = getelementptr inbounds %u8XY, %u8XY* %0, i64 0, i32 4
  %rows = load i32, i32* %11, align 4, !range !0
  %12 = zext i32 %rows to i64
  %13 = getelementptr inbounds %u8XY, %u8XY* %0, i64 0, i32 6, i64 0
  %14 = ptrtoint i8* %13 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  call void @llvm.assume(i1 %16)
  br label %y_body

y_body:                                           ; preds = %x_exit8, %x_exit
  %y = phi i64 [ 0, %x_exit ], [ %y_increment, %x_exit8 ]
  %17 = mul nuw nsw i64 %y, %3
  br label %x_body7

x_body7:                                          ; preds = %x_body7, %y_body
  %x9 = phi i64 [ 0, %y_body ], [ %x_increment10, %x_body7 ]
  %18 = getelementptr float, float* %5, i64 %x9
  %19 = load float, float* %18, align 4
  %20 = add nuw nsw i64 %x9, %17
  %21 = getelementptr %u8XY, %u8XY* %0, i64 0, i32 6, i64 %20
  %22 = load i8, i8* %21, align 1
  %23 = uitofp i8 %22 to float
  %24 = fadd fast float %23, %19
  store float %24, float* %18, align 4
  %x_increment10 = add nuw nsw i64 %x9, 1
  %x_postcondition11 = icmp eq i64 %x_increment10, %3
  br i1 %x_postcondition11, label %x_exit8, label %x_body7

x_exit8:                                          ; preds = %x_body7
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %12
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit8
  %25 = uitofp i32 %rows to float
  %26 = fdiv fast float 1.000000e+00, %25
  br label %x_body16

x_body16:                                         ; preds = %x_body16, %y_exit
  %x18 = phi i64 [ 0, %y_exit ], [ %x_increment19, %x_body16 ]
  %27 = getelementptr float, float* %5, i64 %x18
  %28 = load float, float* %27, align 4, !llvm.mem.parallel_loop_access !2
  %29 = fmul fast float %28, %26
  store float %29, float* %27, align 4, !llvm.mem.parallel_loop_access !2
  %x_increment19 = add nuw nsw i64 %x18, 1
  %x_postcondition20 = icmp eq i64 %x_increment19, %3
  br i1 %x_postcondition20, label %x_exit17, label %x_body16, !llvm.loop !2

x_exit17:                                         ; preds = %x_body16
  ret %f32X* %10
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
