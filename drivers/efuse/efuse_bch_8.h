/*
 * BCH(m = 8)
 * BCH algorithm is supplied by Cheng Qian <cheng.qian@amlogic>
 *
 * Author: Bobby Yang <bo.yang@amlogic.com>
 *
 * Copyright (C) 2010 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __AML_BCH_12_H
#define __AML_BCH_12_H

#ifdef __DEBUG
#define __D(fmt, args...) fprintf(stderr, "debug: " fmt, ## args)
#else
#define __D(fmt, args...)
#endif

#ifdef __ERROR
#define __E(fmt, args...) fprintf(stderr, "error: " fmt, ## args)
#else
#define __E(fmt, args...)
#endif



#define BCH_T           1
#define BCH_M           8
#define BCH_P_BYTES     30

void efuse_bch_enc(const char *ibuf, int isize, char *obuf, int reverse);
void efuse_bch_dec(const char *ibuf, int isize, char *obuf, int reverse);

#endif



