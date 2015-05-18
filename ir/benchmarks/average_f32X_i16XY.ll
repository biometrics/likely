; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32X = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%i16XY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f32X* @average(%i16XY*) {
entry:
  %1 = getelementptr inbounds %i16XY, %i16XY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 8480, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = zext i32 %columns to i64
  %4 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %5 = bitcast %u0CXYT* %4 to float*
  %6 = ptrtoint %u0CXYT* %4 to i64
  %7 = and i64 %6, 31
  %8 = icmp eq i64 %7, 0
  call void @llvm.assume(i1 %8)
  %scevgep1 = bitcast %u0CXYT* %4 to i8*
  %9 = shl nuw nsw i64 %3, 2
  call void @llvm.memset.p0i8.i64(i8* %scevgep1, i8 0, i64 %9, i32 4, i1 false)
  %10 = bitcast %u0CXYT* %2 to %f32X*
  %11 = getelementptr inbounds %i16XY, %i16XY* %0, i64 0, i32 4
  %rows = load i32, i32* %11, align 4, !range !0
  %12 = zext i32 %rows to i64
  %13 = getelementptr inbounds %i16XY, %i16XY* %0, i64 0, i32 6, i64 0
  %14 = ptrtoint i16* %13 to i64
  %15 = and i64 %14, 31
  %16 = icmp eq i64 %15, 0
  call void @llvm.assume(i1 %16)
  br label %y_body

y_body:                                           ; preds = %x_exit8, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit8 ]
  %17 = mul nuw nsw i64 %y, %3
  br label %x_body7

x_body7:                                          ; preds = %x_body7, %y_body
  %x9 = phi i64 [ 0, %y_body ], [ %x_increment10, %x_body7 ]
  %18 = getelementptr float, float* %5, i64 %x9
  %19 = load float, float* %18, align 4
  %20 = add nuw nsw i64 %x9, %17
  %21 = getelementptr %i16XY, %i16XY* %0, i64 0, i32 6, i64 %20
  %22 = load i16, i16* %21, align 2
  %23 = sitofp i16 %22 to float
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
  %28 = load float, float* %27, align 4, !llvm.mem.parallel_loop_access !1
  %29 = fmul fast float %28, %26
  store float %29, float* %27, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment19 = add nuw nsw i64 %x18, 1
  %x_postcondition20 = icmp eq i64 %x_increment19, %3
  br i1 %x_postcondition20, label %x_exit17, label %x_body16, !llvm.loop !1

x_exit17:                                         ; preds = %x_body16
  ret %f32X* %10
}

; Function Attrs: nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #1

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
