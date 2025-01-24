#include "py_module.h"

#include "../plc/plc.h"
#include "../tools/timer.h"

static void PyDict_SetItem_Steal(PyObject* dict, PyObject* k, PyObject* v) {
    PyDict_SetItem(dict, k, v);

    Py_DECREF(k);
    Py_DECREF(v);
}

static void PyList_Append_Steal(PyObject* list, PyObject* v) {
    PyList_Append(list, v);

    Py_DECREF(v);
}

signed ReadMME_polyfill (uint8_t MMV, uint16_t MMTYPE) {
	ssize_t plc_packetsize;
	struct timeval ts;
	struct timeval tc;
	if (gettimeofday (&ts, NULL) == -1)
	{
		error (1, errno, CANT_START_TIMER);
	}
	while ((plc_packetsize = readpacket (&global_channel, &global_message, sizeof (global_message))) >= 0)
	{
		if (UnwantedMessage (&global_message, plc_packetsize, MMV, MMTYPE))
		{
			if (gettimeofday (&tc, NULL) == -1)
			{
				error (1, errno, CANT_RESET_TIMER);
			}
			if (global_channel.timeout < 0)
			{
				continue;
			}
			if (global_channel.timeout > MILLISECONDS (ts, tc))
			{
				continue;
			}
			plc_packetsize = 0;
		}
		break;
	}
	return (plc_packetsize);
}

static char const * CCoMode2 [] = {
	"Auto",
	"Never",
	"Always",
	"User",
	"Covert",
	"Unknown"
};

static char const * MDURole2 [] = {
	"Slave",
	"Master"
};

//r
PyObject* method_plctool_sw_ver(PyObject* self, PyObject* args) {
	struct message * message = &global_message;

#ifndef __GNUC__
#pragma pack (push,1)
#endif

	struct __packed vs_sw_ver_request
	{
		struct ethernet_hdr ethernet;
		struct qualcomm_hdr qualcomm;
		uint32_t COOKIE;
	}
	* request = (struct vs_sw_ver_request *) (message);
	struct __packed vs_sw_ver_confirm
	{
		struct ethernet_hdr ethernet;
		struct qualcomm_hdr qualcomm;
		uint8_t MSTATUS;
		uint8_t MDEVICE_CLASS;
		uint8_t MVERLENGTH;
		char MVERSION [254];
		uint32_t IDENT;
		uint32_t STEPPING_NUM;
		uint32_t COOKIE;
		uint32_t RSVD [6];
	}
	* confirm = (struct vs_sw_ver_confirm *) (message);

#ifndef __GNUC__
#pragma pack (pop)
#endif

    const char* mac_buf;
    Py_ssize_t mac_count;

    if (!PyArg_ParseTuple(args, "y#", &mac_buf, &mac_count)) {
        Py_RETURN_NONE;
    }

    if(mac_count != 6) {
        Py_RETURN_NONE;
    }

	int io_res;

    Py_BEGIN_ALLOW_THREADS;

	memset (message, 0, sizeof (* message));
	EthernetHeader (&request->ethernet, (const unsigned char*)mac_buf, global_channel.host, global_channel.type);
	QualcommHeader (&request->qualcomm, 0, (VS_SW_VER | MMTYPE_REQ));
	slac_memrand(&request->COOKIE, sizeof(request->COOKIE));

	io_res = sendpacket (&global_channel, message, ETHER_MIN_LEN - ETHER_CRC_LEN);
    Py_END_ALLOW_THREADS;

	if (io_res <= 0)
	{
		error (0, errno, CHANNEL_CANTSEND);
		Py_RETURN_NONE;
	}


    // Create a Python list
    PyObject* list = PyList_New(0);
    if (!list) {
        Py_RETURN_NONE; // Return NULL on failure
    }

	while (true)
	{
        Py_BEGIN_ALLOW_THREADS;
        io_res = ReadMME_polyfill ( 0, (VS_SW_VER | MMTYPE_CNF));
        Py_END_ALLOW_THREADS;
        if(io_res <= 0) {
            break;
        }

		uint32_t ident;

		/* firmware doesn't fill confirm->COOKIE field at (confirm->MVERSION + confirm->MVERLENGTH + 9)
		 * (and IDENT, STEPPING_NUM) properly, so we don't check it.
		 */

		if (confirm->MSTATUS)
		{
			//Failure (plc, PLC_WONTDOIT);
			continue;
		}

		chipset (confirm, & ident);
        

        // Create some bytes objects
        PyObject* dict = PyDict_New();
        if (!dict) {
            Py_DECREF(list);
            Py_RETURN_NONE;
        }

        PyDict_SetItem_Steal(dict, PyUnicode_FromString("MAC"), PyBytes_FromStringAndSize((const char*)message->ethernet.OSA, sizeof (message->ethernet.OSA)));
        PyDict_SetItem_Steal(dict, PyUnicode_FromString("IDENT"), PyUnicode_FromString(chipsetname_by_ident (ident, confirm->MDEVICE_CLASS)));
        PyDict_SetItem_Steal(dict, PyUnicode_FromString("VERSION"), PyUnicode_FromString(confirm->MVERSION));
        
        PyList_Append_Steal(list, dict);
	}
	
    return list;
}

//m
PyObject* method_plctool_nw_info(PyObject* self, PyObject* args) {
	//extern char const * StationRole [];
	struct message * message = &global_message;

#ifndef __GNUC__
#pragma pack (push,1)
#endif

	struct __packed vs_nw_info_request
	{
		struct ethernet_hdr ethernet;
		struct qualcomm_fmi qualcomm;
	}
	* request = (struct vs_nw_info_request *)(message);
	struct __packed vs_nw_info_confirm
	{
		struct ethernet_hdr ethernet;
		struct qualcomm_fmi qualcomm;
		uint8_t SUB_VERSION;
		uint8_t Reserved;
		uint16_t DATA_LEN;
		uint8_t DATA [1];
	}
	* confirm = (struct vs_nw_info_confirm *)(message);
	struct __packed station
	{
		uint8_t MAC [ETHER_ADDR_LEN];
		uint8_t TEI;
		uint8_t Reserved [3];
		uint8_t BDA [ETHER_ADDR_LEN];
		uint16_t AVGTX;
		uint8_t COUPLING;
		uint8_t Reserved3;
		uint16_t AVGRX;
		uint16_t Reserved4;
	}
	* station;
	struct __packed network
	{
		uint8_t NID [7];
		uint8_t Reserved1 [2];
		uint8_t SNID;
		uint8_t TEI;
		uint8_t Reserved2 [4];
		uint8_t ROLE;
		uint8_t CCO_MAC [ETHER_ADDR_LEN];
		uint8_t CCO_TEI;
		uint8_t Reserved3 [3];
		uint8_t NUMSTAS;
		uint8_t Reserved4 [5];
		struct station stations [1];
	}
	* network;
	struct __packed networks
	{
		uint8_t Reserved;
		uint8_t NUMAVLNS;
		struct network networks [1];
	}
	* networks = (struct networks *) (confirm->DATA);

#ifndef __GNUC__
#pragma pack (pop)
#endif

    const char* mac_buf;
    Py_ssize_t mac_count;

    if (!PyArg_ParseTuple(args, "y#", &mac_buf, &mac_count)) {
        Py_RETURN_NONE;
    }

    if(mac_count != 6) {
        Py_RETURN_NONE;
    }

	//Request (plc, "Fetch Network Information");
	memset (message, 0, sizeof (* message));
	EthernetHeader (&request->ethernet, (const unsigned char*)mac_buf, global_channel.host, global_channel.type);
	QualcommHeader1 (&request->qualcomm, 1, (VS_NW_INFO | MMTYPE_REQ));
	if (sendpacket (&global_channel, message, ETHER_MIN_LEN - ETHER_CRC_LEN) <= 0)
	{
		error (0, errno, CHANNEL_CANTSEND);
		Py_RETURN_NONE;
	}

    // Create a Python list
    PyObject* list = PyList_New(0);
    if (!list) {
        Py_RETURN_NONE;
    }

    while (true)
	{
        int read_res;
        Py_BEGIN_ALLOW_THREADS;
        read_res = ReadMME_polyfill (1, (VS_NW_INFO | MMTYPE_CNF));
        Py_END_ALLOW_THREADS;
        if(read_res <= 0) {
            break;
        }

		//char string [24];
		//Confirm (plc, "Found %d Network(s)\n", networks->NUMAVLNS);
		//hexdecode (message->ethernet.OSA, sizeof (message->ethernet.OSA), string, sizeof (string));
		//printf("source address = %s\n\n", string);
		network = (struct network *)(&networks->networks);

        // Create a Python list
        PyObject* list2 = PyList_New(0);
        if (!list2) {
            Py_DECREF(list);
            Py_RETURN_NONE;
        }

		while (networks->NUMAVLNS--)
		{
            // Create a Python dictionary
            PyObject* dict = PyDict_New();
            if (!dict) {
                Py_DECREF(list2);
                Py_DECREF(list);
                Py_RETURN_NONE;
            }

			//printf ("\tnetwork->NID = %s\n", hexstring (string, sizeof (string), network->NID, sizeof (network->NID)));
            PyDict_SetItem_Steal(dict, PyUnicode_FromString("NID"), PyBytes_FromStringAndSize((const char*)network->NID, sizeof (network->NID)));
			//printf ("\tnetwork->SNID = %d\n", network->SNID);
            PyDict_SetItem_Steal(dict, PyUnicode_FromString("SNID"), PyLong_FromLong(network->SNID));
			//printf ("\tnetwork->TEI = %d\n", network->TEI);
            PyDict_SetItem_Steal(dict, PyUnicode_FromString("TEI"), PyLong_FromLong(network->TEI));
			//printf ("\tnetwork->ROLE = 0x%02X (%s)\n", network->ROLE, StationRole [network->ROLE]);
            PyDict_SetItem_Steal(dict, PyUnicode_FromString("ROLE"), PyLong_FromLong(network->ROLE));
			//printf ("\tnetwork->CCO_DA = %s\n", hexstring (string, sizeof (string), network->CCO_MAC, sizeof (network->CCO_MAC)));
            PyDict_SetItem_Steal(dict, PyUnicode_FromString("CCO_DA"), PyBytes_FromStringAndSize((const char*)network->CCO_MAC, sizeof (network->CCO_MAC)));
			//printf ("\tnetwork->CCO_TEI = %d\n", network->CCO_TEI);
            PyDict_SetItem_Steal(dict, PyUnicode_FromString("CCO_TEI"), PyLong_FromLong(network->CCO_TEI));
			//printf ("\tnetwork->STATIONS = %d\n", network->NUMSTAS);

            PyObject* list3 = PyList_New(0);
            if (!list3) {
                Py_DECREF(dict);
                Py_DECREF(list2);
                Py_DECREF(list);
                Py_RETURN_NONE;
            }

			//printf ("\n");
			station = (struct station *)(&network->stations);
			while (network->NUMSTAS--)
			{
                // Create a Python dictionary
                PyObject* dict2 = PyDict_New();
                if (!dict2) {
                    Py_DECREF(list3);
                    Py_DECREF(dict);
                    Py_DECREF(list2);
                    Py_DECREF(list);
                    Py_RETURN_NONE;
                }

                //printf ("\t\tstation->MAC = %s\n", hexstring (string, sizeof (string), station->MAC, sizeof (station->MAC)));
                PyDict_SetItem_Steal(dict2, PyUnicode_FromString("MAC"), PyBytes_FromStringAndSize((const char*)station->MAC, sizeof (station->MAC)));
				//printf ("\t\tstation->TEI = %d\n", station->TEI);
                PyDict_SetItem_Steal(dict2, PyUnicode_FromString("TEI"), PyLong_FromLong(network->TEI));
				//printf ("\t\tstation->BDA = %s\n", hexstring (string, sizeof (string), station->BDA, sizeof (station->BDA)));
                PyDict_SetItem_Steal(dict2, PyUnicode_FromString("BDA"), PyBytes_FromStringAndSize((const char*)station->BDA, sizeof (station->BDA)));
				station->AVGTX = LE16TOH (station->AVGTX);
				station->AVGRX = LE16TOH (station->AVGRX);
				//printf ("\t\tstation->AvgPHYDR_TX = %03d mbps %s\n", station->AVGTX, ((station->COUPLING >> 0) & 0x0F)? "Alternate": "Primary");
                PyDict_SetItem_Steal(dict2, PyUnicode_FromString("AvgPHYDR_TX"), PyLong_FromLong(station->AVGTX));
				//printf ("\t\tstation->AvgPHYDR_RX = %03d mbps %s\n", station->AVGRX, ((station->COUPLING >> 4) & 0x0F)? "Alternate": "Primary");
                PyDict_SetItem_Steal(dict2, PyUnicode_FromString("AvgPHYDR_RX"), PyLong_FromLong(station->AVGRX));
                PyDict_SetItem_Steal(dict2, PyUnicode_FromString("COUPLING"), PyLong_FromLong(station->COUPLING));
				//printf ("\n");
				station++;

                PyList_Append_Steal(list3, dict2);
			}
			network = (struct network *)(station);

            PyDict_SetItem_Steal(dict, PyUnicode_FromString("STATIONS"), list3);

            PyList_Append_Steal(list2, dict);
		}

        PyList_Append_Steal(list, list2);
	}

	return list;
}

//I
PyObject* method_plctool_vs_mod(PyObject* self, PyObject* args) {
	struct message * message = &global_message;

#ifndef __GNUC__
#pragma pack (push,1)
#endif

	struct __packed vs_module_operation_read_request
	{
		struct ethernet_hdr ethernet;
		struct qualcomm_hdr qualcomm;
		uint32_t RESERVED;
		uint8_t NUM_OP_DATA;
		struct __packed
		{
			uint16_t MOD_OP;
			uint16_t MOD_OP_DATA_LEN;
			uint32_t MOD_OP_RSVD;
			uint16_t MODULE_ID;
			uint16_t MODULE_SUB_ID;
			uint16_t MODULE_LENGTH;
			uint32_t MODULE_OFFSET;
		}
		MODULE_SPEC;
	}
	* request = (struct vs_module_operation_read_request *)(message);
	struct __packed vs_module_operation_read_confirm
	{
		struct ethernet_hdr ethernet;
		struct qualcomm_hdr qualcomm;
		uint16_t MSTATUS;
		uint16_t ERR_REC_CODE;
		uint32_t RESERVED;
		uint8_t NUM_OP_DATA;
		struct __packed
		{
			uint16_t MOD_OP;
			uint16_t MOD_OP_DATA_LEN;
			uint32_t MOD_OP_RSVD;
			uint16_t MODULE_ID;
			uint16_t MODULE_SUB_ID;
			uint16_t MODULE_LENGTH;
			uint32_t MODULE_OFFSET;
		}
		MODULE_SPEC;
		uint8_t MODULE_DATA [PLC_MODULE_SIZE];
	}
	* confirm = (struct vs_module_operation_read_confirm *)(message);

#ifndef __GNUC__
#pragma pack (pop)
#endif

    const char* mac_buf;
    Py_ssize_t mac_count;

    if (!PyArg_ParseTuple(args, "y#", &mac_buf, &mac_count)) {
        Py_RETURN_NONE;
    }

    if(mac_count != 6) {
        Py_RETURN_NONE;
    }

    int io_res;
    Py_BEGIN_ALLOW_THREADS;

	memset (message, 0, sizeof (* message));
	EthernetHeader (&request->ethernet, (const unsigned char*)mac_buf, global_channel.host, global_channel.type);
	QualcommHeader (&request->qualcomm, 0, (VS_MODULE_OPERATION | MMTYPE_REQ));
	request->NUM_OP_DATA = 1;
	request->MODULE_SPEC.MOD_OP = HTOLE16 (0);
	request->MODULE_SPEC.MOD_OP_DATA_LEN = HTOLE16 (sizeof (request->MODULE_SPEC));
	request->MODULE_SPEC.MOD_OP_RSVD = HTOLE32 (0);
	request->MODULE_SPEC.MODULE_ID = HTOLE16 (PLC_MODULEID_PARAMETERS);
	request->MODULE_SPEC.MODULE_SUB_ID = HTOLE16 (0);
	request->MODULE_SPEC.MODULE_LENGTH = HTOLE16 (PLC_MODULE_SIZE);
	request->MODULE_SPEC.MODULE_OFFSET = HTOLE32 (0);

    
    io_res = sendpacket (&global_channel, message, ETHER_MIN_LEN - ETHER_CRC_LEN);
    Py_END_ALLOW_THREADS;

	if (io_res <= 0)
	{
		error (0, errno, CHANNEL_CANTSEND);
		Py_RETURN_NONE;
	}

    Py_BEGIN_ALLOW_THREADS;
    io_res = ReadMME_polyfill (0, (VS_MODULE_OPERATION | MMTYPE_CNF));
    Py_END_ALLOW_THREADS;

	if (io_res <= 0)
	{
		error (0, errno, CHANNEL_CANTREAD);
		Py_RETURN_NONE;
	}
	if (confirm->MSTATUS)
	{
		//Failure (plc, PLC_WONTDOIT);
		Py_RETURN_NONE;
	}
	//pibchain2 (confirm->MODULE_DATA, "device", plc->flags);
    //void const * memory, char const * filename, flag_t flags

    void const * memory = confirm->MODULE_DATA;
    char const * filename = "device";

    struct nvm_header2 * nvm_header;
	uint32_t origin = ~0;
	uint32_t offset = 0;
	signed module = 0;

    // Create a Python list
    PyObject* list = PyList_New(0);
    if (!list) {
        Py_RETURN_NONE;
    }

	do
	{
		nvm_header = (struct nvm_header2 *)((char *)(memory) + offset);
		if (LE16TOH (nvm_header->MajorVersion) != 1)
		{
			error (0, errno, NVM_HDR_VERSION, filename, module);
			Py_RETURN_NONE;
		}
		if (LE16TOH (nvm_header->MinorVersion) != 1)
		{
			error (0, errno, NVM_HDR_VERSION, filename, module);
			Py_RETURN_NONE;
		}
		if (LE32TOH (nvm_header->PrevHeader) != origin)
		{
			error (0, errno, NVM_HDR_LINK, filename, module);
			Py_RETURN_NONE;
		}
		if (checksum32 (nvm_header, sizeof (* nvm_header), 0))
		{
			error (0, 0, NVM_HDR_CHECKSUM, filename, module);
			Py_RETURN_NONE;
		}
		origin = offset;
		offset += sizeof (* nvm_header);
		if (LE32TOH (nvm_header->ImageType) == NVM_IMAGE_PIB)
		{
			struct pib_header * pib_header = (struct pib_header *)((char *)(memory) + offset);
			pib_header->PIBLENGTH = HTOLE16((uint16_t)(LE32TOH(nvm_header->ImageLength)));
		
			//extern const struct key keys [KEYS];
			extern char const * CCoMode2 [];
			extern char const * MDURole2 [];
			struct PIB3_0 * PIB = (struct PIB3_0 *)((char *)(memory) + offset);
			//char buffer [HPAVKEY_SHA_LEN * 3];
			//size_t key;

			PyObject* dict = PyDict_New();
			if (!dict) {
				Py_DECREF(list);
				Py_RETURN_NONE;
			}

			//printf ("\tPIB %d-%d %d bytes\n", PIB->VersionHeader.FWVersion, PIB->VersionHeader.PIBVersion, LE16TOH (PIB->VersionHeader.PIBLength));
			PyDict_SetItem_Steal(dict, PyUnicode_FromString("FWVersion"), PyLong_FromLong(PIB->VersionHeader.FWVersion));
			PyDict_SetItem_Steal(dict, PyUnicode_FromString("PIBVersion"), PyLong_FromLong(PIB->VersionHeader.PIBVersion));
			PyDict_SetItem_Steal(dict, PyUnicode_FromString("PIBLength"), PyLong_FromLong(LE16TOH (PIB->VersionHeader.PIBLength)));

			//printf ("\tMAC %s\n", hexstring (buffer, sizeof (buffer), PIB->LocalDeviceConfig.MAC, sizeof (PIB->LocalDeviceConfig.MAC)));
			PyDict_SetItem_Steal(dict, PyUnicode_FromString("MAC"), PyBytes_FromStringAndSize((const char*)PIB->LocalDeviceConfig.MAC, sizeof (PIB->LocalDeviceConfig.MAC)));
			//printf ("\tDAK %s", hexstring (buffer, sizeof (buffer), PIB->LocalDeviceConfig.DAK, sizeof (PIB->LocalDeviceConfig.DAK)));
			PyDict_SetItem_Steal(dict, PyUnicode_FromString("DAK"), PyBytes_FromStringAndSize((const char*)PIB->LocalDeviceConfig.DAK, sizeof (PIB->LocalDeviceConfig.DAK)));
			/*for (key = 0; key < KEYS; key++)
			{
				if (!memcmp (keys [key].DAK, PIB->LocalDeviceConfig.DAK, HPAVKEY_DAK_LEN))
				{
					//printf (" (%s)", keys [key].phrase);
					break;
				}
			}*/
			//printf ("\n");
			//printf ("\tNMK %s", hexstring (buffer, sizeof (buffer), PIB->LocalDeviceConfig.NMK, sizeof (PIB->LocalDeviceConfig.NMK)));
			PyDict_SetItem_Steal(dict, PyUnicode_FromString("NMK"), PyBytes_FromStringAndSize((const char*)PIB->LocalDeviceConfig.NMK, sizeof (PIB->LocalDeviceConfig.NMK)));

			/*for (key = 0; key < KEYS; key++)
			{
				if (!memcmp (keys [key].NMK, PIB->LocalDeviceConfig.NMK, HPAVKEY_NMK_LEN))
				{
					printf (" (%s)", keys [key].phrase);
					break;
				}
			}*/
			//printf ("\n");
			//printf ("\tNID %s\n", hexstring (buffer, sizeof (buffer), PIB->LocalDeviceConfig.PreferredNID, sizeof (PIB->LocalDeviceConfig.PreferredNID)));
			PyDict_SetItem_Steal(dict, PyUnicode_FromString("NID"), PyBytes_FromStringAndSize((const char*)PIB->LocalDeviceConfig.PreferredNID, sizeof (PIB->LocalDeviceConfig.PreferredNID)));
			//printf ("\tSecurity level %u\n", (PIB->LocalDeviceConfig.PreferredNID[HPAVKEY_NID_LEN-1] >> 4) & 3);
			PyDict_SetItem_Steal(dict, PyUnicode_FromString("SL"), PyLong_FromLong((PIB->LocalDeviceConfig.PreferredNID[HPAVKEY_NID_LEN-1] >> 4) & 3));
			//printf ("\tNET %s\n", PIB->LocalDeviceConfig.NET);
			PyDict_SetItem_Steal(dict, PyUnicode_FromString("NET"), PyUnicode_FromString((const char*)PIB->LocalDeviceConfig.NET));
			//printf ("\tMFG %s\n", PIB->LocalDeviceConfig.MFG);
			PyDict_SetItem_Steal(dict, PyUnicode_FromString("MFG"), PyUnicode_FromString((const char*)PIB->LocalDeviceConfig.MFG));
			//printf ("\tUSR %s\n", PIB->LocalDeviceConfig.USR);
			PyDict_SetItem_Steal(dict, PyUnicode_FromString("USR"), PyUnicode_FromString((const char*)PIB->LocalDeviceConfig.USR));
			//printf ("\tCCo %s\n", CCoMode2 [PIB->LocalDeviceConfig.CCoSelection > SIZEOF (CCoMode2)-1?SIZEOF (CCoMode2)-1:PIB->LocalDeviceConfig.CCoSelection]);
			PyDict_SetItem_Steal(dict, PyUnicode_FromString("CCo"), PyUnicode_FromString(CCoMode2 [PIB->LocalDeviceConfig.CCoSelection > SIZEOF (CCoMode2)-1?SIZEOF (CCoMode2)-1:PIB->LocalDeviceConfig.CCoSelection]));
			//printf ("\tMDU %s\n", PIB->LocalDeviceConfig.MDUConfiguration? MDURole2 [PIB->LocalDeviceConfig.MDURole & 1]: "N/A");
			PyDict_SetItem_Steal(dict, PyUnicode_FromString("MDU"), PyUnicode_FromString(PIB->LocalDeviceConfig.MDUConfiguration? MDURole2 [PIB->LocalDeviceConfig.MDURole & 1]: "N/A"));

			PyList_Append_Steal(list, dict);
		


			pib_header->PIBLENGTH = 0;
			break;
		}
		if (checksum32 ((char *)(memory) + offset, LE32TOH (nvm_header->ImageLength), nvm_header->ImageChecksum))
		{
			error (0, errno, NVM_IMG_CHECKSUM, filename, module);
			Py_RETURN_NONE;
		}
		offset += LE32TOH (nvm_header->ImageLength);
		module++;
	}
	while (~nvm_header->NextHeader);

	return list;
	
}