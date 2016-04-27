; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%i32XY = type { i32, i32, i32, i32, i32, i32, [0 x i32] }

; Function Attrs: argmemonly nounwind
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %i32XY* @transpose(%i32XY*) {
entry:
  %1 = getelementptr inbounds %i32XY, %i32XY* %0, i64 0, i32 4
  %rows = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %i32XY, %i32XY* %0, i64 0, i32 3
  %columns = load i32, i32* %2, align 4, !range !0
  %3 = call %u0CXYT* @likely_new(i32 25120, i32 1, i32 %rows, i32 %columns, i32 1, i8* null)
  %4 = zext i32 %columns to i64
  %dst_y_step = zext i32 %rows to i64
  %5 = getelementptr inbounds %u0CXYT, %u0CXYT* %3, i64 1, i32 0
  %6 = ptrtoint i32* %5 to i64
  %7 = and i64 %6, 31
  %8 = icmp eq i64 %7, 0
  call void @llvm.assume(i1 %8)
  %9 = getelementptr inbounds %i32XY, %i32XY* %0, i64 0, i32 6, i64 0
  %10 = ptrtoint i32* %9 to i64
  %11 = and i64 %10, 31
  %12 = icmp eq i64 %11, 0
  call void @llvm.assume(i1 %12)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit ]
  %13 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %14 = mul nuw nsw i64 %x, %4
  %15 = add nuw nsw i64 %14, %y
  %16 = getelementptr %i32XY, %i32XY* %0, i64 0, i32 6, i64 %15
  %17 = load i32, i32* %16, align 4, !llvm.mem.parallel_loop_access !1
  %18 = add nuw nsw i64 %x, %13
  %19 = getelementptr i32, i32* %5, i64 %18
  store i32 %17, i32* %19, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %4
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  %dst = bitcast %u0CXYT* %3 to %i32XY*
  ret %i32XY* %dst
}

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
