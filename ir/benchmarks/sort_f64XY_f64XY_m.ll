; ModuleID = 'likely'
source_filename = "likely"

%f64XY = type { i32, i32, i32, i32, i32, i32, [0 x double] }

; Function Attrs: norecurse nounwind
define private void @sort_tmp_thunk0({ %f64XY*, i32, i64, i64, i64, i64 }* noalias nocapture readonly, i64, i64) #0 {
entry:
  %3 = getelementptr inbounds { %f64XY*, i32, i64, i64, i64, i64 }, { %f64XY*, i32, i64, i64, i64, i64 }* %0, i64 0, i32 0
  %4 = load %f64XY*, %f64XY** %3, align 8
  %5 = getelementptr inbounds { %f64XY*, i32, i64, i64, i64, i64 }, { %f64XY*, i32, i64, i64, i64, i64 }* %0, i64 0, i32 1
  %6 = load i32, i32* %5, align 4
  %7 = getelementptr inbounds %f64XY, %f64XY* %4, i64 0, i32 3
  %columns = load i32, i32* %7, align 4, !range !0
  %src_y_step = zext i32 %columns to i64
  %8 = icmp eq i32 %6, 0
  br label %y_body

y_body:                                           ; preds = %exit, %entry
  %y = phi i64 [ %1, %entry ], [ %y_increment, %exit ]
  br i1 %8, label %exit, label %true_entry.lr.ph

true_entry.lr.ph:                                 ; preds = %y_body
  %9 = mul nuw nsw i64 %y, %src_y_step
  br label %true_entry

true_entry:                                       ; preds = %true_entry.lr.ph, %loop.backedge
  %10 = phi i32 [ %15, %loop.backedge ], [ 0, %true_entry.lr.ph ]
  %11 = sext i32 %10 to i64
  %12 = add nuw nsw i64 %11, %9
  %13 = getelementptr %f64XY, %f64XY* %4, i64 0, i32 6, i64 %12
  %14 = load double, double* %13, align 8, !llvm.mem.parallel_loop_access !1
  %15 = add nuw nsw i32 %10, 1
  %16 = icmp eq i32 %15, %6
  br i1 %16, label %exit4, label %true_entry3

exit:                                             ; preds = %loop.backedge, %y_body
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %2
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %exit
  ret void

true_entry3:                                      ; preds = %true_entry, %true_entry3
  %17 = phi i32 [ %26, %true_entry3 ], [ %15, %true_entry ]
  %18 = phi i32 [ %25, %true_entry3 ], [ %10, %true_entry ]
  %19 = phi double [ %24, %true_entry3 ], [ %14, %true_entry ]
  %20 = sext i32 %17 to i64
  %21 = add nuw nsw i64 %20, %9
  %22 = getelementptr %f64XY, %f64XY* %4, i64 0, i32 6, i64 %21
  %element = load double, double* %22, align 8, !llvm.mem.parallel_loop_access !1
  %23 = fcmp fast olt double %element, %19
  %24 = select i1 %23, double %element, double %19
  %25 = select i1 %23, i32 %17, i32 %18
  %26 = add nuw nsw i32 %17, 1
  %27 = icmp eq i32 %26, %6
  br i1 %27, label %exit4, label %true_entry3

exit4:                                            ; preds = %true_entry3, %true_entry
  %.lcssa = phi i32 [ %10, %true_entry ], [ %25, %true_entry3 ]
  %28 = icmp eq i32 %10, %.lcssa
  br i1 %28, label %loop.backedge, label %true_entry7

loop.backedge:                                    ; preds = %exit4, %true_entry7
  br i1 %16, label %exit, label %true_entry

true_entry7:                                      ; preds = %exit4
  %29 = sext i32 %.lcssa to i64
  %30 = add nuw nsw i64 %29, %9
  %31 = getelementptr %f64XY, %f64XY* %4, i64 0, i32 6, i64 %30
  %32 = load double, double* %31, align 8, !llvm.mem.parallel_loop_access !1
  store double %32, double* %13, align 8, !llvm.mem.parallel_loop_access !1
  store double %14, double* %31, align 8, !llvm.mem.parallel_loop_access !1
  br label %loop.backedge
}

declare void @likely_fork(i8* noalias nocapture, i8* noalias nocapture, i64)

define %f64XY* @sort(%f64XY*) {
entry:
  %1 = getelementptr inbounds %f64XY, %f64XY* %0, i64 0, i32 4
  %len = load i32, i32* %1, align 4, !range !0
  %2 = zext i32 %len to i64
  %3 = alloca { %f64XY*, i32, i64, i64, i64, i64 }, align 8
  %4 = getelementptr inbounds { %f64XY*, i32, i64, i64, i64, i64 }, { %f64XY*, i32, i64, i64, i64, i64 }* %3, i64 0, i32 0
  store %f64XY* %0, %f64XY** %4, align 8
  %5 = getelementptr inbounds { %f64XY*, i32, i64, i64, i64, i64 }, { %f64XY*, i32, i64, i64, i64, i64 }* %3, i64 0, i32 1
  store i32 %len, i32* %5, align 8
  %6 = getelementptr inbounds { %f64XY*, i32, i64, i64, i64, i64 }, { %f64XY*, i32, i64, i64, i64, i64 }* %3, i64 0, i32 2
  store i64 1, i64* %6, align 4
  %7 = getelementptr inbounds { %f64XY*, i32, i64, i64, i64, i64 }, { %f64XY*, i32, i64, i64, i64, i64 }* %3, i64 0, i32 3
  store i64 1, i64* %7, align 4
  %8 = getelementptr inbounds { %f64XY*, i32, i64, i64, i64, i64 }, { %f64XY*, i32, i64, i64, i64, i64 }* %3, i64 0, i32 4
  store i64 %2, i64* %8, align 4
  %9 = getelementptr inbounds { %f64XY*, i32, i64, i64, i64, i64 }, { %f64XY*, i32, i64, i64, i64, i64 }* %3, i64 0, i32 5
  store i64 1, i64* %9, align 4
  %10 = bitcast { %f64XY*, i32, i64, i64, i64, i64 }* %3 to i8*
  call void @likely_fork(i8* bitcast (void ({ %f64XY*, i32, i64, i64, i64, i64 }*, i64, i64)* @sort_tmp_thunk0 to i8*), i8* %10, i64 %2)
  %11 = bitcast %f64XY* %0 to i8*
  %12 = call i8* @likely_retain_mat(i8* %11)
  %13 = bitcast i8* %12 to %f64XY*
  ret %f64XY* %13
}

declare i8* @likely_retain_mat(i8* noalias nocapture)

attributes #0 = { norecurse nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
