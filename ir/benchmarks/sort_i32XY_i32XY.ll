; ModuleID = 'likely'

%i32XY = type { i32, i32, i32, i32, i32, i32, [0 x i32] }

; Function Attrs: nounwind
declare void @llvm.assume(i1) #0

; Function Attrs: norecurse
define %i32XY* @sort(%i32XY*) #1 {
entry:
  %1 = getelementptr inbounds %i32XY, %i32XY* %0, i64 0, i32 4
  %len = load i32, i32* %1, align 4, !range !0
  %2 = zext i32 %len to i64
  %3 = getelementptr inbounds %i32XY, %i32XY* %0, i64 0, i32 3
  %columns = load i32, i32* %3, align 4, !range !0
  %src_y_step = zext i32 %columns to i64
  %4 = getelementptr inbounds %i32XY, %i32XY* %0, i64 0, i32 6, i64 0
  %5 = ptrtoint i32* %4 to i64
  %6 = and i64 %5, 31
  %7 = icmp eq i64 %6, 0
  call void @llvm.assume(i1 %7)
  br label %y_body

y_body:                                           ; preds = %exit, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %exit ]
  %8 = mul nuw nsw i64 %y, %src_y_step
  br label %true_entry

true_entry:                                       ; preds = %y_body, %loop.backedge
  %9 = phi i32 [ %14, %loop.backedge ], [ 0, %y_body ]
  %10 = sext i32 %9 to i64
  %11 = add nuw nsw i64 %10, %8
  %12 = getelementptr %i32XY, %i32XY* %0, i64 0, i32 6, i64 %11
  %13 = load i32, i32* %12, align 4, !llvm.mem.parallel_loop_access !1
  %14 = add nuw nsw i32 %9, 1
  %15 = icmp eq i32 %14, %len
  br i1 %15, label %exit5, label %true_entry4

exit:                                             ; preds = %loop.backedge
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %exit
  %16 = bitcast %i32XY* %0 to i8*
  %17 = call i8* @likely_retain_mat(i8* %16)
  %18 = bitcast i8* %17 to %i32XY*
  ret %i32XY* %18

true_entry4:                                      ; preds = %true_entry, %true_entry4
  %19 = phi i32 [ %26, %true_entry4 ], [ %14, %true_entry ]
  %20 = phi i32 [ %., %true_entry4 ], [ %9, %true_entry ]
  %21 = phi i32 [ %element., %true_entry4 ], [ %13, %true_entry ]
  %22 = sext i32 %19 to i64
  %23 = add nuw nsw i64 %22, %8
  %24 = getelementptr %i32XY, %i32XY* %0, i64 0, i32 6, i64 %23
  %element = load i32, i32* %24, align 4, !llvm.mem.parallel_loop_access !1
  %25 = icmp slt i32 %element, %21
  %element. = select i1 %25, i32 %element, i32 %21
  %. = select i1 %25, i32 %19, i32 %20
  %26 = add nuw nsw i32 %19, 1
  %27 = icmp eq i32 %26, %len
  br i1 %27, label %exit5, label %true_entry4

exit5:                                            ; preds = %true_entry4, %true_entry
  %.lcssa = phi i32 [ %9, %true_entry ], [ %., %true_entry4 ]
  %28 = icmp eq i32 %9, %.lcssa
  br i1 %28, label %loop.backedge, label %true_entry8

loop.backedge:                                    ; preds = %exit5, %true_entry8
  br i1 %15, label %exit, label %true_entry

true_entry8:                                      ; preds = %exit5
  %29 = sext i32 %.lcssa to i64
  %30 = add nuw nsw i64 %29, %8
  %31 = getelementptr %i32XY, %i32XY* %0, i64 0, i32 6, i64 %30
  %32 = load i32, i32* %31, align 4, !llvm.mem.parallel_loop_access !1
  store i32 %32, i32* %12, align 4, !llvm.mem.parallel_loop_access !1
  store i32 %13, i32* %31, align 4, !llvm.mem.parallel_loop_access !1
  br label %loop.backedge
}

declare i8* @likely_retain_mat(i8* noalias nocapture)

attributes #0 = { nounwind }
attributes #1 = { norecurse }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
