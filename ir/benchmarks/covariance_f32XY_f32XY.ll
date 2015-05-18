; ModuleID = 'likely'

%u0CXYT = type { i32, i32, i32, i32, i32, i32, [0 x i8] }
%f32XY = type { i32, i32, i32, i32, i32, i32, [0 x float] }

; Function Attrs: nounwind readonly
declare noalias %u0CXYT* @likely_new(i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i32 zeroext, i8* noalias nocapture) #0

; Function Attrs: nounwind
declare void @llvm.assume(i1) #1

define %f32XY* @covariance(%f32XY*) {
entry:
  %1 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 3
  %columns = load i32, i32* %1, align 4, !range !0
  %2 = call %u0CXYT* @likely_new(i32 8480, i32 1, i32 %columns, i32 1, i32 1, i8* null)
  %3 = zext i32 %columns to i64
  %4 = getelementptr inbounds %u0CXYT, %u0CXYT* %2, i64 1
  %5 = bitcast %u0CXYT* %4 to float*
  %6 = ptrtoint %u0CXYT* %4 to i64
  %7 = and i64 %6, 31
  %8 = icmp eq i64 %7, 0
  call void @llvm.assume(i1 %8)
  br label %x_body

x_body:                                           ; preds = %x_body, %entry
  %x = phi i64 [ 0, %entry ], [ %x_increment, %x_body ]
  %9 = getelementptr float, float* %5, i64 %x
  store float 0.000000e+00, float* %9, align 4, !llvm.mem.parallel_loop_access !1
  %x_increment = add nuw nsw i64 %x, 1
  %x_postcondition = icmp eq i64 %x_increment, %3
  br i1 %x_postcondition, label %x_exit, label %x_body, !llvm.loop !1

x_exit:                                           ; preds = %x_body
  %10 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 4
  %rows = load i32, i32* %10, align 4, !range !0
  %11 = zext i32 %rows to i64
  %12 = getelementptr inbounds %f32XY, %f32XY* %0, i64 0, i32 6, i64 0
  %13 = ptrtoint float* %12 to i64
  %14 = and i64 %13, 31
  %15 = icmp eq i64 %14, 0
  call void @llvm.assume(i1 %15)
  br label %y_body

y_body:                                           ; preds = %x_exit8, %x_exit
  %y = phi i64 [ 0, %x_exit ], [ %y_increment, %x_exit8 ]
  %16 = mul nuw nsw i64 %y, %3
  br label %x_body7

x_body7:                                          ; preds = %x_body7, %y_body
  %x9 = phi i64 [ 0, %y_body ], [ %x_increment10, %x_body7 ]
  %17 = getelementptr float, float* %5, i64 %x9
  %18 = load float, float* %17, align 4
  %19 = add nuw nsw i64 %x9, %16
  %20 = getelementptr %f32XY, %f32XY* %0, i64 0, i32 6, i64 %19
  %21 = load float, float* %20, align 4
  %22 = fadd fast float %21, %18
  store float %22, float* %17, align 4
  %x_increment10 = add nuw nsw i64 %x9, 1
  %x_postcondition11 = icmp eq i64 %x_increment10, %3
  br i1 %x_postcondition11, label %x_exit8, label %x_body7

x_exit8:                                          ; preds = %x_body7
  %y_increment = add nuw nsw i64 %y, 1
  %y_postcondition = icmp eq i64 %y_increment, %11
  br i1 %y_postcondition, label %y_exit, label %y_body

y_exit:                                           ; preds = %x_exit8
  %23 = uitofp i32 %rows to float
  %24 = fdiv fast float 1.000000e+00, %23
  br label %x_body16

x_body16:                                         ; preds = %x_body16, %y_exit
  %x18 = phi i64 [ 0, %y_exit ], [ %x_increment19, %x_body16 ]
  %25 = getelementptr float, float* %5, i64 %x18
  %26 = load float, float* %25, align 4, !llvm.mem.parallel_loop_access !2
  %27 = fmul fast float %26, %24
  store float %27, float* %25, align 4, !llvm.mem.parallel_loop_access !2
  %x_increment19 = add nuw nsw i64 %x18, 1
  %x_postcondition20 = icmp eq i64 %x_increment19, %3
  br i1 %x_postcondition20, label %x_exit17, label %x_body16, !llvm.loop !2

x_exit17:                                         ; preds = %x_body16
  %28 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %rows, i32 1, i8* null)
  %29 = getelementptr inbounds %u0CXYT, %u0CXYT* %28, i64 1
  %30 = bitcast %u0CXYT* %29 to float*
  %31 = ptrtoint %u0CXYT* %29 to i64
  %32 = and i64 %31, 31
  %33 = icmp eq i64 %32, 0
  call void @llvm.assume(i1 %33)
  br label %y_body32

y_body32:                                         ; preds = %x_exit36, %x_exit17
  %y34 = phi i64 [ 0, %x_exit17 ], [ %y_increment40, %x_exit36 ]
  %34 = mul nuw nsw i64 %y34, %3
  br label %x_body35

x_body35:                                         ; preds = %x_body35, %y_body32
  %x37 = phi i64 [ 0, %y_body32 ], [ %x_increment38, %x_body35 ]
  %35 = add nuw nsw i64 %x37, %34
  %36 = getelementptr %f32XY, %f32XY* %0, i64 0, i32 6, i64 %35
  %37 = load float, float* %36, align 4, !llvm.mem.parallel_loop_access !3
  %38 = getelementptr float, float* %5, i64 %x37
  %39 = load float, float* %38, align 4, !llvm.mem.parallel_loop_access !3
  %40 = fsub fast float %37, %39
  %41 = getelementptr float, float* %30, i64 %35
  store float %40, float* %41, align 4, !llvm.mem.parallel_loop_access !3
  %x_increment38 = add nuw nsw i64 %x37, 1
  %x_postcondition39 = icmp eq i64 %x_increment38, %3
  br i1 %x_postcondition39, label %x_exit36, label %x_body35, !llvm.loop !3

x_exit36:                                         ; preds = %x_body35
  %y_increment40 = add nuw nsw i64 %y34, 1
  %y_postcondition41 = icmp eq i64 %y_increment40, %11
  br i1 %y_postcondition41, label %y_exit33, label %y_body32

y_exit33:                                         ; preds = %x_exit36
  %42 = call %u0CXYT* @likely_new(i32 24864, i32 1, i32 %columns, i32 %columns, i32 1, i8* null)
  %43 = getelementptr inbounds %u0CXYT, %u0CXYT* %42, i64 1
  %44 = bitcast %u0CXYT* %43 to float*
  %45 = ptrtoint %u0CXYT* %43 to i64
  %46 = and i64 %45, 31
  %47 = icmp eq i64 %46, 0
  call void @llvm.assume(i1 %47)
  br label %y_body53

y_body53:                                         ; preds = %x_exit57, %y_exit33
  %y55 = phi i64 [ 0, %y_exit33 ], [ %y_increment63, %x_exit57 ]
  %48 = mul nuw nsw i64 %y55, %3
  br label %x_body56

x_body56:                                         ; preds = %exit, %y_body53
  %x58 = phi i64 [ 0, %y_body53 ], [ %x_increment61, %exit ]
  %49 = icmp ugt i64 %y55, %x58
  br i1 %49, label %exit, label %true_enry59

exit:                                             ; preds = %x_body56, %exit60
  %x_increment61 = add nuw nsw i64 %x58, 1
  %x_postcondition62 = icmp eq i64 %x_increment61, %3
  br i1 %x_postcondition62, label %x_exit57, label %x_body56, !llvm.loop !4

x_exit57:                                         ; preds = %exit
  %y_increment63 = add nuw nsw i64 %y55, 1
  %y_postcondition64 = icmp eq i64 %y_increment63, %3
  br i1 %y_postcondition64, label %y_exit54, label %y_body53

y_exit54:                                         ; preds = %x_exit57
  %50 = bitcast %u0CXYT* %42 to %f32XY*
  %51 = bitcast %u0CXYT* %2 to i8*
  call void @likely_release_mat(i8* %51)
  %52 = bitcast %u0CXYT* %28 to i8*
  call void @likely_release_mat(i8* %52)
  ret %f32XY* %50

true_enry59:                                      ; preds = %x_body56, %true_enry59
  %53 = phi i32 [ %67, %true_enry59 ], [ 0, %x_body56 ]
  %54 = phi double [ %66, %true_enry59 ], [ 0.000000e+00, %x_body56 ]
  %55 = sext i32 %53 to i64
  %56 = mul nuw nsw i64 %55, %3
  %57 = add nuw nsw i64 %56, %x58
  %58 = getelementptr float, float* %30, i64 %57
  %59 = load float, float* %58, align 4, !llvm.mem.parallel_loop_access !4
  %60 = fpext float %59 to double
  %61 = add nuw nsw i64 %56, %y55
  %62 = getelementptr float, float* %30, i64 %61
  %63 = load float, float* %62, align 4, !llvm.mem.parallel_loop_access !4
  %64 = fpext float %63 to double
  %65 = fmul fast double %64, %60
  %66 = fadd fast double %65, %54
  %67 = add nuw nsw i32 %53, 1
  %68 = icmp eq i32 %67, %rows
  br i1 %68, label %exit60, label %true_enry59

exit60:                                           ; preds = %true_enry59
  %69 = fptrunc double %66 to float
  %70 = add nuw nsw i64 %x58, %48
  %71 = getelementptr float, float* %44, i64 %70
  store float %69, float* %71, align 4, !llvm.mem.parallel_loop_access !4
  %72 = mul nuw nsw i64 %x58, %3
  %73 = add nuw nsw i64 %72, %y55
  %74 = getelementptr float, float* %44, i64 %73
  store float %69, float* %74, align 4, !llvm.mem.parallel_loop_access !4
  br label %exit
}

declare void @likely_release_mat(i8* noalias nocapture)

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }

!0 = !{i32 1, i32 -1}
!1 = distinct !{!1}
!2 = distinct !{!2}
!3 = distinct !{!3}
!4 = distinct !{!4}
