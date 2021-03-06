#ifndef APPLICATION_USER_CORE_APPTASK_H_
#define APPLICATION_USER_CORE_APPTASK_H_

#include <stdint.h>
#include <stdbool.h>

#define MAX_NODES 64
#define MAX_ENDPS 16
#define TEXTAREA_SIZE1 8192

typedef struct {
	uint8_t Endpoint;
	uint8_t SimpleDescriptorRetry;
	uint8_t Binding;
	bool C06Exists;
	bool C08Exists;
	uint8_t C06ValueRetry;
	uint8_t C08ValueRetry;
	uint8_t C06BindRetry;
	uint8_t C08BindRetry;
	bool C06Value;
	uint8_t C08Value;
} Endpoint_t;

typedef struct {
	uint8_t LqiRetry;
	uint8_t ActiveEndpointRetry;
	uint16_t Address;
	uint64_t IEEE;
	uint8_t Type;
	uint8_t EndpointCount;
	uint8_t NameRetry;
	char ManufacturerName[32];
	char ModelIdentifier[32];
	Endpoint_t Endpoints[MAX_ENDPS];
} Node_t;

typedef struct {
	uint8_t Ready;
	uint8_t DeviceType;
	uint8_t NodesCount;
	Node_t Nodes[MAX_NODES];
} Configuration_t;

typedef struct {
	uint8_t nNodes;
	uint8_t nNodesAEOk;
	uint8_t nNodesLQOk;
	uint8_t nNodesNameOk;
	uint8_t nEndpoints;
	uint8_t nEndpointsSDOk;
	uint8_t nEndpointsBindOk;
	uint8_t nEndpointsValueOk;
} Summary_t;

typedef union {
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
} Fake_t;

typedef struct {
	Endpoint_t * Endpoint;
	Node_t * Node;
} Tuple1_t;

typedef struct {
	uint64_t IEEE;
	uint8_t Endpoint;
} Tuple2_t;

#define MID_VW_LOG				0

#define RUN(FN,PAR)	\
	struct AppMessage message = { .fn = (void (*)(void*)) (&FN) }; \
	memcpy(message.params, &PAR, sizeof(PAR)); \
	xQueueSendToFront(xQueue, (void* ) &message, (TickType_t ) portMAX_DELAY);

#define RUNNOW(FN,PAR)	\
	struct AppMessage message = { .fn = (void (*)(void*)) (&FN) }; \
	memcpy(message.params, &PAR, sizeof(PAR)); \
	xQueueSendToBack(xQueue, (void* ) &message, (TickType_t ) portMAX_DELAY);

struct AppMessage {
	void (*fn)(void*);
	char params[140];
};

uint8_t appScanner(void *none);
uint8_t appStartStack(void *none);
uint8_t app_init(void *none);
uint8_t appReset(Fake_t *devType);
uint8_t appRepair();
uint8_t appPrintf(const char *fmt, ...);

#endif
