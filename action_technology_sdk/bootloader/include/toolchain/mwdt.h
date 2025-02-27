/*
 * Copyright (c) 2020 Synopsys.
 * Author: Eugeniy Paltsev <Eugeniy.Paltsev@synopsys.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_MWDT_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_MWDT_H_

#ifndef _LINKER
#if defined(_ASMLANGUAGE)

#include <toolchain/common.h>

#define FUNC_CODE()
#define FUNC_INSTR(a)

.macro section_var_mwdt, section, symbol
	.section .\&section\&.\&symbol, "aw"
	symbol :
.endm

.macro section_func_mwdt, section, symbol
	.section .\&section\&.\&symbol, "ax"
	FUNC_CODE()
	PERFOPT_ALIGN
	symbol :
	FUNC_INSTR(symbol)
.endm

.macro section_subsec_func_mwdt, section, subsection, symbol
	.section .\&section\&.\&subsection, "ax"
	PERFOPT_ALIGN
	symbol :
.endm

#define SECTION_VAR(sect, sym) section_var_mwdt sect, sym
#define SECTION_FUNC(sect, sym) section_func_mwdt sect, sym
#define SECTION_SUBSEC_FUNC(sect, subsec, sym) \
	section_subsec_func_mwdt sect, subsec, sym

.macro glbl_text_mwdt, symbol
	.globl symbol
	.type symbol, @function
.endm

.macro glbl_data_mwdt, symbol
	.globl symbol
	.type symbol, @object
.endm

.macro weak_data_mwdt, symbol
	.weak symbol
	.type symbol, @object
.endm

#define GTEXT(sym) glbl_text_mwdt sym
#define GDATA(sym) glbl_data_mwdt sym
#define WDATA(sym) weak_data_mwdt sym

#else /* defined(_ASMLANGUAGE) */

/* MWDT toolchain misses ssize_t definition which is used by Zephyr */
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
#ifdef CONFIG_64BIT
	typedef long ssize_t;
#else
	typedef int ssize_t;
#endif
#endif /* _SSIZE_T_DEFINED */

#ifdef CONFIG_NEWLIB_LIBC
  #error "ARC MWDT doesn't support building with CONFIG_NEWLIB_LIBC as it doesn't have newlib"
#endif /* CONFIG_NEWLIB_LIBC */

#ifdef CONFIG_NATIVE_APPLICATION
  #error "ARC MWDT doesn't support building Zephyr as an native application"
#endif /* CONFIG_NATIVE_APPLICATION */


#define __no_optimization __attribute__((optnone))

#include <toolchain/gcc.h>

/* Metaware toolchain has _Static_assert. However it not able to calculate
 * conditional expression in build time for some realy complex cases. ARC GNU
 * toolchain works fine in this cases, so it looks like MWDT bug. So, disable
 * BUILD_ASSERT macro until we fix that issue in MWDT toolchain.
 */
#undef BUILD_ASSERT
#define BUILD_ASSERT(EXPR, MSG...)

#define __builtin_arc_nop()	_nop()

#endif /* _ASMLANGUAGE */

#endif /* !_LINKER */
#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_MWDT_H_ */
