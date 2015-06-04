; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }
%u8XY = type { i32, i32, i32, i32, i32, i32, [0 x i8] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f32XY* @covariance(%u8XY*) {
entry:
  %1 = getelementptr inbounds %u8XY, %u8XY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 8480, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = getelementptr inbounds %u8XY, %u8XY* %0, i64 0, i32 4
  %rows = load i32, i32* %3, align 4, !range !0
  %4 = zext i32 %columns to i64
  %5 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %6 = bitcast %u0CXYT* %5 to float*
  %7 = ptrtoint %u0CXYT* %5 to i64
  %8 = and i64 %7, 31
  %9 = icmp eq i64 %8, 0
  call void @llvm.assume(i1 %9)
  %scevgep1 = bitcast %u0CXYT* %5 to i8*
  %10 = shl nuw nsw i64 %4, 2
  call void @llvm.memset.p0i8.i64(i8* %scevgep1, i8 0, i64 %10, i32 4, i1 false)
  %11 = zext i32 %rows to i64
  %12 = getelementptr inbounds %u8XY, %u8XY* %0, i64 0, i32 6, i64 0
  %13 = ptrtoint i8* %12 to i64
  %14 = and i64 %13, 31
  %15 = icmp eq i64 %14, 0
  call void @llvm.assume(i1 %15)
  br label %y_body

y_body:                                           ; preds = %x_exit8, %entry
  %y = phi i64 [ 0, %entry ], [ %y_increment, %x_exit8 ]
  %16 = mul nuw nsw i64 %y, %4
  br label %x_body7

x_body7:                                          ; preds = %y_body, %x_body7
  %x9 = phi i64 [ %x_increment10, %x_body7 ], [ 0, %y_body ]
  %17 = getelementptr float, float* %6, i64 %x9
  %18 = load float, float* %17, align 4
  %19 = add nuw nsw i64 %x9, %16
  %20 = getelementptr %u8XY, %u8XY* %0, i64 0, i32 6, i64 %19
  %21 = load i8, i8* %20, align 1
  %22 = uitofp i8 %21 to float
  %23 = fadd fast float %22, %18
  store float %23, float* %17, align 4
  %x_increment10 = add nuw nsw i64 %x9, 1
  %x_postcondition11 = icmp eq i64 %x_increment10, %4
  br i1 %x_postcondition11, label %x_exit8, label %x_body7

x_exit8:                                          ; preds = %x_body7
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %11
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit8
  %24 = icmp eq i32 %rows, 1
  br i1 %24, label %Flow3, label %true_entry

true_entry:                                       ; preds = %y_exit
  %25 = uitofp i32 %rows to float
  %26 = fdiv fast float 1.000000e+00, %25
  br label %x_body15

Flow3:                                            ; preds = %x_body15, %y_exit
  %27 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %28 = getelementptr inbounds %u0CXYT, %u0CXYT* %27, i64 1
  %29 = bitcast %u0CXYT* %28 to float*
  %30 = ptrtoint %u0CXYT* %28 to i64
  %31 = and i64 %30, 31
  %32 = icmp eq i64 %31, 0
  call void @llvm.assume(i1 %32)
  br label %y_body31

x_body15:                                         ; preds = %true_entry, %x_body15
  %x17 = phi i64 [ %x_increment18, %x_body15 ], [ 0, %true_entry ]
  %33 = getelementptr float, float* %6, i64 %x17
  %34 = load float, float* %33, align 4, !llvm.mem.parallel_loop_access !1
  %35 = fmul fast float %34, %26
  store float %35, float* %33, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment18 = add nuw nsw i64 %x17, 1
  %x_postcondition19 = icmp eq i64 %x_increment18, %4
  br i1 %x_postcondition19, label %Flow3, label %x_body15

y_body31:                                         ; preds = %x_exit35, %Flow3
  %y33 = phi i64 [ 0, %Flow3 ], [ %y_increment39, %x_exit35 ]
  %36 = mul nuw nsw i64 %y33, %4
  br label %x_body34

x_body34:                                         ; preds = %y_body31, %x_body34
  %x36 = phi i64 [ %x_increment37, %x_body34 ], [ 0, %y_body31 ]
  %37 = add nuw nsw i64 %x36, %36
  %38 = getelementptr %u8XY, %u8XY* %0, i64 0, i32 6, i64 %37
  %39 = load i8, i8* %38, align 1, !llvm.mem.parallel_loop_access !2
  %40 = getelementptr float, float* %6, i64 %x36
  %41 = load float, float* %40, align 4, !llvm.mem.parallel_loop_access !2
  %42 = uitofp i8 %39 to float
  %43 = fsub fast float %42, %41
  %44 = getelementptr float, float* %29, i64 %37
  store float %43, float* %44, align 4, !llvm.mem.parallel_loop_access !2
  %x_increment37 = add nuw nsw i64 %x36, 1
  %x_postcondition38 = icmp eq i64 %x_increment37, %4
  br i1 %x_postcondition38, label %x_exit35, label %x_body34

x_exit35:                                         ; preds = %x_body34
  %y_increment39 = add nuw nsw i64 %y33, 1
  %y_postcondition40 = icmp eq i64 %y_increment39, %11
  br i1 %y_postcondition40, label %y_exit32, label %y_body31

y_exit32:                                         ; preds = %x_exit35
  %45 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %46 = getelementptr inbounds %u0CXYT, %u0CXYT* %45, i64 1
  %47 = bitcast %u0CXYT* %46 to float*
  %48 = ptrtoint %u0CXYT* %46 to i64
  %49 = and i64 %48, 31
  %50 = icmp eq i64 %49, 0
  call void @llvm.assume(i1 %50)
  br label %y_body52

y_body52:                                         ; preds = %x_exit56, %y_exit32
  %y54 = phi i64 [ 0, %y_exit32 ], [ %y_increment64, %x_exit56 ]
  %51 = mul nuw nsw i64 %y54, %4
  br label %x_body55

x_body55:                                         ; preds = %y_body52, %Flow
  %x57 = phi i64 [ %x_increment62, %Flow ], [ 0, %y_body52 ]
  %52 = icmp ugt i64 %y54, %x57
  br i1 %52, label %Flow, label %true_entry60

x_exit56:                                         ; preds = %Flow
  %y_increment64 = add nuw nsw i64 %y54, 1
  %y_postcondition65 = icmp eq i64 %y_increment64, %4
  br i1 %y_postcondition65, label %y_exit53, label %y_body52

y_exit53:                                         ; preds = %x_exit56
  %dst = bitcast %u0CXYT* %45 to %f32XY*
  %53 = bitcast %u0CXYT* %2 to i8*
  call void @likely_release_mat(i8* %53)
  %54 = bitcast %u0CXYT* %27 to i8*
  call void @likely_release_mat(i8* %54)
  ret %f32XY* %dst

true_entry60:                                     ; preds = %x_body55, %true_entry60
  %55 = phi i32 [ %69, %true_entry60 ], [ 0, %x_body55 ]
  %56 = phi double [ %68, %true_entry60 ], [ 0.000000e+00, %x_body55 ]
  %57 = sext i32 %55 to i64
  %58 = mul nuw nsw i64 %57, %4
  %59 = add nuw nsw i64 %58, %x57
  %60 = getelementptr float, float* %29, i64 %59
  %61 = load float, float* %60, align 4, !llvm.mem.parallel_loop_access !3
  %62 = fpext float %61 to double
  %63 = add nuw nsw i64 %58, %y54
  %64 = getelementptr float, float* %29, i64 %63
  %65 = load float, float* %64, align 4, !llvm.mem.parallel_loop_access !3
  %66 = fpext float %65 to double
  %67 = fmul fast double %66, %62
  %68 = fadd fast double %67, %56
  %69 = add nuw nsw i32 %55, 1
  %70 = icmp eq i32 %69, %rows
  br i1 %70, label %exit61, label %true_entry60

Flow:                                             ; preds = %x_body55, %exit61
  %x_increment62 = add nuw nsw i64 %x57, 1
  %x_postcondition63 = icmp eq i64 %x_increment62, %4
  br i1 %x_postcondition63, label %x_exit56, label %x_body55

exit61:                                           ; preds = %true_entry60
  %71 = add nuw nsw i64 %x57, %51
  %72 = getelementptr float, float* %47, i64 %71
  %73 = fptrunc double %68 to float
  store float %73, float* %72, align 4, !llvm.mem.parallel_loop_access !3
  %74 = mul nuw nsw i64 %x57, %4
  %75 = add nuw nsw i64 %74, %y54
  %76 = getelementptr float, float* %47, i64 %75
  store float %73, float* %76, align 4, !llvm.mem.parallel_loop_access !3
  br label %Flow
}

; Function Attrs: nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) #1

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
