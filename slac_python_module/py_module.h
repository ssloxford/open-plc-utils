#define PY_SSIZE_T_CLEAN
#include <Python.h>


#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netdb.h>


/*====================================================================*
 *   custom header files;
 *--------------------------------------------------------------------*/

 //#include "../tools/getoptv.h"
 //#include "../tools/putoptv.h"
#include "../tools/memory.h"
#include "../tools/number.h"
#include "../tools/types.h"
#include "../tools/flags.h"
#include "../tools/files.h"
#include "../tools/error.h"
#include "../tools/config.h"
#include "../tools/permissions.h"
#include "../ether/channel.h"
#include "../slac/slac.h"

#include "adc_wrapper.h"

 /*====================================================================*
  *   custom source files;
  *--------------------------------------------------------------------*/

#ifndef MAKEFILE
#include "../tools/getoptv.c"
#include "../tools/putoptv.c"
#include "../tools/version.c"
#include "../tools/hexdump.c"
#include "../tools/hexdecode.c"
#include "../tools/hexencode.c"
#include "../tools/hexstring.c"
#include "../tools/decdecode.c"
#include "../tools/decstring.c"
#include "../tools/uintspec.c"
#include "../tools/todigit.c"
#include "../tools/strfbits.c"
#include "../tools/config.c"
#include "../tools/memincr.c"
#include "../tools/error.c"
#include "../tools/desuid.c"
#endif

#ifndef MAKEFILE
#include "../plc/Devices.c"
#endif

#ifndef MAKEFILE
#include "../mme/EthernetHeader.c"
#include "../mme/QualcommHeader.c"
#include "../mme/HomePlugHeader1.c"
#include "../mme/UnwantedMessage.c"
#include "../mme/readmessage.c"
#include "../mme/sendmessage.c"
#endif

#ifndef MAKEFILE
#include "../ether/channel.c"
#include "../ether/initchannel.c"
#include "../ether/openchannel.c"
#include "../ether/closechannel.c"
#include "../ether/sendpacket.c"
#include "../ether/readpacket.c"
#endif

#ifndef MAKEFILE
#include "../slac/slac_session.c"
#include "../slac/slac_connect.c"
#include "../slac/slac_debug.c"
#include "../slac/pev_cm_slac_param.c"
#include "../slac/pev_cm_start_atten_char.c"
#include "../slac/pev_cm_atten_char.c"
#include "../slac/pev_cm_mnbc_sound.c"
#include "../slac/pev_cm_slac_match.c"
#include "../slac/pev_cm_set_key.c"
#endif


extern struct channel channel;
extern struct session global_session;
#define global_channel channel
extern struct message global_message;


enum SLAC_ERRORS {
    SLAC_ERROR_OK = 0,
    SLAC_ERROR_INTERRUPT,
    SLAC_ERROR_AGAIN,
    SLAC_ERROR_ARGUMENT,
    SLAC_ERROR_WIPE_KEY,
    SLAC_ERROR_SET_KEY,
    SLAC_ERROR_START_ATTEN_CHAR,
    SLAC_ERROR_SOUNDING,
    SLAC_ERROR_ATTEN_CHAR,
    SLAC_ERROR_CONNECT,
    SLAC_ERROR_MATCH,
};

PyObject* method_chan_init(PyObject* self, PyObject* args);

//
// EV
//

PyObject* method_ev_slac_init(PyObject* self, PyObject* args);

//Initialize SLAC
PyObject* method_ev_slac_reset(PyObject* self, PyObject* args);

//Wipe
PyObject* method_ev_slac_wipekey(PyObject* self, PyObject* args);

//Run SLAC
PyObject* method_ev_slac_param(PyObject* self, PyObject* args);
PyObject* method_ev_slac_start_atten(PyObject* self, PyObject* args);
PyObject* method_ev_slac_mnbc_sound(PyObject* self, PyObject* args);
PyObject* method_ev_slac_atten_char(PyObject* self, PyObject* args);
PyObject* method_ev_slac_connect(PyObject* self, PyObject* args);
PyObject* method_ev_slac_slac_match(PyObject* self, PyObject* args);

//Connect
PyObject* method_ev_slac_set_nmk(PyObject* self, PyObject* args);

//
// Read struct
//

PyObject* method_slac_set_nmk(PyObject* self, PyObject* args);
PyObject* method_slac_set_nid(PyObject* self, PyObject* args);

PyObject* method_slac_read_nmk(PyObject* self, PyObject* args);
PyObject* method_slac_read_nid(PyObject* self, PyObject* args);
PyObject* method_slac_read_run_id(PyObject* self, PyObject* args);
PyObject* method_slac_read_pev_mac(PyObject* self, PyObject* args);
PyObject* method_slac_read_evse_mac(PyObject* self, PyObject* args);
PyObject* method_slac_read_pev_id(PyObject* self, PyObject* args);
PyObject* method_slac_read_evse_id(PyObject* self, PyObject* args);
PyObject* method_slac_read_aag(PyObject* self, PyObject* args);
PyObject* method_slac_read_num_sounds(PyObject* self, PyObject* args);

//
//Basic signalling
//

PyObject* method_init_adc(PyObject* self, PyObject* args);
PyObject* method_measure_cp_stat(PyObject* self, PyObject* args);
PyObject* method_measure_raw(PyObject* self, PyObject* args);

//
// NW Discovery
//
PyObject* method_plctool_sw_ver(PyObject* self, PyObject* args);
PyObject* method_plctool_nw_info(PyObject* self, PyObject* args);
PyObject* method_plctool_vs_mod(PyObject* self, PyObject* args);