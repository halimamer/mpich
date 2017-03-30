## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2016 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##
##  Portions of this code were written by Intel Corporation.
##  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
##  to Argonne National Laboratory subject to Software Grant and Corporate
##  Contributor License Agreement dated February 8, 2012.
##
if BUILD_CH4_NETMOD_OFI

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/netmod/ofi

noinst_HEADERS     +=
mpi_core_sources   += src/mpid/ch4/netmod/ofi/func_table.c \
                      src/mpid/ch4/netmod/ofi/globals.c \
                      src/mpid/ch4/netmod/ofi/util.c
errnames_txt_files += src/mpid/ch4/netmod/ofi/errnames.txt
external_subdirs   += @ofisrcdir@
pmpi_convenience_libs += @ofilib@


include_HEADERS	+= src/mpid/ch4/netmod/ofi/ofi_send.h \
		   src/mpid/ch4/netmod/ofi/fi_list.h \
		   src/mpid/ch4/netmod/ofi/netmod_direct.h \
		   src/mpid/ch4/netmod/ofi/ofi_am_events.h \
		   src/mpid/ch4/netmod/ofi/ofi_am.h \
		   src/mpid/ch4/netmod/ofi/ofi_am_impl.h \
		   src/mpid/ch4/netmod/ofi/ofi_capability_sets.h \
		   src/mpid/ch4/netmod/ofi/ofi_coll.h \
		   src/mpid/ch4/netmod/ofi/ofi_comm.h \
		   src/mpid/ch4/netmod/ofi/ofi_control.h \
		   src/mpid/ch4/netmod/ofi/ofi_datatype.h \
		   src/mpid/ch4/netmod/ofi/ofi_events.h \
		   src/mpid/ch4/netmod/ofi/ofi_impl.h \
		   src/mpid/ch4/netmod/ofi/ofi_init.h \
		   src/mpid/ch4/netmod/ofi/ofi_iovec_util.h \
		   src/mpid/ch4/netmod/ofi/ofi_op.h \
		   src/mpid/ch4/netmod/ofi/ofi_pre.h \
		   src/mpid/ch4/netmod/ofi/ofi_probe.h \
		   src/mpid/ch4/netmod/ofi/ofi_proc.h \
		   src/mpid/ch4/netmod/ofi/ofi_progress.h \
		   src/mpid/ch4/netmod/ofi/ofi_recv.h \
		   src/mpid/ch4/netmod/ofi/ofi_rma.h \
		   src/mpid/ch4/netmod/ofi/ofi_spawn.h \
		   src/mpid/ch4/netmod/ofi/ofi_types.h \
		   src/mpid/ch4/netmod/ofi/ofi_unimpl.h \
		   src/mpid/ch4/netmod/ofi/ofi_win.h


endif
