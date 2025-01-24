#!/usr/bin/make -f
# file: slac_python_module/Makefile

# ====================================================================
# environment definitions;
# --------------------------------------------------------------------

include ../make.def

# ====================================================================
# symbols;
# --------------------------------------------------------------------

PYTHON_VERSION = 3.11

CFLAGS+=
LDFLAGS+=
TRASH=*.o *.obj *.exe t t.* *~ *.[0-9][0-9][0-9]
FILES=Makefile *.mak *.sh *.c *.h
TOOLS=slac_wrapper.so

# ====================================================================
# pseudo targets;
# --------------------------------------------------------------------

all compile: ${TOOLS}
compact: compile
	if [ ! -z ${STRIP} ]; then ${STRIP} ${TOOLS}; fi
	if [ -x /usr/local/bin/upx ]; then upx --best ${TOOLS}; fi
library:
	install -m ${DIR_PERM} -o ${OWNER} -g ${GROUP} -d ${BIN}
scripts:
manuals:
	if [ ! -d ${MAN} ]; then install -m ${DIR_PERM} -o ${OWNER} -g ${GROUP} -d ${MAN}; fi
	install -m ${MAN_PERM} -o ${OWNER} -g ${GROUP} ${PAGES} ${MAN}
install: compile library
	install -m ${SUID_PERM} -o ${OWNER} -g ${GROUP} ${TOOLS} ${BIN}
uninstall:
	cd ${BIN}; rm -f ${TOOLS}
	cd ${MAN}; rm -f ${PAGES}
clean:
	rm -f ${TRASH} ${TOOLS}
ignore:
	echo ${TOOLS} | tr ' ' '\n' > .gitignore

# ====================================================================
# rules;
# --------------------------------------------------------------------

%: %.o
	${CC} -o ${@} ${^} ${LDFLAGS}
%.o: %.c
	${CC} -c ${<} ${CFLAGS}
%.h: %.h

# ====================================================================
# programs;
# --------------------------------------------------------------------

slac_wrapper.so: slac_wrapper.o mod_main.o adc_wrapper.o plc_wrapper.o Devices.o EthernetHeader.o QualcommHeader.o HomePlugHeader1.o UnwantedMessage.o channel.o closechannel.o config.o decdecode.o decstring.o error.o hexdecode.o hexdump.o hexencode.o hexstring.o memincr.o initchannel.o openchannel.o pev_cm_atten_char.o pev_cm_mnbc_sound.o pev_cm_slac_match.o pev_cm_slac_param.o pev_cm_start_atten_char.o pev_cm_set_key.o readmessage.o readpacket.o sendmessage.o sendpacket.o slac_connect.o slac_session.o strfbits.o todigit.o uintspec.o version.o slac_debug.o desuid.o evse_cm_atten_char.o evse_cm_mnbc_sound.o evse_cm_slac_match.o evse_cm_slac_param.o evse_cm_start_atten_char.o evse_cm_set_key.o hexout.o chipset.o checksum32.o QualcommHeader1.o typename.o
	${CC} -shared -o ${@} ${^} ${LDFLAGS} -g -fPIC
slac_wrapper.o: slac_wrapper.c py_module.h adc_wrapper.h
	${CC} -shared -c ${<} ${CFLAGS} -g -fPIC -I/usr/include/python$(PYTHON_VERSION)
adc_wrapper.o: adc_wrapper.c adc_wrapper.h
	${CC} -shared -c ${<} ${CFLAGS} -g -fPIC
plc_wrapper.o: plc_wrapper.c py_module.h
	${CC} -shared -c ${<} ${CFLAGS} -g -fPIC -I/usr/include/python$(PYTHON_VERSION)
mod_main.o: mod_main.c py_module.h
	${CC} -shared -c ${<} ${CFLAGS} -g -fPIC -I/usr/include/python$(PYTHON_VERSION)

# ====================================================================
# includes;
# --------------------------------------------------------------------

include ../tools/tools.mak
include ../ether/ether.mak
include ../mme/mme.mak
include ../plc/plc.mak
include ../key/key.mak
include ../slac/slac.mak
