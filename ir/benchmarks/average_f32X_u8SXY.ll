; ModuleID = 'likely'
source_filename = "likely"

%u0Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32Matrix = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%u8Matrix = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: argmemonly nounwind
declare noalias %u0Matrix* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define noalias %f32Matrix* @average(%u8Matrix* noalias nocapture readonly) #1 {
entry:
  br label %entry.split

entry.split:                                      ; preds = %entry
  %1 = getelementptr inbounds %u8Matrix, %u8Matrix* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0Matrix* @likely_new(i32 9504, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %u8Matrix, %u8Matrix* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0Matrix, %u0Matrix* %2, i64 1
  %6 = bitcast %u0Matrix* %5 to i8*
  %7 = bitcast %u0Matrix* %5 to float*
  %8 = shl nuw nsw i64 %4, 2
  call void @llvm.memset.p0i8.i64(i8* %6, i8 0, i64 %8, i32 4, i1 false)
  %9 = bitcast %u0Matrix* %2 to %f32Matrix*
  %10 = zext i32 %rows to i64
  br label %y_body

y_body:                                           ; preds = %x_exit8, %entry.split
  %y = phi i64 [ 0, %entry.split ], [ %y_increment, %x_exit8 ]
  %11 = mul nuw nsw i64 %y, %4
  br label %x_body7

x_body7:                                          ; preds = %y_body, %x_body7
  %x9 = phi i64 [ %x_increment10, %x_body7 ], [ 0, %y_body ]
  %12 = getelementptr float, float* %7, i64 %x9
  %13 = load float, float* %12, align 4
  %14 = add nuw nsw i64 %x9, %11
  %15 = getelementptr %u8Matrix, %u8Matrix* %0, i64 0, i32 6, i64 %14
  %16 = load i8, i8* %15, align 1
  %17 = uitofp i8 %16 to float
  %18 = fadd fast float %17, %13
  store float %18, float* %12, align 4
  %x_increment10 = add nuw nsw i64 %x9, 1
  %x_postcondition11 = icmp eq i64 %x_increment10, %4
  br i1 %x_postcondition11, label %x_exit8, label %x_body7

x_exit8:                                          ; preds = %x_body7
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %10
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit8
  %19 = icmp eq i32 %rows, 1
  br i1 %19, label %Flow1, label %true_entry

true_entry:                                       ; preds = %y_exit
  %20 = uitofp i32 %rows to float
  %21 = fdiv fast float 1.000000e+00, %20
  br label %x_body15

Flow1:                                            ; preds = %x_body15, %y_exit
  ret %f32Matrix* %9

x_body15:                                         ; preds = %true_entry, %x_body15
  %x17 = phi i64 [ %x_increment18, %x_body15 ], [ 0, %true_entry ]
  %22 = getelementptr float, float* %7, i64 %x17
  %23 = load float, float* %22, align 4, !llvm.mem.parallel_loop_access !1
  %24 = fmul fast float %23, %21
  store float %24, float* %22, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment18 = add nuw nsw i64 %x17, 1
  %x_postcondition19 = icmp eq i64 %x_increment18, %4
  br i1 %x_postcondition19, label %Flow1, label %x_body15
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #0

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
