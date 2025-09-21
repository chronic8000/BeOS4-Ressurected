
SRC_DIR:=src/servers/app/bitstream

# 4demo_srcs
SRCS := \
	4demo/fs_io.c \
	4demo/bitmap.c \
	4demo/outline.cpp \
	4demo/be_glue.c \
	4demo/msl.c \
	4demo/frontend.c \
	4demo/cmt.c \
	4demo/cmtglob.c \
	4demo/msst.c \
	4demo/msstglob.c \
	4demo/cspglue.c

# 4test_srcs
# SRCS += \
# 	4test/4test.c \
# 	4test/frontend.c \
# 	4test/cmt.c \
# 	4test/cmtglob.c \
# 	4test/msst.c \
# 	4test/msstglob.c \
# 	4test/msl.c \
# 	4test/cspglue.c

# txtdemo_srcs
# SRCS += \
# 	txtdemo/txtdemo.c \
# 	txtdemo/frontend.c \
# 	txtdemo/cmt.c \
# 	txtdemo/cmtglob.c \
# 	txtdemo/msst.c \
# 	txtdemo/msstglob.c \
# 	txtdemo/msl.c \
# 	txtdemo/cspglue.c

# wfdemo_srcs
# SRCS += \
# 	wfdemo/wfdemo.c \
# 	wfdemo/frontend.c \
# 	wfdemo/cmt.c \
# 	wfdemo/cmtglob.c \
# 	wfdemo/msst.c \
# 	wfdemo/msstglob.c \
# 	wfdemo/msl.c \
# 	wfdemo/cspglue.c

# fdump_srcs
# SRCS += \
# 	fdump/fdump.c \
# 	fdump/frontend.c \
# 	fdump/cmt.c \
# 	fdump/cmtglob.c \
# 	fdump/msst.c \
# 	fdump/msstglob.c \
# 	fdump/msl.c \
# 	fdump/cspglue.c

# selsym_srcs
# SRCS += \
# 	selsym/selsym.c \
# 	selsym/frontend.c \
# 	selsym/cmt.c \
# 	selsym/cmtglob.c \
# 	selsym/msst.c \
# 	selsym/msstglob.c \
# 	selsym/msl.c \
# 	selsym/cspglue.c

# tt_srcs
SRCS += \
	tt/tt_iface.c \
	tt/tt_specs.c \
	tt/fontscal.c \
	tt/fsglue.c \
	tt/fnt.c \
	tt/sfntd.c \
	tt/mapstrng.c \
	tt/newscan.c \
	tt/fixed_pt.c

# type1_srcs
SRCS += \
	type1/tr_names.c \
	type1/tr_ldfnt.c \
	type1/tr_mkchr.c \
	type1/tr_trans.c

# out_srcs
SRCS += \
	speedo/out_blk.c \
	speedo/out_scrn.c \
	speedo/out_bl2d.c \
	speedo/out_wht.c \
	speedo/out_qck.c \
	speedo/out_gray.c \
	speedo/out_util.c \
	speedo/out_outl.c 

# speedo_srcs
SRCS += \
	speedo/reset.c \
	speedo/do_char.c \
	speedo/do_trns.c \
	speedo/set_spcs.c \
	speedo/set_trns.c \
	speedo/set_keys.c \
	speedo/spm_fns.c

# csp_srcs
SRCS += \
	csp/csp_api.c \
	csp/csp_pfr.c \
	csp/csp_gps.c \
	csp/csp_stk.c \
	csp/csp_crv.c \
	csp/csp_tcb.c \
	csp/csp_bmp.c \
	csp/csp_otl.c \
	csp/csp_mem.c \
	csp/csp_utl.c \
	csp/csp_dbg.c \
	csp/csp_fbx.c \
	csp/csp_cbx.c \
	csp/csp_dir.c \
	csp/csp_emb.c \
	csp/csp_str.c \
	csp/csp_gpc.c

# hp_srcs
SRCS += \
	pcl5/hpfnt1.c \
	pcl5/hpfnt2.c \
	4demo/load_pcl.c

MY_DIRS:=
MY_DIRS_PREFIX:=

MY_CFLAGS:=-DINCL_TPS -DMAX_INTERCEPTS=2000 -DINCL_ALL_WRITERS -DINCL_LCD=0 -DINCL_PFB \
                -DHAVE_MSL2INDEX -DINCL_CACHE=0 -DPFR_ON_DISK -DINCL_COMPRESSED_PFR \
                -DINCL_UNDOCUMENTED_API

MY_ASFLAGS :=

MY_INCLUDES:= -I$(BUILDHOME)/$(SRC_DIR)/includes \
				-I$(BUILDHOME)/src/servers/app/render \
				-I$(BUILDHOME)/src/servers/app \
				-I$(BUILDHOME)/src/inc/app_server_p


include $(SDIR_TEMPLATES)/ComplexCompileTemplate.mk
