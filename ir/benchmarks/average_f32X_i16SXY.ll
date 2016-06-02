; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32SX = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%i16SXY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

define noalias %f32SX* @average(%i16SXY* nocapture readonly) {
entry:
  %1 = getelementptr inbounds %i16SXY, %i16SXY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 9504, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %i16SXY, %i16SXY* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %6 = bitcast %u0CXYT* %5 to float*
  %scevgep1 = bitcast %u0CXYT* %5 to i8*
  %7 = shl nuw nsw i64 %4, 2
  call void @llvm.memset.p0i8.i64(i8* %scevgep1, i8 0, i64 %7, i32 4, i1 false)
  %8 = bitcast %u0CXYT* %2 to %f32SX*
  %9 = zext i32 %rows to i64
  br label %y_body

y_body:                                           ; preds = %x_exit8, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit8 ]
  %10 = mul nuw nsw i64 %y, %4
  br label %x_body7

x_body7:                                          ; preds = %y_body, %x_body7
  %x9 = phi i64 [ %x_increment10, %x_body7 ], [ 0, %y_body ]
  %11 = getelementptr float, float* %6, i64 %x9
  %12 = load float, float* %11, align 4
  %13 = add nuw nsw i64 %x9, %10
  %14 = getelementptr %i16SXY, %i16SXY* %0, i64 0, i32 6, i64 %13
  %15 = load i16, i16* %14, align 2
  %16 = sitofp i16 %15 to float
  %17 = fadd fast float %16, %12
  store float %17, float* %11, align 4
  %x_increment10 = add nuw nsw i64 %x9, 1
  %x_postcondition11 = icmp eq i64 %x_increment10, %4
  br i1 %x_postcondition11, label %x_exit8, label %x_body7

x_exit8:                                          ; preds = %x_body7
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %9
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit8
  %18 = icmp eq i32 %rows, 1
  br i1 %18, label %Flow2, label %true_entry

true_entry:                                       ; preds = %y_exit
  %19 = uitofp i32 %rows to float
  %20 = fdiv fast float 1.000000e+00, %19
  br label %x_body15

Flow2:                                            ; preds = %x_body15, %y_exit
  ret %f32SX* %8

x_body15:                                         ; preds = %true_entry, %x_body15
  %x17 = phi i64 [ %x_increment18, %x_body15 ], [ 0, %true_entry ]
  %21 = getelementptr float, float* %6, i64 %x17
  %22 = load float, float* %21, align 4, !llvm.mem.parallel_loop_access !1
  %23 = fmul fast float %22, %20
  store float %23, float* %21, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment18 = add nuw nsw i64 %x17, 1
  %x_postcondition19 = icmp eq i64 %x_increment18, %4
  br i1 %x_postcondition19, label %Flow2, label %x_body15
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #0

attributes #0 = { argmemonly nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
