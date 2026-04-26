/**
 * Shadowed fpscr.h — forces SSE exception masks on every setcsr call.
 * On Linux, PPC FPSCR writes translate to ldmxcsr with exception bits cleared,
 * causing SIGFPE. Windows handles these via SEH; we prevent them here instead.
 */

#pragma once

#include <cmath>

#include <rex/types.h>

#include <simde/x86/sse.h>

#ifndef _MM_DENORMALS_ZERO_MASK
#define _MM_DENORMALS_ZERO_MASK 0x0040
#endif

namespace rex::platform {

#if defined(__x86_64__) || defined(_M_X64)

struct FPSCRPlatform {
  static constexpr size_t RoundShift = 13;
  static constexpr size_t RoundMaskVal = SIMDE_MM_ROUND_MASK;
  static constexpr size_t FlushMask = SIMDE_MM_FLUSH_ZERO_MASK | _MM_DENORMALS_ZERO_MASK;
  static constexpr u32 ExceptionMask = (1 << 7) | (1 << 8) | (1 << 9) |
                                       (1 << 10) | (1 << 11) | (1 << 12);
  static constexpr size_t GuestToHost[] = {SIMDE_MM_ROUND_NEAREST, SIMDE_MM_ROUND_TOWARD_ZERO,
                                           SIMDE_MM_ROUND_UP, SIMDE_MM_ROUND_DOWN};

  static inline u32 getcsr() noexcept { return simde_mm_getcsr(); }

  // Always keep exception masks set — prevents SIGFPE from unmasked SSE exceptions
  // that PPC FPSCR writes would otherwise enable (handled via SEH on Windows).
  static inline void setcsr(u32 csr) noexcept { simde_mm_setcsr(csr | ExceptionMask); }

  static inline void InitHostExceptions(u32& csr) noexcept {
    csr |= ExceptionMask;
  }
};

#elif defined(__aarch64__) || defined(_M_ARM64)

struct FPSCRPlatform {
  static constexpr size_t RoundShift = 22;
  static constexpr size_t RoundMaskVal = 3 << RoundShift;
  static constexpr size_t FlushMask = (1 << 19) | (1 << 24);
  static constexpr size_t GuestToHost[] = {0 << RoundShift, 3 << RoundShift, 1 << RoundShift,
                                           2 << RoundShift};
  static constexpr u32 ExceptionMask = (1 << 8) | (1 << 9) | (1 << 10) |
                                       (1 << 11) | (1 << 12) | (1 << 15);

  static inline u32 getcsr() noexcept {
    u64 csr;
    __asm__ __volatile__("mrs %0, fpcr" : "=r"(csr));
    return csr;
  }

  static inline void setcsr(u32 csr) noexcept { __asm__ __volatile__("msr fpcr, %0" : : "r"(csr)); }

  static inline void InitHostExceptions(u32& csr) noexcept {
    csr &= ~ExceptionMask;
  }
};

#else
#error "Missing implementation for FPSCR."
#endif

}  // namespace rex::platform
