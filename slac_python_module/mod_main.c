#include "py_module.h"

struct session global_session;
struct message global_message;

const char* program_name = "slac_wrapper.so";


// Binding

static PyMethodDef custom_methods[] = {
    {"init_adc", method_init_adc, METH_VARARGS, NULL},
    {"chan_init", method_chan_init, METH_VARARGS, NULL},
    

    {"ev_init", method_ev_slac_init, METH_VARARGS, NULL},
    {"ev_reset", method_ev_slac_reset, METH_NOARGS, NULL},
    {"ev_wipekey", method_ev_slac_wipekey, METH_NOARGS, NULL},
    {"ev_param", method_ev_slac_param, METH_NOARGS, NULL},
    {"ev_start_atten", method_ev_slac_start_atten, METH_NOARGS, NULL},
    {"ev_mnbc_sound", method_ev_slac_mnbc_sound, METH_NOARGS, NULL},
    {"ev_atten_char", method_ev_slac_atten_char, METH_NOARGS, NULL},
    {"ev_connect", method_ev_slac_connect, METH_NOARGS, NULL},
    {"ev_match", method_ev_slac_slac_match, METH_NOARGS, NULL},
    {"ev_set_nmk", method_ev_slac_set_nmk, METH_NOARGS, NULL},

    {"set_nmk", method_slac_set_nmk, METH_VARARGS, NULL},
    {"set_nid", method_slac_set_nid, METH_VARARGS, NULL},

    {"read_nmk", method_slac_read_nmk, METH_NOARGS, NULL},
    {"read_nid", method_slac_read_nid, METH_NOARGS, NULL},
    {"read_run_id", method_slac_read_run_id, METH_NOARGS, NULL},
    {"read_pev_mac", method_slac_read_pev_mac, METH_NOARGS, NULL},
    {"read_evse_mac", method_slac_read_evse_mac, METH_NOARGS, NULL},
    {"read_pev_id", method_slac_read_pev_id, METH_NOARGS, NULL},
    {"read_evse_id", method_slac_read_evse_id, METH_NOARGS, NULL},
    {"read_aag", method_slac_read_aag, METH_NOARGS, NULL},
    {"read_num_sounds", method_slac_read_num_sounds, METH_NOARGS, NULL},

    {"measure_cp_stat", method_measure_cp_stat, METH_NOARGS, NULL},
    {"measure_raw", method_measure_raw, METH_VARARGS, NULL},

    {"sw_ver", method_plctool_sw_ver, METH_VARARGS, NULL},
    {"nw_info", method_plctool_nw_info, METH_VARARGS, NULL},
    {"vs_mod", method_plctool_vs_mod, METH_VARARGS, NULL},

    {NULL, NULL, 0, NULL} // Sentinel
};

static struct PyModuleDef module_definition = {
    PyModuleDef_HEAD_INIT,
    "slac_wrapper",   // Module name
    NULL,       // Module documentation, may be NULL
    -1,         // Size of per-interpreter state of the module, or -1 if the module keeps state in global variables.
    custom_methods,
    NULL,
    NULL,
    NULL,
    NULL,
};

bool init_slac_add_const(PyObject* module, const char* name, int value) {
    PyObject* obj = PyLong_FromLong(value);
    int res = PyModule_AddObjectRef(module, name, obj);
    Py_XDECREF(obj);
    if (res < 0) {
        Py_DECREF(module);
        return true;
    }
    return false;
}

PyMODINIT_FUNC PyInit_slac_wrapper(void) {
    PyObject* module = PyModule_Create(&module_definition);
    if (module == NULL)
        return NULL;

    //Bind constants
    if (init_slac_add_const(module, "ERROR_OK", SLAC_ERROR_OK)) return NULL;
    if (init_slac_add_const(module, "ERROR_INTERRUPT", SLAC_ERROR_INTERRUPT)) return NULL;
    if (init_slac_add_const(module, "ERROR_AGAIN", SLAC_ERROR_AGAIN)) return NULL;
    if (init_slac_add_const(module, "ERROR_ARGUMENT", SLAC_ERROR_ARGUMENT)) return NULL;
    if (init_slac_add_const(module, "ERROR_WIPE_KEY", SLAC_ERROR_WIPE_KEY)) return NULL;
    if (init_slac_add_const(module, "ERROR_SET_KEY", SLAC_ERROR_SET_KEY)) return NULL;
    if (init_slac_add_const(module, "ERROR_START_ATTEN_CHAR", SLAC_ERROR_START_ATTEN_CHAR)) return NULL;
    if (init_slac_add_const(module, "ERROR_SOUNDING", SLAC_ERROR_SOUNDING)) return NULL;
    if (init_slac_add_const(module, "ERROR_ATTEN_CHAR", SLAC_ERROR_ATTEN_CHAR)) return NULL;
    if (init_slac_add_const(module, "ERROR_CONNECT", SLAC_ERROR_CONNECT)) return NULL;
    if (init_slac_add_const(module, "ERROR_MATCH", SLAC_ERROR_MATCH)) return NULL;

    return module;
}