#include <zigbee.h>

#include "cmsis_os.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <timers.h>
#include <queue.h>
#include <system.h>
#include <mtAf.h>
/********************************************************************************/
extern Configuration_t sys_cfg;
extern TimerHandle_t xTimer;
extern QueueHandle_t xQueueViewToBackend;
/********************************************************************************/
devStates_t zb_device_state = DEV_HOLD;
mtAfCb_t mtAfCb = { .pfnAfDataConfirm = NULL, //
		.pfnAfDataReqeuestSrsp = NULL, //
		.pfnAfDataRetrieveSrsp = NULL, //
		.pfnAfIncomingMsg = zb_af_incoming_msg, //
		.pfnAfIncomingMsgExt = NULL, //
		.pfnAfReflectError = NULL, //
		.pfnAfRegisterSrsp = NULL //
		};
mtSysCb_t mtSysCb = { NULL, NULL, NULL, zb_sys_reset, zb_sys_version, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
mtZdoCb_t mtZdoCb = { NULL,       // MT_ZDO_NWK_ADDR_RSP
		NULL,      // MT_ZDO_IEEE_ADDR_RSP
		NULL,      // MT_ZDO_NODE_DESC_RSP
		NULL,     // MT_ZDO_POWER_DESC_RSP
		zb_zdo_simple_descriptor,    // MT_ZDO_SIMPLE_DESC_RSP
		zb_zdo_active_endpoint,      // MT_ZDO_ACTIVE_EP_RSP
		NULL,     // MT_ZDO_MATCH_DESC_RSP
		NULL,   // MT_ZDO_COMPLEX_DESC_RSP
		NULL,      // MT_ZDO_USER_DESC_RSP
		NULL,     // MT_ZDO_USER_DESC_CONF
		NULL,    // MT_ZDO_SERVER_DISC_RSP
		NULL, // MT_ZDO_END_DEVICE_BIND_RSP
		NULL,          // MT_ZDO_BIND_RSP
		NULL,        // MT_ZDO_UNBIND_RSP
		NULL,   // MT_ZDO_MGMT_NWK_DISC_RSP
		zb_zdo_mgmt_remote_lqi,       // MT_ZDO_MGMT_LQI_RSP
		NULL,       // MT_ZDO_MGMT_RTG_RSP
		NULL,      // MT_ZDO_MGMT_BIND_RSP
		NULL,     // MT_ZDO_MGMT_LEAVE_RSP
		NULL,     // MT_ZDO_MGMT_DIRECT_JOIN_RSP
		NULL,     // MT_ZDO_MGMT_PERMIT_JOIN_RSP
		zb_zdo_state_changed,   // MT_ZDO_STATE_CHANGE_IND
		zb_zdo_end_device_announce,   // MT_ZDO_END_DEVICE_ANNCE_IND
		NULL,        // MT_ZDO_SRC_RTG_IND
		NULL,	 //MT_ZDO_BEACON_NOTIFY_IND
		NULL,			 //MT_ZDO_JOIN_CNF
		NULL,	 //MT_ZDO_NWK_DISCOVERY_CNF
		NULL,                    // MT_ZDO_CONCENTRATOR_IND_CB
		NULL,         // MT_ZDO_LEAVE_IND
		NULL,   //MT_ZDO_STATUS_ERROR_RSP
		NULL,  //MT_ZDO_MATCH_DESC_RSP_SENT
		NULL, NULL };
/********************************************************************************/
uint8_t zb_sys_reset(ResetIndFormat_t *msg) {
	printf("Reset ZNP Version: %d.%d.%d\n", msg->MajorRel, msg->MinorRel, msg->HwRev);
	return 0;
}

uint8_t zb_sys_version(VersionSrspFormat_t *msg) {
	printf("Request ZNP Version: %d.%d.%d\n", msg->MajorRel, msg->MinorRel, msg->Product);
	return 0;
}

uint8_t zb_zdo_state_changed(uint8_t newDevState) {

	switch (newDevState) {
	case DEV_HOLD:
		app_show("Hold");
		break;
	case DEV_INIT:
		app_show("Init");
		break;
	case DEV_NWK_DISC:
		app_show("Network Discovering");
		break;
	case DEV_NWK_JOINING:
		app_show("Network Joining");
		break;
	case DEV_NWK_REJOIN:
		app_show("Network Rejoining");
		break;
	case DEV_END_DEVICE_UNAUTH:
		app_show("Network Authenticating");
		break;
	case DEV_END_DEVICE:
		app_show("Network Joined as End Device");
		break;
	case DEV_ROUTER:
		app_show("Network Joined as Router");
		break;
	case DEV_COORD_STARTING:
		app_show("Network Starting");
		break;
	case DEV_ZB_COORD:
		app_show("Network Started");
		break;
	case DEV_NWK_ORPHAN:
		app_show("Network Orphaned");
		break;
	default:
		app_show("Unknown state");
		break;
	}

	zb_device_state = (devStates_t) newDevState;

	return SUCCESS;
}

uint8_t zb_zdo_explore(uint16_t address) {
	Node_t *node = zb_find_node_by_address(address);
	if(node == NULL){
		//uint8_t devType = msg->NeighborLqiList[i].DevTyp_RxOnWhenIdle_Relat & 3;
		node = &sys_cfg.Nodes[sys_cfg.NodesCount];
		sys_cfg.NodesCount++;
		node->LqiCompleted = ZB_KO;
		node->ActiveEndpointCompleted = ZB_KO;
		node->Address = address;
		//node->Type = devType;
	}
	if (node->ActiveEndpointCompleted != ZB_OK) {
		ActiveEpReqFormat_t req = { .DstAddr = node->Address, .NwkAddrOfInterest = node->Address };
		ENQUEUE(MID_ZB_ZBEE_ACTEND, ActiveEpReqFormat_t, req);
	}
	if (node->ManufacturerName[0] == 0x00 || node->ModelIdentifier[0] == 0x00) {
		DataRequestFormat_t req = { .ClusterID = 0, .DstAddr = node->Address, .DstEndpoint = 1, .SrcEndpoint = 1, .Len = 7, .Options = 0, .Radius = 0, .TransID = 1 };
		req.Data[0] = 0; //ZCL->1
		req.Data[1] = 0xFF;
		req.Data[2] = 0; //CmdId
		req.Data[3] = 0x05;
		req.Data[4] = 0x00;
		req.Data[5] = 0x04;
		req.Data[6] = 0x00;
		ENQUEUE(MID_ZB_ZBEE_DATARQ, DataRequestFormat_t, req);

	}
	if (node->LqiCompleted == ZB_OK) {
		//if (node->Type == DEVICETYPE_ROUTER || node->Type == DEVICETYPE_COORDINATOR) {
			MgmtLqiReqFormat_t req = { .DstAddr = node->Address, .StartIndex = 0 };
			ENQUEUE(MID_ZB_ZBEE_LQIREQ, MgmtLqiReqFormat_t, req);
		//} else {
		//	node->LqiCompleted = 0xFF;
		//}
	}
	return 0;
}

uint8_t zb_zdo_mgmt_remote_lqi(MgmtLqiRspFormat_t *msg) {
	if (msg->Status != MT_RPC_SUCCESS) {
		return msg->Status;
	}
	xTimerReset(xTimer, 0);
	if (msg->StartIndex + msg->NeighborLqiListCount < msg->NeighborTableEntries) {
		MgmtLqiReqFormat_t req = { .DstAddr = msg->SrcAddr, .StartIndex = msg->StartIndex + msg->NeighborLqiListCount };
		ENQUEUE(MID_ZB_ZBEE_LQIREQ, MgmtLqiReqFormat_t, req)
	}
	if (msg->StartIndex + msg->NeighborLqiListCount == msg->NeighborTableEntries) {
		Node_t *node1 = zb_find_node_by_address(msg->SrcAddr);
		node1->LqiCompleted = ZB_OK;
	}
	uint32_t iNeighbor = 0;
	for (iNeighbor = 0; iNeighbor < msg->NeighborLqiListCount; iNeighbor++) {
		uint16_t address = msg->NeighborLqiList[iNeighbor].NetworkAddress;
		//uint8_t devType = msg->NeighborLqiList[i].DevTyp_RxOnWhenIdle_Relat & 3;
		zb_zdo_explore(address);
		/*
		Node_t *node1 = zb_find_node_by_address(address);
		if (node1 != NULL) {
			return msg->Status;
		}
		printf("mtZdoMgmtLqiRspCb -> Found: %04X, Count: %d\n", address, sys_cfg.NodesCount);
		Node_t *node = &sys_cfg.Nodes[sys_cfg.NodesCount];
		sys_cfg.NodesCount++;
		node->LqiCompleted = 0x00;
		node->ActiveEndpointCompleted = 0x00;
		node->Address = address;
		node->Type = devType;
		*/
		//break;
	}
	return msg->Status;
}

uint8_t zb_zdo_simple_descriptor(SimpleDescRspFormat_t *msg) {
	if (msg->Status != MT_RPC_SUCCESS) {
		return msg->Status;
	}
	xTimerReset(xTimer, 0);
	printf("mtZdoSimpleDescRspCb -> %04X:%d, In: %d\n", msg->NwkAddr, msg->Endpoint, msg->NumInClusters);
	Node_t *node = zb_find_node_by_address(msg->NwkAddr);
	Endpoint_t *endpoint = zb_find_endpoint(node, msg->Endpoint);
	endpoint->SimpleDescriptorCompleted = ZB_OK;
	endpoint->InClusterCount = msg->NumInClusters;
	endpoint->OutClusterCount = msg->NumOutClusters;
	uint32_t i;
	for (i = 0; i < msg->NumInClusters; i++) {
		endpoint->InClusters[i].Cluster = msg->InClusterList[i];
	}
	for (i = 0; i < msg->NumOutClusters; i++) {
		endpoint->OutClusters[i].Cluster = msg->OutClusterList[i];
	}
	return msg->Status;
}

uint8_t zb_zdo_active_endpoint(ActiveEpRspFormat_t *msg) {
	if (msg->Status != MT_RPC_SUCCESS) {
		return msg->Status;
	}
	xTimerReset(xTimer, 0);
	Node_t *node = zb_find_node_by_address(msg->SrcAddr);
	node->ActiveEndpointCompleted = ZB_OK;
	node->EndpointCount = msg->ActiveEPCount;
	printf("mtZdoActiveEpRspCb -> Address: %04X, Endpoints: %d\n", msg->SrcAddr, msg->ActiveEPCount);
	uint32_t i;
	for (i = 0; i < msg->ActiveEPCount; i++) {
		uint8_t endpoint = msg->ActiveEPList[i];
		node->Endpoints[i].Endpoint = endpoint;
		node->Endpoints[i].SimpleDescriptorCompleted = ZB_KO;
		SimpleDescReqFormat_t req = { .DstAddr = msg->NwkAddr, .NwkAddrOfInterest = msg->NwkAddr, .Endpoint = endpoint };
		ENQUEUE(MID_ZB_ZBEE_SIMDES, SimpleDescReqFormat_t, req)
	}
	return msg->Status;

}

uint8_t zb_af_incoming_msg(IncomingMsgFormat_t *msg) {
	if (msg->ClusterId == 0 && msg->Len > 1 && msg->Data[1] == 0xFF) {
		Node_t *node = zb_find_node_by_address(msg->SrcAddr);
		uint8_t j = 3;
		while (j + 2 < msg->Len) {
			uint16_t currentAttributeId = msg->Data[j++];
			currentAttributeId |= msg->Data[j++] << 8;
			uint8_t success = msg->Data[j++];
			if (success == 0) {
				uint8_t zigbeeType = msg->Data[j++];
				if (zigbeeType == 66) {
					uint8_t strlen = msg->Data[j++];
					uint8_t *str = &(msg->Data[j]);
					if (currentAttributeId == 5) {
						memcpy(node->ModelIdentifier, str, strlen);
					} else {
						memcpy(node->ManufacturerName, str, strlen);
					}
					j += strlen;
				}
			}
		}
		xTimerReset(xTimer, 0);

	}
	return MT_RPC_SUCCESS;
}

Endpoint_t* zb_find_endpoint(Node_t *node, uint8_t endpoint) {
	uint8_t i1;
	for (i1 = 0; i1 < node->EndpointCount; ++i1) {
		if (node->Endpoints[i1].Endpoint == endpoint) {
			return &node->Endpoints[i1];
		}
	}
	return NULL;
}

Node_t* zb_find_node_by_address(uint16_t address) {
	uint8_t i1;
	for (i1 = 0; i1 < sys_cfg.NodesCount; ++i1) {
		if (sys_cfg.Nodes[i1].IEEE == address) {
			return &sys_cfg.Nodes[i1];
		}
	}
	return NULL;
}

Node_t* zb_find_node_by_ieee(uint64_t ieee) {
	uint8_t i1;
	for (i1 = 0; i1 < sys_cfg.NodesCount; ++i1) {
		if (sys_cfg.Nodes[i1].Address == ieee) {
			return &sys_cfg.Nodes[i1];
		}
	}
	return NULL;
}


uint8_t zb_zdo_end_device_announce(EndDeviceAnnceIndFormat_t *msg) {
	printf("Joined: 0x%04X\n", msg->NwkAddr);
	zb_zdo_explore(msg->NwkAddr);
	return 0x00;
}

void zb_init() {
	sysRegisterCallbacks(mtSysCb);
	zdoRegisterCallbacks(mtZdoCb);
	afRegisterCallbacks(mtAfCb);
}
