// REQUIRES: lld-available

// Building the instrumented binary will fail because lld doesn't support
// big-endian ELF for PPC (aka ABI 1).
// ld.lld: error: /lib/../lib64/Scrt1.o: ABI version 1 is not supported
// UNSUPPORTED: ppc && host-byteorder-big-endian

// RUN: rm -rf %t && mkdir %t && cd %t

// RUN: %clangxx_pgogen -fuse-ld=lld -O2 -fprofile-generate=. -mllvm -enable-vtable-value-profiling %s -o test
// RUN: env LLVM_PROFILE_FILE=test.profraw ./test

// Show vtable profiles from raw profile.
// RUN: llvm-profdata show --function=main --ic-targets --show-vtables test.profraw | FileCheck %s --check-prefixes=COMMON,RAW

// Generate indexed profile from raw profile and show the data.
// RUN: llvm-profdata merge test.profraw -o test.profdata
// RUN: llvm-profdata show --function=main --ic-targets --show-vtables test.profdata | FileCheck %s --check-prefixes=COMMON,INDEXED

// Generate text profile from raw and indexed profiles respectively and show the data.
// RUN: llvm-profdata merge --text test.profraw -o raw.proftext
// RUN: llvm-profdata show --function=main --ic-targets --show-vtables --text raw.proftext | FileCheck %s --check-prefix=ICTEXT
// RUN: llvm-profdata merge --text test.profdata -o indexed.proftext
// RUN: llvm-profdata show --function=main --ic-targets --show-vtables --text indexed.proftext | FileCheck %s --check-prefix=ICTEXT

// Generate indexed profile from text profiles and show the data
// RUN: llvm-profdata merge --binary raw.proftext -o text.profraw
// RUN: llvm-profdata show --function=main --ic-targets --show-vtables text.profraw | FileCheck %s --check-prefixes=COMMON,INDEXED
// RUN: llvm-profdata merge --binary indexed.proftext -o text.profdata
// RUN: llvm-profdata show --function=main --ic-targets --show-vtables text.profdata | FileCheck %s --check-prefixes=COMMON,INDEXED

// COMMON: Counters:
// COMMON-NEXT:  main:
// COMMON-NEXT:  Hash: 0x068617320ec408a0
// COMMON-NEXT:  Counters: 4
// COMMON-NEXT:  Indirect Call Site Count: 2
// COMMON-NEXT:  Number of instrumented vtables: 2
// RAW:  Indirect Target Results:
// RAW-NEXT:       [  0, _ZN8Derived14funcEii,        50 ] (25.00%)
// RAW-NEXT:       [  0, {{.*}}instrprof-vtable-value-prof.cpp;_ZN12_GLOBAL__N_18Derived24funcEii,        150 ] (75.00%)
// RAW-NEXT:       [  1, _ZN8Derived1D0Ev,        250 ] (25.00%)
// RAW-NEXT:       [  1, {{.*}}instrprof-vtable-value-prof.cpp;_ZN12_GLOBAL__N_18Derived2D0Ev,        750 ] (75.00%)
// RAW-NEXT:  VTable Results:
// RAW-NEXT:       [  0, _ZTV8Derived1,        50 ] (25.00%)
// RAW-NEXT:       [  0, {{.*}}instrprof-vtable-value-prof.cpp;_ZTVN12_GLOBAL__N_18Derived2E,        150 ] (75.00%)
// RAW-NEXT:       [  1, _ZTV8Derived1,        250 ] (25.00%)
// RAW-NEXT:       [  1, {{.*}}instrprof-vtable-value-prof.cpp;_ZTVN12_GLOBAL__N_18Derived2E,        750 ] (75.00%)
// INDEXED:     Indirect Target Results:
// INDEXED-NEXT:         [  0, {{.*}}instrprof-vtable-value-prof.cpp;_ZN12_GLOBAL__N_18Derived24funcEii,        150 ] (75.00%)
// INDEXED-NEXT:         [  0, _ZN8Derived14funcEii,        50 ] (25.00%)
// INDEXED-NEXT:         [  1, {{.*}}instrprof-vtable-value-prof.cpp;_ZN12_GLOBAL__N_18Derived2D0Ev,        750 ] (75.00%)
// INDEXED-NEXT:         [  1, _ZN8Derived1D0Ev,        250 ] (25.00%)
// INDEXED-NEXT:     VTable Results:
// INDEXED-NEXT:         [  0, {{.*}}instrprof-vtable-value-prof.cpp;_ZTVN12_GLOBAL__N_18Derived2E,        150 ] (75.00%)
// INDEXED-NEXT:         [  0, _ZTV8Derived1,        50 ] (25.00%)
// INDEXED-NEXT:         [  1, {{.*}}instrprof-vtable-value-prof.cpp;_ZTVN12_GLOBAL__N_18Derived2E,        750 ] (75.00%)
// INDEXED-NEXT:         [  1, _ZTV8Derived1,        250 ] (25.00%)
// COMMON: Instrumentation level: IR  entry_first = 0
// COMMON-NEXT: Functions shown: 1
// COMMON-NEXT: Total functions: 7
// COMMON-NEXT: Maximum function count: 1000
// COMMON-NEXT: Maximum internal block count: 1000
// COMMON-NEXT: Statistics for indirect call sites profile:
// COMMON-NEXT:   Total number of sites: 2
// COMMON-NEXT:   Total number of sites with values: 2
// COMMON-NEXT:   Total number of profiled values: 4
// COMMON-NEXT:   Value sites histogram:
// COMMON-NEXT:         NumTargets, SiteCount
// COMMON-NEXT:         2, 2
// COMMON-NEXT: Statistics for vtable profile:
// COMMON-NEXT:   Total number of sites: 2
// COMMON-NEXT:   Total number of sites with values: 2
// COMMON-NEXT:   Total number of profiled values: 4
// COMMON-NEXT:   Value sites histogram:
// COMMON-NEXT:         NumTargets, SiteCount
// COMMON-NEXT:         2, 2

// ICTEXT: :ir
// ICTEXT: main
// ICTEXT: # Func Hash:
// ICTEXT: 470088714870327456
// ICTEXT: # Num Counters:
// ICTEXT: 4
// ICTEXT: # Counter Values:
// ICTEXT: 1000
// ICTEXT: 1000
// ICTEXT: 200
// ICTEXT: 1
// ICTEXT: # Num Value Kinds:
// ICTEXT: 2
// ICTEXT: # ValueKind = IPVK_IndirectCallTarget:
// ICTEXT: 0
// ICTEXT: # NumValueSites:
// ICTEXT: 2
// ICTEXT: 2
// ICTEXT: {{.*}}instrprof-vtable-value-prof.cpp;_ZN12_GLOBAL__N_18Derived24funcEii:150
// ICTEXT: _ZN8Derived14funcEii:50
// ICTEXT: 2
// ICTEXT: {{.*}}instrprof-vtable-value-prof.cpp;_ZN12_GLOBAL__N_18Derived2D0Ev:750
// ICTEXT: _ZN8Derived1D0Ev:250
// ICTEXT: # ValueKind = IPVK_VTableTarget:
// ICTEXT: 2
// ICTEXT: # NumValueSites:
// ICTEXT: 2
// ICTEXT: 2
// ICTEXT: {{.*}}instrprof-vtable-value-prof.cpp;_ZTVN12_GLOBAL__N_18Derived2E:150
// ICTEXT: _ZTV8Derived1:50
// ICTEXT: 2
// ICTEXT: {{.*}}instrprof-vtable-value-prof.cpp;_ZTVN12_GLOBAL__N_18Derived2E:750
// ICTEXT: _ZTV8Derived1:250

// Test indirect call promotion transformation using vtable profiles.
// RUN: %clangxx -fprofile-use=test.profdata -fuse-ld=lld -flto=thin -fwhole-program-vtables -O2 -mllvm -enable-vtable-value-profiling -mllvm -icp-enable-vtable-cmp -Rpass=pgo-icall-prom %s 2>&1 | FileCheck %s --check-prefix=REMARK --implicit-check-not="!VP"

// REMARK: Promote indirect call to _ZN12_GLOBAL__N_18Derived24funcEii with count 150 out of 200, compare 1 vtables and sink 1 instructions
// REMARK: Promote indirect call to _ZN8Derived14funcEii with count 50 out of 50, compare 1 vtables and sink 1 instructions
// REMARK: Promote indirect call to _ZN12_GLOBAL__N_18Derived2D0Ev with count 750 out of 1000, compare 1 vtables and sink 2 instructions
// REMARK: Promote indirect call to _ZN8Derived1D0Ev with count 250 out of 250, compare 1 vtables and sink 2 instructions

#include <cstdio>
#include <cstdlib>
class Base {
public:
  virtual int func(int a, int b) = 0;

  virtual ~Base() {};
};
class Derived1 : public Base {
public:
  int func(int a, int b) override { return a * b; }

  ~Derived1() {}
};
namespace {
class Derived2 : public Base {
public:
  int func(int a, int b) override { return a * (a - b); }

  ~Derived2() {}
};
} // namespace
__attribute__((noinline)) Base *createType(int a) {
  Base *base = nullptr;
  if (a % 4 == 0)
    base = new Derived1();
  else
    base = new Derived2();
  return base;
}
int main(int argc, char **argv) {
  int sum = 0;
  for (int i = 0; i < 1000; i++) {
    int a = rand();
    int b = rand();
    Base *ptr = createType(i);
    if (i % 5 == 0)
      sum += ptr->func(b, a);

    delete ptr;
  }
  printf("sum is %d\n", sum);
  return 0;
}
