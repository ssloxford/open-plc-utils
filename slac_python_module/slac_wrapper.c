#include "py_module.h"

#define PEV_STATE_NONE 0
#define PEV_STATE_DISCONNECTED 1
#define PEV_STATE_UNMATCHED 2
#define PEV_STATE_MATCHED 3
#define PEV_STATE_CONNECTED 4

#define EVSE_STATE_UNAVAILABLE 0
#define EVSE_STATE_UNOCCUPIED 1
#define EVSE_STATE_UNMATCHED 2
#define EVSE_STATE_MATCHED 3

PyObject* method_chan_init(PyObject* self, PyObject* args) {
    const char* str;
    if (!PyArg_ParseTuple(args, "s", &str)) {
        return PyLong_FromLong(SLAC_ERROR_ARGUMENT);
    }

    Py_BEGIN_ALLOW_THREADS;

    initchannel(&global_channel);

    memset(&global_session, 0, sizeof(global_session));
    memset(&global_message, 0, sizeof(global_message));
    global_channel.timeout = SLAC_TIMEOUT;
    global_channel.ifname = strdup(str);

    openchannel(&global_channel);
    Py_END_ALLOW_THREADS;
    printf("Initialized channel for %s\n", str);
    return PyLong_FromLong(SLAC_ERROR_OK);
}

//
// EV
//

PyObject* method_ev_slac_init(PyObject* self, PyObject* args) {
    memset(global_session.RunID, 0, sizeof(global_session.RunID));

    Py_RETURN_NONE;
}

//Initialize SLAC
PyObject* method_ev_slac_reset(PyObject* self, PyObject* args) {
    time_t now;
    time(&now);
    
    memcpy(global_session.PEV_MAC, global_channel.host, sizeof(global_session.PEV_MAC));

    global_session.next = global_session.prev = &global_session;
    memset(global_session.PEV_ID, 0, sizeof(global_session.PEV_ID));
    memset(global_session.EVSE_ID, 0, sizeof(global_session.EVSE_ID));
    hexencode(global_session.NMK, sizeof(global_session.NMK), "50D3E4933F855B7040784DF815AA8DB7");
    hexencode(global_session.NID, sizeof(global_session.NID), "B0F2E695666B03");
    global_session.limit = 40;
    global_session.pause = 20;
    global_session.settletime = 10;
    global_session.chargetime = 3600;
    global_session.state = PEV_STATE_DISCONNECTED;
    memcpy(global_session.original_nmk, global_session.NMK, sizeof(global_session.original_nmk));
    memcpy(global_session.original_nid, global_session.NID, sizeof(global_session.original_nid));

    slac_memrand(global_session.RunID, sizeof(global_session.RunID));

    Py_RETURN_NONE;
}

//Wipe
PyObject* method_ev_slac_wipekey(PyObject* self, PyObject* args) {
    signed int res;
    Py_BEGIN_ALLOW_THREADS;
    res = pev_cm_set_key(&global_session, &global_channel, &global_message);
    Py_END_ALLOW_THREADS;
    if (res) {
        printf("Wipe key error %i\n", res);
        return PyLong_FromLong(SLAC_ERROR_WIPE_KEY);
    }
    global_session.state = PEV_STATE_DISCONNECTED;

    return PyLong_FromLong(SLAC_ERROR_OK);
}

//Run SLAC
PyObject* method_ev_slac_param(PyObject* self, PyObject* args) {
    signed int res;
    //slac_session(&global_session);
    //slac_debug(&global_session, 0, __func__, "Probing ...");

    Py_BEGIN_ALLOW_THREADS;

    //slac_memrand(global_session.RunID, sizeof(global_session.RunID));

    res = pev_cm_slac_param(&global_session, &global_channel, &global_message);
    Py_END_ALLOW_THREADS;
        
    if (res) {
        return PyLong_FromLong(SLAC_ERROR_AGAIN);
    }
    global_session.state = PEV_STATE_UNMATCHED;
    return PyLong_FromLong(SLAC_ERROR_OK);
}

PyObject* method_ev_slac_start_atten(PyObject* self, PyObject* args) {
    signed int res;
    //slac_session(&global_session);
    //slac_debug(&global_session, 0, __func__, "Sounding ...");
    Py_BEGIN_ALLOW_THREADS;
    res = pev_cm_start_atten_char(&global_session, &global_channel, &global_message);
    Py_END_ALLOW_THREADS;
    if (res) {
        global_session.state = PEV_STATE_DISCONNECTED;
        return PyLong_FromLong(SLAC_ERROR_START_ATTEN_CHAR);
    }

    return PyLong_FromLong(SLAC_ERROR_OK);
}

PyObject* method_ev_slac_mnbc_sound(PyObject* self, PyObject* args) {
    signed int res;
    Py_BEGIN_ALLOW_THREADS;
    res = pev_cm_mnbc_sound(&global_session, &global_channel, &global_message);
    Py_END_ALLOW_THREADS;
    if (res) {
        global_session.state = PEV_STATE_DISCONNECTED;
        return PyLong_FromLong(SLAC_ERROR_SOUNDING);
    }

    return PyLong_FromLong(SLAC_ERROR_OK);
}

PyObject* method_ev_slac_atten_char(PyObject* self, PyObject* args) {
    signed int res;
    Py_BEGIN_ALLOW_THREADS;
    res = pev_cm_atten_char(&global_session, &global_channel, &global_message);
    Py_END_ALLOW_THREADS;
    if (res) {
        global_session.state = PEV_STATE_DISCONNECTED;
        return PyLong_FromLong(SLAC_ERROR_ATTEN_CHAR);
    }

    return PyLong_FromLong(SLAC_ERROR_OK);
}

PyObject* method_ev_slac_connect(PyObject* self, PyObject* args) {
    signed int res;
    Py_BEGIN_ALLOW_THREADS;
    res = slac_connect(&global_session);
    Py_END_ALLOW_THREADS;
    if (res) {
        global_session.state = PEV_STATE_DISCONNECTED;
        return PyLong_FromLong(SLAC_ERROR_CONNECT);
    }

    return PyLong_FromLong(SLAC_ERROR_OK);
}

PyObject* method_ev_slac_slac_match(PyObject* self, PyObject* args) {
    signed int res;
    //slac_debug(&global_session, 0, __func__, "Matching ...");
    Py_BEGIN_ALLOW_THREADS;
    res = pev_cm_slac_match(&global_session, &global_channel, &global_message);
    Py_END_ALLOW_THREADS;
    if (res) {
        global_session.state = PEV_STATE_DISCONNECTED;
        return PyLong_FromLong(SLAC_ERROR_MATCH);
    }
    global_session.state = PEV_STATE_MATCHED;

    //slac_session(&global_session);
    return PyLong_FromLong(SLAC_ERROR_OK);
}

//Connect
PyObject* method_ev_slac_set_nmk(PyObject* self, PyObject* args) {
    signed int res;
    Py_BEGIN_ALLOW_THREADS;
    res = pev_cm_set_key(&global_session, &global_channel, &global_message);
    Py_END_ALLOW_THREADS;
    if (res) {
        global_session.state = PEV_STATE_DISCONNECTED;
        return PyLong_FromLong(SLAC_ERROR_SET_KEY);
    }
    global_session.state = PEV_STATE_CONNECTED;
    return PyLong_FromLong(SLAC_ERROR_OK);
}

//
// Read struct
//

//Functions to read SLAC struct params

PyObject* method_slac_set_nmk(PyObject* self, PyObject* args) {
    const char* buf;
    Py_ssize_t count;

    if (!PyArg_ParseTuple(args, "y#", &buf, &count))
    {
        return PyLong_FromLong(SLAC_ERROR_ARGUMENT);
    }

    if(count != SLAC_NMK_LEN) {
        return PyLong_FromLong(SLAC_ERROR_ARGUMENT);
    }

    memcpy(global_session.NMK, buf, SLAC_NMK_LEN);

    return PyLong_FromLong(SLAC_ERROR_OK);
}

PyObject* method_slac_set_nid(PyObject* self, PyObject* args) {
    const char* buf;
    Py_ssize_t count;

    if (!PyArg_ParseTuple(args, "y#", &buf, &count))
    {
        return PyLong_FromLong(SLAC_ERROR_ARGUMENT);
    }

    if(count != SLAC_NID_LEN) {
        return PyLong_FromLong(SLAC_ERROR_ARGUMENT);
    }

    memcpy(global_session.NID, buf, SLAC_NID_LEN);

    return PyLong_FromLong(SLAC_ERROR_OK);
}

PyObject* method_slac_read_nmk(PyObject* self, PyObject* args) {
    return Py_BuildValue("y#", global_session.NMK, SLAC_NMK_LEN);
}

PyObject* method_slac_read_nid(PyObject* self, PyObject* args) {
    return Py_BuildValue("y#", global_session.NID, SLAC_NID_LEN);
}

PyObject* method_slac_read_run_id(PyObject* self, PyObject* args) {
    return Py_BuildValue("y#", global_session.RunID, sizeof(global_session.RunID));
}

PyObject* method_slac_read_pev_mac(PyObject* self, PyObject* args) {
    return Py_BuildValue("y#", global_session.PEV_MAC, ETHER_ADDR_LEN);
}

PyObject* method_slac_read_evse_mac(PyObject* self, PyObject* args) {
    return Py_BuildValue("y#", global_session.EVSE_MAC, ETHER_ADDR_LEN);
}

PyObject* method_slac_read_pev_id(PyObject* self, PyObject* args) {
    return Py_BuildValue("y#", global_session.PEV_ID, SLAC_UNIQUE_ID_LEN);
}

PyObject* method_slac_read_evse_id(PyObject* self, PyObject* args) {
    return Py_BuildValue("y#", global_session.EVSE_ID, SLAC_UNIQUE_ID_LEN);
}

PyObject* method_slac_read_aag(PyObject* self, PyObject* args) {
    return Py_BuildValue("y#", global_session.AAG, (global_session.NumGroups < sizeof(global_session.AAG)) ? global_session.NumGroups : sizeof(global_session.AAG));
}

PyObject* method_slac_read_num_sounds(PyObject* self, PyObject* args) {
    return PyLong_FromLong(global_session.NUM_SOUNDS);
}

//Basic signalling
PyObject* method_init_adc(PyObject* self, PyObject* args) {
    int cal_m12, cal_0, cal_p12;
    if (!PyArg_ParseTuple(args, "iii", &cal_m12, &cal_0, &cal_p12)) {
        return PyLong_FromLong(SLAC_ERROR_ARGUMENT);
    }

    Py_BEGIN_ALLOW_THREADS;

    spi_init(cal_m12, cal_0, cal_p12);

    Py_END_ALLOW_THREADS;
    return PyLong_FromLong(SLAC_ERROR_OK);
}


PyObject* method_measure_cp_stat(PyObject* self, PyObject* args) {
    struct CP_state state;
    Py_BEGIN_ALLOW_THREADS;
    
    state = read_pwm_signal(1);
    
    Py_END_ALLOW_THREADS;
    
    return Py_BuildValue("fff", state.level_low, state.level_high, state.duty);
}

PyObject* method_measure_raw(PyObject* self, PyObject* args) {
    int chan;
    if (!PyArg_ParseTuple(args, "i", &chan)) {
        return PyLong_FromLong(-1);
    }

    int res = -1;

    Py_BEGIN_ALLOW_THREADS;

    res = read_adc_single(chan);

    Py_END_ALLOW_THREADS;
    return PyLong_FromLong(res);
}
