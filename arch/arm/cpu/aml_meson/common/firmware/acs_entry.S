/*
 * Amlogic Configurable SPL
 * entry point
 * @author xiaobo.gu
 */

#include <config.h>

#ifdef CONFIG_ACS
.globl _acs_start
_acs_start:
  .word __acs_set
#endif
