; ModuleID = 'likely'

%u8SXY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: norecurse nounwind
define private void @sort_tmp_thunk0({ %u8SXY*, i32, i64, i64, i64, i64 }* noalias nocapture readonly, i64, i64) #0 {
entry:
  %3 = getelementptr inbounds { %u8SXY*, i32, i64, i64, i64, i64 }, { %u8SXY*, i32, i64, i64, i64, i64 }* %0, i64 0, i32 0
  %4 = load %u8SXY*, %u8SXY** %3, align 8
  %5 = getelementptr inbounds { %u8SXY*, i32, i64, i64, i64, i64 }, { %u8SXY*, i32, i64, i64, i64, i64 }* %0, i64 0, i32 1
  %6 = load i32, i32* %5, align 4
  %7 = getelementptr inbounds %u8SXY, %u8SXY* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !0
  %src_y_step = zext i32 %columns to i64
  %8 = getelementptr inbounds %u8SXY, %u8SXY* %4, i64 0, i32 6, i64 0
  %9 = ptrtoint i8* %8 to i64
  %10 = and i64 %9, 31
  %11 = icmp eq i64 %10, 0
  call void @llvm.assume(i1 %11)
  %12 = icmp eq i32 %6, 0
  br label %y_body

y_body:                                           ; preds = %exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %exit ]
  br i1 %12, label %exit, label %true_entry.lr.ph

true_entry.lr.ph:                                 ; preds = %y_body
  %13 = mul nuw nsw i64 %y, %src_y_step
  br label %true_entry

true_entry:                                       ; preds = %true_entry.lr.ph, %loop.backedge
  %14 = phi i32 [ %19, %loop.backedge ], [ 0, %true_entry.lr.ph ]
  %15 = sext i32 %14 to i64
  %16 = add nuw nsw i64 %15, %13
  %17 = getelementptr %u8SXY, %u8SXY* %4, i64 0, i32 6, i64 %16
  %18 = load i8, i8* %17, align 1, !llvm.mem.parallel_loop_access !1
  %19 = add nuw nsw i32 %14, 1
  %20 = icmp eq i32 %19, %6
  br i1 %20, label %exit4, label %true_entry3

exit:                                             ; preds = %loop.backedge, %y_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %exit
  ret void

true_entry3:                                      ; preds = %true_entry, %true_entry3
  %21 = phi i32 [ %28, %true_entry3 ], [ %19, %true_entry ]
  %22 = phi i32 [ %., %true_entry3 ], [ %14, %true_entry ]
  %23 = phi i8 [ %element., %true_entry3 ], [ %18, %true_entry ]
  %24 = sext i32 %21 to i64
  %25 = add nuw nsw i64 %24, %13
  %26 = getelementptr %u8SXY, %u8SXY* %4, i64 0, i32 6, i64 %25
  %element = load i8, i8* %26, align 1, !llvm.mem.parallel_loop_access !1
  %27 = icmp ult i8 %element, %23
  %element. = select i1 %27, i8 %element, i8 %23
  %. = select i1 %27, i32 %21, i32 %22
  %28 = add nuw nsw i32 %21, 1
  %29 = icmp eq i32 %28, %6
  br i1 %29, label %exit4, label %true_entry3

exit4:                                            ; preds = %true_entry3, %true_entry
  %.lcssa = phi i32 [ %14, %true_entry ], [ %., %true_entry3 ]
  %30 = icmp eq i32 %14, %.lcssa
  br i1 %30, label %loop.backedge, label %true_entry7

loop.backedge:                                    ; preds = %exit4, %true_entry7
  br i1 %20, label %exit, label %true_entry

true_entry7:                                      ; preds = %exit4
  %31 = sext i32 %.lcssa to i64
  %32 = add nuw nsw i64 %31, %13
  %33 = getelementptr %u8SXY, %u8SXY* %4, i64 0, i32 6, i64 %32
  %34 = load i8, i8* %33, align 1, !llvm.mem.parallel_loop_access !1
  store i8 %34, i8* %17, align 1, !llvm.mem.parallel_loop_access !1
  store i8 %18, i8* %33, align 1, !llvm.mem.parallel_loop_access !1
  br label %loop.backedge
}

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %u8SXY* @sort(%u8SXY*) {
entry:
  %1 = getelementptr inbounds %u8SXY, %u8SXY* %0, i64 0, i32 4
  %len = load i32, i32* %1, align 4, !range !0
  %2 = zext i32 %len to i64
  %3 = alloca { %u8SXY*, i32, i64, i64, i64, i64 }, align 8
  %4 = getelementptr inbounds { %u8SXY*, i32, i64, i64, i64, i64 }, { %u8SXY*, i32, i64, i64, i64, i64 }* %3, i64 0, i32 0
  store %u8SXY* %0, %u8SXY** %4, align 8
  %5 = getelementptr inbounds { %u8SXY*, i32, i64, i64, i64, i64 }, { %u8SXY*, i32, i64, i64, i64, i64 }* %3, i64 0, i32 1
  store i32 %len, i32* %5, align 8
  %6 = getelementptr inbounds { %u8SXY*, i32, i64, i64, i64, i64 }, { %u8SXY*, i32, i64, i64, i64, i64 }* %3, i64 0, i32 2
  store i64 1, i64* %6, align 4
  %7 = getelementptr inbounds { %u8SXY*, i32, i64, i64, i64, i64 }, { %u8SXY*, i32, i64, i64, i64, i64 }* %3, i64 0, i32 3
  store i64 1, i64* %7, align 4
  %8 = getelementptr inbounds { %u8SXY*, i32, i64, i64, i64, i64 }, { %u8SXY*, i32, i64, i64, i64, i64 }* %3, i64 0, i32 4
  store i64 %2, i64* %8, align 4
  %9 = getelementptr inbounds { %u8SXY*, i32, i64, i64, i64, i64 }, { %u8SXY*, i32, i64, i64, i64, i64 }* %3, i64 0, i32 5
  store i64 1, i64* %9, align 4
  %10 = bitcast { %u8SXY*, i32, i64, i64, i64, i64 }* %3 to i8*
  call void @likely_fork(i8* bitcast (void ({ %u8SXY*, i32, i64, i64, i64, i64 }*, i64, i64)* @sort_tmp_thunk0 to i8*), i8* %10, i64 %2)
  %11 = bitcast %u8SXY* %0 to i8*
  %12 = call i8* @likely_retain_mat(i8* %11)
  %13 = bitcast i8* %12 to %u8SXY*
  ret %u8SXY* %13
}

declare i8* @likely_retain_mat(i8* noalias nocapture)

attributes #0 = { norecurse nounwind }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
