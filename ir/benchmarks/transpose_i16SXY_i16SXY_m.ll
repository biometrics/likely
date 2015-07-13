; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%i16SXY = type { i32, i32, i32, i32, i32, i32, [0 x i16] }

; Function Attrs: nounwind argmemonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @transpose_tmp_thunk0({ %i16SXY*, %i16SXY* }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %i16SXY*, %i16SXY* }, { %i16SXY*, %i16SXY* }* %0, i64 0, i32 0
  %4 = load %i16SXY*, %i16SXY** %3, align 8
  %5 = getelementptr inbounds { %i16SXY*, %i16SXY* }, { %i16SXY*, %i16SXY* }* %0, i64 0, i32 1
  %6 = load %i16SXY*, %i16SXY** %5, align 8
  %7 = getelementptr inbounds %i16SXY, %i16SXY* %6, i64 0, i32 3
  %columns1 = load i32, i32* %7, align 4, !range !0
  %dst_y_step = zext i32 %columns1 to i64
  %8 = getelementptr inbounds %i16SXY, %i16SXY* %4, i64 0, i32 6, i64 0
  %9 = ptrtoint i16* %8 to i64
  %10 = and i64 %9, 31
  %11 = icmp eq i64 %10, 0
  call void @llvm.assume(i1 %11)
  %12 = getelementptr inbounds %i16SXY, %i16SXY* %6, i64 0, i32 6, i64 0
  %13 = ptrtoint i16* %12 to i64
  %14 = and i64 %13, 31
  %15 = icmp eq i64 %14, 0
  call void @llvm.assume(i1 %15)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %16 = mul nuw nsw i64 %y, %dst_y_step
  br label %x_body

x_body:                                           ; preds = %y_body, %x_body
  %x = phi i64 [ %x_increment, %x_body ], [ 0, %y_body ]
  %17 = mul nuw nsw i64 %x, %dst_y_step
  %18 = add nuw nsw i64 %17, %y
  %19 = getelementptr %i16SXY, %i16SXY* %6, i64 0, i32 6, i64 %18
  %20 = load i16, i16* %19, align 2, !llvm.mem.parallel_loop_access !1
  %21 = add nuw nsw i64 %x, %16
  %22 = getelementptr %i16SXY, %i16SXY* %4, i64 0, i32 6, i64 %21
  store i16 %20, i16* %22, align 2, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %dst_y_step
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %x_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %i16SXY* @transpose(%i16SXY*) {
entry:
  %1 = getelementptr inbounds %i16SXY, %i16SXY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = getelementptr inbounds %i16SXY, %i16SXY* %0, i64 0, i32 4
  %rows = load i32, i32* %2, align 4, !range !0
  %3 = call %u0CXYT* @likely_new(i32 26128, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %dst = bitcast %u0CXYT* %3 to %i16SXY*
  %4 = zext i32 %rows to i64
  %5 = alloca { %i16SXY*, %i16SXY* }, align 8
  %6 = bitcast { %i16SXY*, %i16SXY* }* %5 to %u0CXYT**
  store %u0CXYT* %3, %u0CXYT** %6, align 8
  %7 = getelementptr inbounds { %i16SXY*, %i16SXY* }, { %i16SXY*, %i16SXY* }* %5, i64 0, i32 1
  store %i16SXY* %0, %i16SXY** %7, align 8
  %8 = bitcast { %i16SXY*, %i16SXY* }* %5 to i8*
  call void @likely_fork(i8* bitcast (void ({ %i16SXY*, %i16SXY* }*, i64, i64)* @transpose_tmp_thunk0 to i8*), i8* %8, i64 %4)
  ret %i16SXY* %dst
}

attributes #0 = { nounwind argmemonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
