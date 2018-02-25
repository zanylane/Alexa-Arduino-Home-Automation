/*
 * Copyright 2010-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef aws_iot_mqtt_h
#define aws_iot_mqtt_h

#include "Arduino.h"
#include "string.h"
#include "aws_iot_config_SDK.h"
#include "aws_iot_error.h"

typedef enum {
	MQTTv31 = 3,
	MQTTv311 = 4
} MQTTv_t;

// Check baudrate
typedef enum {
	BAUD_TYPE_UNKNOWN = -1,
	BAUD_TYPE_ARDUINO = 0,
	BAUD_TYPE_LININO = 1
} Baud_t;

// Callback message status
typedef enum {
	STATUS_DEBUG = -1,
	STATUS_NORMAL = 0,
	STATUS_SHADOW_TIMEOUT = 1,
	STATUS_SHADOW_ACCEPTED = 2,
	STATUS_SHADOW_REJECTED = 3,
	STATUS_MESSAGE_OVERFLOW = 4
} Message_status_t;

// JSON key-value pair access type
typedef enum {
	GENERAL_SECTION = 0,
	DESIRED_SECTION = 1,
	REPORTED_SECTION = 2,
	DELTA_SECTION = 3
} KV_access_t;

// PublishQueue Drop Behavior type
typedef enum {
	DROP_OLDEST = 0,
	DROP_NEWEST = 1
} DropBehavior_t;

typedef void(*message_callback)(char*, unsigned int, Message_status_t);

class aws_iot_mqtt_client {
	public:
		aws_iot_mqtt_client() {
			timeout_flag = false;
			memset(rw_buf, '\0', MAX_BUF_SIZE);
			memset(msg_buf, '\0', MAX_BUF_SIZE);
			int i;
			for(i = 0; i < MAX_SUB; i++) {
				sub_group[i].is_used = false;
				sub_group[i].is_shadow_gud = false;
				sub_group[i].callback = NULL;
			}
		}
		IoT_Error_t setup(const char* client_id, bool clean_session=true, MQTTv_t MQTT_version=MQTTv311, bool useWebsocket=false);
		IoT_Error_t config(const char* host, unsigned int port, const char* cafile_path, const char* keyfile_path, const char* certfile_path);
		IoT_Error_t configWss(const char* host, unsigned int port, const char* cafile_path);
		IoT_Error_t connect(unsigned int keepalive_interval=60);
		IoT_Error_t publish(const char* topic, const char* payload, unsigned int payload_len, unsigned int qos, bool retain);
		IoT_Error_t subscribe(const char* topic, unsigned int qos, message_callback cb);
		IoT_Error_t unsubscribe(const char* topic);
		IoT_Error_t yield();
		IoT_Error_t disconnect();
		// Device shadow support
		IoT_Error_t shadow_init(const char* thingName);
		IoT_Error_t shadow_update(const char* thingName, const char* payload, unsigned int payload_len, message_callback cb, unsigned int timeout);
		IoT_Error_t shadow_get(const char* thingName, message_callback cb, unsigned int timeout);
		IoT_Error_t shadow_delete(const char* thingName, message_callback cb, unsigned int timeout);
		IoT_Error_t shadow_register_delta_func(const char* thingName, message_callback cb);
		IoT_Error_t shadow_unregister_delta_func(const char* thingName);
		// JSON key-value-pair access
		IoT_Error_t getDesiredValueByKey(const char* JSONIdentifier, const char* key, char* externalJSONBuf, unsigned int bufSize);
		IoT_Error_t getReportedValueByKey(const char* JSONIdentifier, const char* key, char* externalJSONBuf, unsigned int bufSize);
		IoT_Error_t getDeltaValueByKey(const char* JSONIdentifier, const char* key, char* externalJSONBuf, unsigned int bufSize);
		IoT_Error_t getValueByKey(const char* JSONIdentifier, const char* key, char* externalJSONBuf, unsigned int bufSize);
		// Progressive backoff configuration
		IoT_Error_t configBackoffTiming(unsigned int baseReconnectQuietTimeSecond, unsigned int maxReconnectQuietTimeSecond, unsigned int stableConnectionTimeSecond);
		// Offline publish queue configuration
		IoT_Error_t configOfflinePublishQueue(unsigned int queueSize, DropBehavior_t behavior);
		// Draining interval configuration
		IoT_Error_t configDrainingInterval(float numberOfSeconds);

	private:
		typedef struct {
			bool is_used;
			bool is_shadow_gud; // if this is a shadow get/update/delete request
			message_callback callback;
		} mqtt_sub_element;
		char rw_buf[MAX_BUF_SIZE];
		char msg_buf[MAX_BUF_SIZE]; // To store message chunks
		mqtt_sub_element sub_group[MAX_SUB];
		bool timeout_flag; // Is there a timeout when executing RPC
		Baud_t find_baud_type();
		IoT_Error_t getJSONValueLoop(const char* JSONIdentifier, const char* key, char* externalJSONBuf, unsigned int bufSize, KV_access_t accessType);
		IoT_Error_t setup_exec(const char* client_id, bool clean_session, MQTTv_t MQTT_version, bool useWebsocket);
		void exec_cmd(const char* cmd, bool wait, bool single_line);
		int find_unused_subgroup();
		void clearProtocolOnSerialBegin(long baudrate);
		bool is_num(char* src);
};

#endif
