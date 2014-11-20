/*
 * aml6236-m nand spl
 * @author jerry.yu
 *
 */
/* These define will be replace with */
#include <config.h>
#include <version.h>
#include <asm/arch/romboot.h>

.global __magic_word
__magic_word:
  .word MAGIC_WORD1
  .word MAGIC_WORD2
  .word 0
  .word 0
  .word 0xfe0b0101
  .word 0x3ec1e07f
  .word 0xa9e00000
  .word 0x00000075
  .space 0x1fe-0x1b0 - (. - __magic_word)
  .short 0xaa55
.global __end_magic
__end_magic:
