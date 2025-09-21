.AUTODEPEND

#		*Translator Definitions*
CC = bcc +FLCOPY.CFG
LIBPATH = C:\UTIL\BORLANDC\LIB
TLINK = tlink /x/c/P-/L$(LIBPATH)
SRCPATH = ..\..\src
SYSTEMPATH = ..\..\systems\bc31
INCLUDEPATH = $(SRCPATH);$(SYSTEMPATH);C:\UTIL\BORLANDC\INCLUDE


#		*Implicit Rules*
.c.obj:
  $(CC) -c $<

{$(SRCPATH)}.c.obj:
  $(CC) -c $<

{$(SYSTEMPATH)}.c.obj:
  $(CC) -c $<

#		*List Macros*

OBJECTS =  \
 blockdev.obj \
 fatlite.obj \
 dosformt.obj \
 flbase.obj \
 flflash.obj \
 fltl.obj \
 flparse.obj \
 flsocket.obj \
 diskonc.obj \
 docsoc.obj \
 nftllite.obj \
 reedsol.obj \
 flsystem.obj \
 flcustom.obj \
 flcopy.obj \
 fldir.obj

fldir.exe: fldir.rsp flcopy.cfg $(OBJECTS)
  $(TLINK) @fldir.rsp, fldir,fldir,cl.lib

flcopy.exe: flcopy.rsp flcopy.cfg $(OBJECTS) fldir.exe
  $(TLINK) @flcopy.rsp, flcopy,flcopy,cl.lib

#		*Compiler Configuration File*
flcopy.cfg: makefile.
  copy &&|
-ml
-v-
-O
-Oe
-Ob
-Z
-p
-K-
-vi-
-y
-w
-I$(INCLUDEPATH)
-L$(LIBPATH)
-P-
| flcopy.cfg

tlink.rsp: makefile.
  copy &&|
c0l.obj+
blockdev.obj+
fatlite.obj+
dosformt.obj+
flparse.obj+
flbase.obj+
fltl.obj+
flflash.obj+
flsocket.obj+
flsystem.obj+
flcustom.obj+
docsoc.obj+
diskonc.obj+
nftllite.obj+
reedsol.obj+
| tlink.rsp

flcopy.rsp: tlink.rsp
  copy tlink.rsp flcopy.rsp
  echo flcopy.obj >> flcopy.rsp

fldir.rsp: tlink.rsp
  copy tlink.rsp fldir.rsp
  echo fldir.obj >> fldir.rsp
