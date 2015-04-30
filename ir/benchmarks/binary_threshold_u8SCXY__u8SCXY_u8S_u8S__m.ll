; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%u8SCXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
define private void @binary_threshold_tmp_thunk0({ %u8SCXY*, %u8SCXY*, i8, i8 }* noalias nocapture readonly, i64, i64) #1 {
entry:
  %3 = getelementptr inbounds { %u8SCXY*, %u8SCXY*, i8, i8 }, { %u8SCXY*, %u8SCXY*, i8, i8 }* %0, i64 0, i32 0
  %4 = load %u8SCXY*, %u8SCXY** %3, align 8
  %5 = getelementptr inbounds { %u8SCXY*, %u8SCXY*, i8, i8 }, { %u8SCXY*, %u8SCXY*, i8, i8 }* %0, i64 0, i32 1
  %6 = load %u8SCXY*, %u8SCXY** %5, align 8
  %7 = getelementptr { %u8SCXY*, %u8SCXY*, i8, i8 }, { %u8SCXY*, %u8SCXY*, i8, i8 }* %0, i64 0, i32 2
  %8 = bitcast i8* %7 to i16*
  %.combined = load i16, i16* %8, align 1
  %combine.extract.trunc = trunc i16 %.combined to i8
  %combine.extract.shift = lshr i16 %.combined, 8
  %combine.extract.trunc7 = trunc i16 %combine.extract.shift to i8
  %9 = getelementptr %u8SCXY, %u8SCXY* %4, i64 0, i32 2
  %10 = bitcast i32* %9 to i64*
  %channels.combined = load i64, i64* %10, align 4
  %dst_c = and i64 %channels.combined, 4294967295
  %combine.extract.shift12 = lshr i64 %channels.combined, 32
  %11 = getelementptr inbounds %u8SCXY, %u8SCXY* %4, i64 0, i32 6, i64 0
  %12 = ptrtoint i8* %11 to i64
  %13 = and i64 %12, 31
  %14 = icmp eq i64 %13, 0
  call void @llvm.assume(i1 %14)
  %15 = getelementptr %u8SCXY, %u8SCXY* %6, i64 0, i32 2
  %16 = bitcast i32* %15 to i64*
  %channels1.combined = load i64, i64* %16, align 4
  %src_c = and i64 %channels1.combined, 4294967295
  %combine.extract.shift9 = lshr i64 %channels1.combined, 32
  %17 = getelementptr inbounds %u8SCXY, %u8SCXY* %6, i64 0, i32 6, i64 0
  %18 = ptrtoint i8* %17 to i64
  %19 = and i64 %18, 31
  %20 = icmp eq i64 %19, 0
  call void @llvm.assume(i1 %20)
  br label %y_body

y_body:                                           ; preds = %x_exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %x_exit ]
  %21 = mul i64 %y, %combine.extract.shift9
  %22 = mul i64 %y, %combine.extract.shift12
  br label %x_body

x_body:                                           ; preds = %c_exit, %y_body
  %x = phi i64 [ 0, %y_body ], [ %x_increment, %c_exit ]
  %tmp = add i64 %x, %21
  %tmp4 = mul i64 %tmp, %src_c
  %tmp5 = add i64 %x, %22
  %tmp6 = mul i64 %tmp5, %dst_c
  br label %c_body

c_body:                                           ; preds = %c_body, %x_body
  %c = phi i64 [ 0, %x_body ], [ %c_increment, %c_body ]
  %23 = add i64 %tmp4, %c
  %24 = getelementptr %u8SCXY, %u8SCXY* %6, i64 0, i32 6, i64 %23
  %25 = load i8, i8* %24, align 1, !llvm.mem.parallel_loop_access !0
  %26 = icmp ugt i8 %25, %combine.extract.trunc
  %. = select i1 %26, i8 %combine.extract.trunc7, i8 0
  %27 = add i64 %tmp6, %c
  %28 = getelementptr %u8SCXY, %u8SCXY* %4, i64 0, i32 6, i64 %27
  store i8 %., i8* %28, align 1, !llvm.mem.parallel_loop_access !0
  %c_increment = add nuw nsw i64 %c, 1
  %c_postcondition = icmp eq i64 %c_increment, %dst_c
  br i1 %c_postcondition, label %c_exit, label %c_body, !llvm.loop !0

c_exit:                                           ; preds = %c_body
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %combine.extract.shift12
  br i1 %x_postcondition, label %x_exit, label %x_body

x_exit:                                           ; preds = %c_exit
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit
  ret void
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %u8SCXY* @binary_threshold(%u8SCXY*, i8, i8) {
entry:
  %3 = getelementptr %u8SCXY, %u8SCXY* %0, i64 0, i32 2
  %4 = bitcast i32* %3 to i64*
  %channels.combined = load i64, i64* %4, align 4
  %combine.extract.trunc = trunc i64 %channels.combined to i32
  %combine.extract.shift = lshr i64 %channels.combined, 32
  %combine.extract.trunc1 = trunc i64 %combine.extract.shift to i32
  %5 = getelementptr inbounds %u8SCXY, %u8SCXY* %0, i64 0, i32 4
  %rows = load i32, i32* %5, align 4, !range !1
  %6 = call %u0CXYT* @likely_new(i32 29704, i32 %combine.extract.trunc, i32 %combine.extract.trunc1, i32 %rows, i32 1, i8* null)
  %7 = bitcast %u0CXYT* %6 to %u8SCXY*
  %8 = zext i32 %rows to i64
  %9 = alloca { %u8SCXY*, %u8SCXY*, i8, i8 }, align 8
  %10 = bitcast { %u8SCXY*, %u8SCXY*, i8, i8 }* %9 to %u0CXYT**
  store %u0CXYT* %6, %u0CXYT** %10, align 8
  %11 = getelementptr inbounds { %u8SCXY*, %u8SCXY*, i8, i8 }, { %u8SCXY*, %u8SCXY*, i8, i8 }* %9, i64 0, i32 1
  store %u8SCXY* %0, %u8SCXY** %11, align 8
  %12 = getelementptr inbounds { %u8SCXY*, %u8SCXY*, i8, i8 }, { %u8SCXY*, %u8SCXY*, i8, i8 }* %9, i64 0, i32 2
  store i8 %1, i8* %12, align 8
  %13 = getelementptr inbounds { %u8SCXY*, %u8SCXY*, i8, i8 }, { %u8SCXY*, %u8SCXY*, i8, i8 }* %9, i64 0, i32 3
  store i8 %2, i8* %13, align 1
  %14 = bitcast { %u8SCXY*, %u8SCXY*, i8, i8 }* %9 to i8*
  call void @likely_fork(i8* bitcast (void ({ %u8SCXY*, %u8SCXY*, i8, i8 }*, i64, i64)* @binary_threshold_tmp_thunk0 to i8*), i8* %14, i64 %8)
  ret %u8SCXY* %7
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = distinct !{!0}
!1 = !{i32 1, i32 -1}
