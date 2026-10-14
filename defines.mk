ifdef RELEASEBUILD
	DEFINES += -DRELEASEBUILD
	ifeq ($(ENJ_TARGET),dreamcast)
		ENJ_LDFLAGS += -s
	else ifeq ($(ENJ_TARGET),pc-endjinn)
		ENJ_LDFLAGS += -Wl,-S
	else ifeq ($(ENJ_TARGET),web-endjinn)
		ENJ_LDFLAGS += -g0
	endif
else
	DEFINES += -g
endif

ifdef OPTLEVEL
	DEFINES += -O${OPTLEVEL}
else
	DEFINES += -O${ENJ_DEFAULT_OPTLEVEL}
	OPTLEVEL=${ENJ_DEFAULT_OPTLEVEL}
endif

ifdef ENJ_SHOWFRAMETIMES
	DEFINES += -DENJ_SHOWFRAMETIMES=${ENJ_SHOWFRAMETIMES}
endif

ifdef ENJ_FRAME_RATE
	DEFINES += -DENJ_FRAME_RATE=${ENJ_FRAME_RATE}
endif

ifdef ENJ_CBASEPATH
	DEFINES += -DENJ_CBASEPATH="\"${ENJ_CBASEPATH}\""
else
	DEFINES += -DENJ_CBASEPATH="\"/cd/${ENJ_BASENAME}/\""
endif
	
ifdef ENJ_DBG_PRINT
	DEFINES += -DENJ_DBG_PRINT
endif

ifdef ENJ_DBG_GDB
	DEFINES += -DENJ_DBG_GDB
endif

ifdef ENJ_FSAA
	DEFINES += -DENJ_FSAA=${ENJ_FSAA}
endif

ifdef ENJ_DCTRACE
DEFINES += -finstrument-functions -DENJ_p
OBJS += $(ENJ_BUILDDIR)/enDjinn/profilers/dcprofiler.o
endif 

ifdef ENJ_DCPROF
DEFINES += -DENJ_DCPROF
OBJS += $(ENJ_BUILDDIR)/enDjinn/profilers/dcprof/profiler.o
endif

ifdef ENJ_INJECT_QFONT
	DEFINES += -DENJ_INJECT_QFONT=1
endif
