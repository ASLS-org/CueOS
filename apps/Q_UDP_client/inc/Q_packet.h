#ifndef _Q_PACKET_H_
#define _Q_PACKET_H_

#include <stdint.h>
#include <stdlib.h>
#include "Q_defs.h"

#define Q_PACKETID_BYTELENGTH 		6
#define Q_OPCODE_BYTELENGTH	  		1
#define Q_REQUESTID_BYTELENGTH		2
#define Q_CONFIGERR_BYTELENGTH		1
#define Q_NODETYPE_BYTELENGTH		1
#define Q_NODEID_BYTELENGTH			1

#define Q_HEADER_BYTELENGTH			Q_PACKETID_BYTELENGTH + Q_OPCODE_BYTELENGTH
#define Q_CONFIGREPLY_BYTELENGTH	Q_HEADER_BYTELENGTH + Q_REQUESTID_BYTELENGTH + Q_CONFIGERR_BYTELENGTH
#define Q_DISCOVERREPLY_BYTELENGTH	Q_HEADER_BYTELENGTH + Q_NODETYPE_BYTELENGTH + Q_NODEID_BYTELENGTH
#define Q_POLLREPLY_BYTELENGTH		Q_DISCOVERREPLY_BYTELENGTH

#define Q_OPCODE_INDEX		 		Q_PACKETID_BYTELENGTH
#define Q_PAYLOAD_INDEX				Q_OPCODE_INDEX

#define Q_OPCODE_TYPE_MASK			0xF0
#define Q_OPCODE_CMD_MASK			0x0F

static const char Q_PACKETID_STRING[Q_PACKETID_BYTELENGTH] = "ASLS-Q";

typedef enum{
	Q_PACKET_INVALID,
	Q_PACKET_VALID
}Q_packet_validity_e;

typedef enum{
	Q_OPCODE_TRIGGER	= 0x00,
	Q_OPCODE_CONTROL	= 0x10,
	Q_OPCODE_CONFIG		= 0x20,
	Q_OPCODE_PROBE		= 0x30,
	Q_OPCODE_REPLY		= 0x40,
}Q_packet_opcode_type_e;

typedef enum{
	Q_CMD_TRIGGER_START,
	Q_CMD_TRIGGER_PAUSE,
	Q_CMD_TRIGGER_RESUME,
	Q_CMD_TRIGGER_STOP,
	Q_CMD_CONTROL_ALLOFF,
	Q_CMD_CONTROL_RESTORE,
	Q_CMD_CONTROL_ACTUATOR,
	Q_CMD_CONFIG_LOAD_PATCH,
	Q_CMD_CONFIG_LOAD_SCENE,
	Q_CMD_CONFIG_LOAD_CHASER,
	Q_CMD_CONFIG_LOAD_EFFECT,
	Q_CMD_CONFIG_CLEAR_PATCH,
	Q_CMD_CONFIG_CLEAR_SCENE,
	Q_CMD_CONFIG_CLEAR_CHASER,
	Q_CMD_CONFIG_CLEAR_EFFECT,
	Q_CMD_PROBE_DISCOVER,
	Q_CMD_PROBE_POLL,
	Q_CMD_PROBE_DIAGNOSTIC,
	Q_CMD_REPLY_DISCOVER,
	Q_CMD_REPLY_POLL,
	Q_CMD_REPLY_DIAGNOSTIC,
	Q_CMD_REPLY_CONFIG,
}Q_packet_opcode_cmd_e;

typedef struct Q_packet{
	uint8_t opcode;
	char *raw_data;
	char *payload;
}Q_packet_s;

Q_packet_s *Q_packet_parse(void *data, uint8_t len);
char *Q_packet_forge_discover_reply(uint8_t node_id);
char *Q_packet_forge_poll_reply(uint8_t node_id);
char *Q_packet_forge_config_reply(uint16_t request_id, uint8_t err_code);
void Q_packet_free(Q_packet_s *packet);

#endif
