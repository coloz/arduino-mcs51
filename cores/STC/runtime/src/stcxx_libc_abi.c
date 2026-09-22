#if defined(STCXX_CPP_CORE) && STCXX_CPP_CORE

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if !defined(__SDCC_mcs51) && !defined(__SDCC_mcs251)
# error "stcxx_libc_abi.c requires the SDCC MCS51 or MCS251 target"
#endif

#if !defined(__SDCC_STACK_AUTO)
# error "the C++ libc ABI wrappers require SDCC --stack-auto"
#endif

/*
 * These assertions describe the ABI boundary, rather than merely documenting
 * it.  Classic MCS51 returns malloc-family values as two-byte __xdata
 * pointers, but Clang-generated C++ expects a three-byte generic pointer.
 * MCS251 uses a flat three-byte representation for both pointer classes.
 */
typedef char stcxx_generic_pointer_must_be_24_bit[(sizeof(void *) == 3) ? 1 : -1];
#if defined(__SDCC_mcs51)
typedef char stcxx_xdata_pointer_must_be_16_bit[(sizeof(void __xdata *) == 2) ? 1 : -1];
#else
typedef char stcxx_xdata_pointer_must_be_24_bit[(sizeof(void __xdata *) == 3) ? 1 : -1];
#endif

void *__stcxx_libc_malloc(size_t size)
{
  void __xdata *pointer = malloc(size);
  return pointer;
}

#endif
