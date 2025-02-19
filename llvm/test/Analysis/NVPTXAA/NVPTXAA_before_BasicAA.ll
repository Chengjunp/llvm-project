; RUN: opt -aa-pipeline=default -passes='require<aa>' -debug-pass-manager -disable-output -S < %s 2>&1 | FileCheck %s

; Target-specific AA should run before BasicAA to reduce compile time
target triple = "nvptx64-nvidia-cuda"

; CHECK: Running analysis: NVPTXAA on foo
; CHECK-NEXT: Running analysis: BasicAA on foo
define void @foo(){
entry:
  ret void
}
