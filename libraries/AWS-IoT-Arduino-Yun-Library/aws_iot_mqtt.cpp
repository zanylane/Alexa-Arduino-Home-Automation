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

#include "aws_iot_mqtt.h"
#include "Arduino.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "ctype.h"
#include "avr/pgmspace.h"

#define LINUX_BAUD_DEFAULT 250000
#define LINUX_BAUD_LININO 115200
#define RETURN_KEY 13 // ASCII code for '\r'
#define NEXTLINE_KEY 10 // ASCII code for '\n'
#define MAX_NUM_PARA 6 // Maximum number of parameters in protocol communication
#define NUM_ATTEMPT_BEFORE_EXIT MAX_NUM_PARA/2+1 // Number of '~' to fully exit the protocol command

// PGM_P is defined as const char*
// Helper string constants
PGM_P OUT_OF_BUFFER_ERR_MSG = "OUT OF BUFFER SIZE";
PGM_P EMPTY_STRING = "";
// Internal non-protocol commands
PGM_P CMD_CHECK_LINUX_LIVE = "uname\n";
PGM_P CMD_CD_TO_PY_RUNTIME = "cd /root/AWS-IoT-Python-Runtime/runtime/\n";
PGM_P CMD_START_PY_RUNTIME = "python run.py\n";

// Choose different baudrate for different version of openWRT OS
Baud_t aws_iot_mqtt_client::find_baud_type() {
	Baud_t rc_type = BAUD_TYPE_UNKNOWN;
	// 1st attempt
	clearProtocolOnSerialBegin(LINUX_BAUD_DEFAULT);
	exec_cmd(CMD_CHECK_LINUX_LIVE, true, false); // check OS version
	if(strncmp_P(rw_buf, PSTR("Linux"), 5) != 0) { // Not an Arduino?
		clearProtocolOnSerialBegin(LINUX_BAUD_LININO);
		exec_cmd(CMD_CHECK_LINUX_LIVE, true, false); // check OS version
		if(strncmp_P(rw_buf, PSTR("Linux"), 5) != 0) {
			// No more board types to try
		}
		else {rc_type = BAUD_TYPE_LININO;}
	}
	else {rc_type = BAUD_TYPE_ARDUINO;}

	return rc_type;
}

IoT_Error_t aws_iot_mqtt_client::setup_exec(const char* client_id, bool clean_session, MQTTv_t MQTT_version, bool useWebsocket) {
	// Serial1 is started before this call
	IoT_Error_t rc = NONE_ERROR;
	exec_cmd(CMD_CD_TO_PY_RUNTIME, false, false);
	exec_cmd(CMD_START_PY_RUNTIME, false, false);

	// Create obj
	exec_cmd("5\n", false, false);

	exec_cmd("i\n", false, false);

	snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%s\n"), client_id);
	exec_cmd(rw_buf, false, false);

	int num_temp = clean_session ? 1 : 0;
	snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), num_temp);
	exec_cmd(rw_buf, false, false);

	snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%u\n"), MQTT_version);
	exec_cmd(rw_buf, false, false);

	// Websocket flag
	num_temp = useWebsocket ? 1 : 0;
	snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), num_temp);
	exec_cmd(rw_buf, true, false);

	if(strncmp_P(rw_buf, PSTR("I T"), 3) != 0) {
		if(strncmp_P(rw_buf, PSTR("I F"), 3) == 0) {rc = SET_UP_ERROR;}
		else rc = GENERIC_ERROR;
	}

	return rc;
}

IoT_Error_t aws_iot_mqtt_client::setup(const char* client_id, bool clean_session, MQTTv_t MQTT_version, bool useWebsocket) {
	IoT_Error_t rc = NONE_ERROR;
	if(client_id == NULL) {rc = NULL_VALUE_ERROR;}
	else if(strlen(client_id) >= MAX_BUF_SIZE) {rc = OVERFLOW_ERROR;}
	// No input error below this line
	else {
		Baud_t baud_type = find_baud_type(); // Find out baud type
		// Communication failed due to baud rate issue
		if(BAUD_TYPE_UNKNOWN == baud_type) {rc = SERIAL1_COMMUNICATION_ERROR;}
		else {
			rc = setup_exec(client_id, clean_session, MQTT_version, useWebsocket);
		}
	}

	return rc;
}

IoT_Error_t aws_iot_mqtt_client::configWss(const char* host, unsigned int port, const char* cafile_path) {
	return config(host, port, cafile_path, "", ""); // No need for key and cert, IAM credentials are used.
}

IoT_Error_t aws_iot_mqtt_client::config(const char* host, unsigned int port, const char* cafile_path, const char* keyfile_path, const char* certfile_path) {
	IoT_Error_t rc = NONE_ERROR;

	if(host != NULL && strlen(host) >= MAX_BUF_SIZE) {rc = OVERFLOW_ERROR;}
	else if(cafile_path != NULL && strlen(cafile_path) >= MAX_BUF_SIZE) {rc = OVERFLOW_ERROR;}
	else if(keyfile_path != NULL && strlen(keyfile_path) >= MAX_BUF_SIZE) {rc = OVERFLOW_ERROR;}
	else if(certfile_path != NULL && strlen(certfile_path) >= MAX_BUF_SIZE) {rc = OVERFLOW_ERROR;}
	else {
		PGM_P helper = "";

		exec_cmd("6\n", false, false);

		exec_cmd("g\n", false, false);

		helper = host == NULL ? EMPTY_STRING : host;
		snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%s\n"), helper);
		exec_cmd(rw_buf, false, false);

		snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), port);
		exec_cmd(rw_buf, false, false);

		helper = cafile_path == NULL ? EMPTY_STRING : cafile_path;
		snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%s\n"), helper);
		exec_cmd(rw_buf, false, false);

		helper = keyfile_path == NULL ? EMPTY_STRING : keyfile_path;
		snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%s\n"), helper);
		exec_cmd(rw_buf, false, false);

		helper = certfile_path == NULL ? EMPTY_STRING : certfile_path;
		snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%s\n"), helper);
		exec_cmd(rw_buf, true, false);

		if(strncmp_P(rw_buf, PSTR("G T"), 3) != 0) {
			if(strncmp_P(rw_buf, PSTR("G1F"), 3) == 0) {rc = NO_SET_UP_ERROR;}
			else if(strncmp_P(rw_buf, PSTR("G2F"), 3) == 0) {rc = WRONG_PARAMETER_ERROR;}
			else if(strncmp_P(rw_buf, PSTR("GFF"), 3) == 0) {rc = CONFIG_GENERIC_ERROR;}
			else rc = GENERIC_ERROR;
		}
	}

	return rc;
}

IoT_Error_t aws_iot_mqtt_client::configBackoffTiming(unsigned int baseReconnectQuietTimeSecond, unsigned int maxReconnectQuietTimeSecond, unsigned int stableConnectionTimeSecond) {
	IoT_Error_t rc = NONE_ERROR;

	exec_cmd("4\n", false, false);

	exec_cmd("bf\n", false, false);

	snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), baseReconnectQuietTimeSecond);
	exec_cmd(rw_buf, false, false);

	snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), maxReconnectQuietTimeSecond);
	exec_cmd(rw_buf, false, false);

	snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), stableConnectionTimeSecond);
	exec_cmd(rw_buf, true, false);

	if(strncmp_P(rw_buf, PSTR("BF T"), 4) != 0) {
		if(strncmp_P(rw_buf, PSTR("BF1F"), 4) == 0) {rc = NO_SET_UP_ERROR;}
		else if(strncmp_P(rw_buf, PSTR("BF2F"), 4) == 0) {rc = WRONG_PARAMETER_ERROR;}
		else if(strncmp_P(rw_buf, PSTR("BF3F"), 4) == 0) {rc = WRONG_PARAMETER_ERROR;}
		else if(strncmp_P(rw_buf, PSTR("BFFF"), 4) == 0) {rc = CONFIG_GENERIC_ERROR;}
		else rc = GENERIC_ERROR;
	}

	return rc;
}

IoT_Error_t aws_iot_mqtt_client::configOfflinePublishQueue(unsigned int queueSize, DropBehavior_t behavior) {
	IoT_Error_t rc = NONE_ERROR;

	exec_cmd("3\n", false, false);

	exec_cmd("pq\n", false, false);

	snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), queueSize);
	exec_cmd(rw_buf, false, false);


	snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), behavior);
	exec_cmd(rw_buf, true, false);

	if(strncmp_P(rw_buf, PSTR("PQ T"), 4) != 0) {
		if(strncmp_P(rw_buf, PSTR("PQ1F"), 4) == 0) {rc = NO_SET_UP_ERROR;}
		else if(strncmp_P(rw_buf, PSTR("PQ2F"), 4) == 0) {rc = WRONG_PARAMETER_ERROR;}
		else if(strncmp_P(rw_buf, PSTR("PQ3F"), 4) == 0) {rc = WRONG_PARAMETER_ERROR;}
		else if(strncmp_P(rw_buf, PSTR("PQFF"), 4) == 0) {rc = CONFIG_GENERIC_ERROR;}
		else rc = GENERIC_ERROR;
	}

	return rc;
}

IoT_Error_t aws_iot_mqtt_client::configDrainingInterval(float numberOfSeconds) {
	IoT_Error_t rc = NONE_ERROR;

	exec_cmd("2\n", false, false);

	exec_cmd("di\n", false, false);

	dtostrf(numberOfSeconds, 5, 2, rw_buf); // Only support XX.XX (including sign+/-)
	strcat(rw_buf, "\n");
	exec_cmd(rw_buf, true, false);

	if(strncmp_P(rw_buf, PSTR("DI T"), 4) != 0) {
		if(strncmp_P(rw_buf, PSTR("DI1F"), 4) == 0) {rc = NO_SET_UP_ERROR;}
		else if(strncmp_P(rw_buf, PSTR("DI2F"), 4) == 0) {rc = WRONG_PARAMETER_ERROR;}
		else if(strncmp_P(rw_buf, PSTR("DI3F"), 4) == 0) {rc = WRONG_PARAMETER_ERROR;}
		else if(strncmp_P(rw_buf, PSTR("DIFF"), 4) == 0) {rc = CONFIG_GENERIC_ERROR;}
		else rc = GENERIC_ERROR;
	}

	return rc;
}

IoT_Error_t aws_iot_mqtt_client::connect(unsigned int keepalive_interval) {
	IoT_Error_t rc = NONE_ERROR;
	exec_cmd("2\n", false, false);

	exec_cmd("c\n", false, false);

	snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), keepalive_interval);
	exec_cmd(rw_buf, true, false);

	if(strncmp_P(rw_buf, PSTR("C T"), 3) != 0) {
		if(strncmp_P(rw_buf, PSTR("C1F"), 3) == 0) {rc = NO_SET_UP_ERROR;}
		else if(strncmp_P(rw_buf, PSTR("C2F"), 3) == 0) {rc = WRONG_PARAMETER_ERROR;}
		else if(strncmp_P(rw_buf, PSTR("C3F"), 3) == 0) {rc = CONNECT_SSL_ERROR;}
		else if(strncmp_P(rw_buf, PSTR("C4F"), 3) == 0) {rc = CONNECT_ERROR;}
		else if(strncmp_P(rw_buf, PSTR("C5F"), 3) == 0) {rc = CONNECT_TIMEOUT;}
		else if(strncmp_P(rw_buf, PSTR("C6F"), 3) == 0) {rc = CONNECT_CREDENTIAL_NOT_FOUND;}
		else if(strncmp_P(rw_buf, PSTR("C7F"), 3) == 0) {rc = WEBSOCKET_CREDENTIAL_NOT_FOUND;}
		else if(strncmp_P(rw_buf, PSTR("CFF"), 3) == 0) {rc = CONNECT_GENERIC_ERROR;}
		else rc = GENERIC_ERROR;
	}

	return rc;
}

IoT_Error_t aws_iot_mqtt_client::publish(const char* topic, const char* payload, unsigned int payload_len, unsigned int qos, bool retain) {
	IoT_Error_t rc = NONE_ERROR;
	if(topic == NULL || payload == NULL) {rc = NULL_VALUE_ERROR;}
	else if(strlen(topic) >= MAX_BUF_SIZE || payload_len >= MAX_BUF_SIZE) {rc = OVERFLOW_ERROR;}
	else {
		exec_cmd("5\n", false, false);

		exec_cmd("p\n", false, false);

		snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%s\n"), topic);
		exec_cmd(rw_buf, false, false);

		snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%s\n"), payload);
		exec_cmd(rw_buf, false, false);

		snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), qos);
		exec_cmd(rw_buf, false, false);

		int num_temp = retain ? 1 : 0;
		snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), num_temp);
		exec_cmd(rw_buf, true, false);

		if(strncmp_P(rw_buf, PSTR("P T"), 3) != 0) {
			if(strncmp_P(rw_buf, PSTR("P1F"), 3) == 0) {rc = NO_SET_UP_ERROR;}
			else if(strncmp_P(rw_buf, PSTR("P2F"), 3) == 0) {rc = WRONG_PARAMETER_ERROR;}
			else if(strncmp_P(rw_buf, PSTR("P3F"), 3) == 0) {rc = PUBLISH_ERROR;}
			else if(strncmp_P(rw_buf, PSTR("P4F"), 3) == 0) {rc = PUBLISH_TIMEOUT;}
			else if(strncmp_P(rw_buf, PSTR("P5F"), 3) == 0) {rc = PUBLISH_QUEUE_FULL;}
			else if(strncmp_P(rw_buf, PSTR("P6F"), 3) == 0) {rc = PUBLISH_QUEUE_DISABLED;}
			else if(strncmp_P(rw_buf, PSTR("PFF"), 3) == 0) {rc = PUBLISH_GENERIC_ERROR;}
			else rc = GENERIC_ERROR;
		}
	}
	return rc;
}

IoT_Error_t aws_iot_mqtt_client::subscribe(const char* topic, unsigned int qos, message_callback cb) {
	IoT_Error_t rc = NONE_ERROR;
	if(topic == NULL) {rc = NULL_VALUE_ERROR;}
	else if(strlen(topic) >= MAX_BUF_SIZE) {rc = OVERFLOW_ERROR;}
	else {
		// find unused slots for new subscribe
		int i = find_unused_subgroup();
		if(i < MAX_SUB) {
			exec_cmd("4\n", false, false);

			exec_cmd("s\n", false, false);

			snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%s\n"), topic);
			exec_cmd(rw_buf, false, false);

			snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), qos);
			exec_cmd(rw_buf, false, false);

			snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), i); // ino_id
			exec_cmd(rw_buf, true, false);

			if(strncmp_P(rw_buf, PSTR("S T"), 3) == 0) {
				sub_group[i].is_used = true;
				sub_group[i].is_shadow_gud = false;
				sub_group[i].callback = cb;
			}
			else {
				if(strncmp_P(rw_buf, PSTR("S1F"), 3) == 0) {rc = NO_SET_UP_ERROR;}
				else if(strncmp_P(rw_buf, PSTR("S2F"), 3) == 0) {rc = WRONG_PARAMETER_ERROR;}
				else if(strncmp_P(rw_buf, PSTR("S3F"), 3) == 0) {rc = SUBSCRIBE_ERROR;}
				else if(strncmp_P(rw_buf, PSTR("S4F"), 3) == 0) {rc = SUBSCRIBE_TIMEOUT;}
				else if(strncmp_P(rw_buf, PSTR("SFF"), 3) == 0) {rc = SUBSCRIBE_GENERIC_ERROR;}
				else rc = GENERIC_ERROR;
			}
		}
		else {rc = OUT_OF_SKETCH_SUBSCRIBE_MEMORY;}
	}
	return rc;
}

IoT_Error_t aws_iot_mqtt_client::unsubscribe(const char* topic) {
	IoT_Error_t rc = NONE_ERROR;
	if(topic == NULL) {rc = NULL_VALUE_ERROR;}
	else if(strlen(topic) >= MAX_BUF_SIZE) {rc = OVERFLOW_ERROR;}
	else {
		exec_cmd("2\n", false, false);

		exec_cmd("u\n", false, false);

		snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%s\n"), topic);
		exec_cmd(rw_buf, true, false);

		// Unsubscribe to a topic never subscribed, ignore
		if(strncmp_P(rw_buf, PSTR("U T"), 3) == 0) {rc = NONE_ERROR;}
		else {
			char* saveptr;
			char* p;
			p = strtok_r(rw_buf, " ", &saveptr); // 'U'
			p = strtok_r(NULL, " ", &saveptr); // ino_id
			int ino_id = -1;
			if(p != NULL) {ino_id = is_num(p) ? atoi(p) : -1;}
			if(ino_id >= 0 && ino_id < MAX_SUB) {
				sub_group[ino_id].is_used = false;
				sub_group[ino_id].is_shadow_gud = false;
				sub_group[ino_id].callback = NULL;
			}
			else if(strncmp_P(rw_buf, PSTR("U1F"), 3) == 0) {rc = NO_SET_UP_ERROR;}
			else if(strncmp_P(rw_buf, PSTR("U2F"), 3) == 0) {rc = WRONG_PARAMETER_ERROR;}
			else if(strncmp_P(rw_buf, PSTR("U3F"), 3) == 0) {rc = UNSUBSCRIBE_ERROR;}
			else if(strncmp_P(rw_buf, PSTR("U4F"), 3) == 0) {rc = UNSUBSCRIBE_TIMEOUT;}
			else if(strncmp_P(rw_buf, PSTR("UFF"), 3) == 0) {rc = UNSUBSCRIBE_GENERIC_ERROR;}
			else rc = GENERIC_ERROR;
		}
	}
	return rc;
}

IoT_Error_t aws_iot_mqtt_client::yield() {
	IoT_Error_t rc = NONE_ERROR;
	exec_cmd("1\n", false, false);
	exec_cmd("z\n", true, false); // tell the python runtime to lock the current msg queue size
	if(strncmp_P(rw_buf, PSTR("Z T"), 3) != 0) {rc = YIELD_ERROR;} // broken protocol
	else { // start the BIG yield loop
		while(true) {
			exec_cmd("1\n", false, false);
			exec_cmd("y\n", true, false);
			if(strncmp_P(rw_buf, PSTR("Y F"), 3) == 0) {break;}
			if(rw_buf[0] != 'Y') { // filter out garbage feedback
				rc = YIELD_ERROR;
				break;
			}
			// From here, there is a new message chunk in rw_buf
			char* saveptr;
			char* p;
			p = strtok_r(rw_buf, " ", &saveptr); // 'Y'
			p = strtok_r(NULL, " ", &saveptr); // ino_id
			if(p != NULL) {
			  	int ino_id = is_num(p) ? atoi(p) : -1;
			    size_t id_len = strlen(p);
			    p = strtok_r(NULL, " ", &saveptr); // more chunks?
			    if(p != NULL) {
			      	int more = is_num(p) ? atoi(p) : -1;
			      	if(more != 1 && more != 0) { // broken protocol
			      		rc = YIELD_ERROR;
			      		break;
			      	}
			      	else if(ino_id == -1) {
			      		rc = YIELD_ERROR;
			      		break;
			      	}
			      	else {
			      		char* payload = rw_buf + id_len + 5; // step over the protocol and get payload
			      		if(strlen(msg_buf) + strlen(payload) > MAX_BUF_SIZE) {
			      			rc = OVERFLOW_ERROR; // if it is exceeding MAX_BUF_SIZE, return the corresponding error code
			      		}
			      		else {strcat(msg_buf, payload);}
			      		if(more == 0) { // This is the end of this message, do callback and clean up
						    // user callback, watch out for ino_id boundary issue and callback registration
						    if(ino_id >= 0 && ino_id < MAX_SUB && sub_group[ino_id].is_used) {
                                // User callback
                                if(sub_group[ino_id].callback != NULL) {
									if(rc == NONE_ERROR) {
										if(sub_group[ino_id].is_shadow_gud) {
											// See if it is timeout
											if(strncmp_P(msg_buf, PSTR("JSON-X"), 6) == 0) {sub_group[ino_id].callback(msg_buf, (unsigned int)strlen(msg_buf), STATUS_SHADOW_TIMEOUT);}
											else {
												// See if it is accepted/rejected
												// Delta is treated as normal MQTT messages
												int type_num = atoi(msg_buf+5);
												if(type_num%3 == 0) {sub_group[ino_id].callback(msg_buf, (unsigned int)strlen(msg_buf), STATUS_SHADOW_ACCEPTED);} // accepted
												else if(type_num%3 == 1) {sub_group[ino_id].callback(msg_buf, (unsigned int)strlen(msg_buf), STATUS_SHADOW_REJECTED);} // rejected
												else {
													rc = YIELD_ERROR;
													break;
												}
											}
										}
										else {sub_group[ino_id].callback(msg_buf, (unsigned int)strlen(msg_buf), STATUS_NORMAL);}
									}
									if(rc == OVERFLOW_ERROR) {
										sub_group[ino_id].callback((char*)(OUT_OF_BUFFER_ERR_MSG), (unsigned int)strlen(OUT_OF_BUFFER_ERR_MSG), STATUS_MESSAGE_OVERFLOW);
									}
								}
								// always free the shadow slot and recover the context
								if(sub_group[ino_id].is_shadow_gud) {
									sub_group[ino_id].is_used = false;
									sub_group[ino_id].is_shadow_gud = false;
									sub_group[ino_id].callback = NULL;
								}
						    }
						    // clean up
						    msg_buf[0] = '\0'; // mark msg_buf as 'unused', ready for the next flush
			      		}
			      		// more to come? do NOTHING to msg_buf and DO NOT call callback
			      	}
			    }
			    else {
			      	rc = YIELD_ERROR;
			      	break;
			    }
			}
			else {
			    rc = YIELD_ERROR;
			    break;
			}
		}
	}
	return rc;
}

IoT_Error_t aws_iot_mqtt_client::disconnect() {
	IoT_Error_t rc = NONE_ERROR;
	exec_cmd("1\n", false, false);

	exec_cmd("d\n", true, false);

	if(strncmp_P(rw_buf, PSTR("D T"), 3) != 0) {
		if(strncmp_P(rw_buf, PSTR("D1F"), 3) == 0) {rc = NO_SET_UP_ERROR;}
		else if(strncmp_P(rw_buf, PSTR("D2F"), 3) == 0) {rc = DISCONNECT_ERROR;}
		else if(strncmp_P(rw_buf, PSTR("D3F"), 3) == 0) {rc = DISCONNECT_TIMEOUT;}
		else if(strncmp_P(rw_buf, PSTR("DFF"), 3) == 0) {rc = DISCONNECT_GENERIC_ERROR;}
		else rc = GENERIC_ERROR;
	}
	return rc;
}

// DeviceShadow-support API
IoT_Error_t aws_iot_mqtt_client::shadow_init(const char* thingName) {
	IoT_Error_t rc = NONE_ERROR;
	if(thingName == NULL) {rc = NULL_VALUE_ERROR;}
	else if(strlen(thingName) >= MAX_BUF_SIZE) {rc = OVERFLOW_ERROR;}
	else {
		exec_cmd("3\n", false, false);

		exec_cmd("si\n", false, false);

		snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%s\n"), thingName);
		exec_cmd(rw_buf, false, false);

		exec_cmd("1\n", true, false); // isPersistentSubscribe, always true

		if(strncmp_P(rw_buf, PSTR("SI T"), 4) != 0) {
			if(strncmp_P(rw_buf, PSTR("SI F"), 4) == 0) {rc = SHADOW_INIT_ERROR;}
			else rc = GENERIC_ERROR;
		}
	}
	return rc;
}

IoT_Error_t aws_iot_mqtt_client::shadow_register_delta_func(const char* thingName, message_callback cb) {
	IoT_Error_t rc = NONE_ERROR;
	if(thingName == NULL) {rc = NULL_VALUE_ERROR;}
	else if(strlen(thingName) >= MAX_BUF_SIZE) {rc = OVERFLOW_ERROR;}
	else {
		// find unused slots for new subscribe
		int i = find_unused_subgroup();
		if(i < MAX_SUB) {
			exec_cmd("3\n", false, false);

			exec_cmd("s_rd\n", false, false);

			snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%s\n"), thingName);
			exec_cmd(rw_buf, false, false);

			snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), i);
			exec_cmd(rw_buf, true, false);

			if(strncmp_P(rw_buf, PSTR("S_RD T"), 6) == 0) {
				sub_group[i].is_used = true;
				sub_group[i].callback = cb;
			}
			else {
				if(strncmp_P(rw_buf, PSTR("S_RD1F"), 6) == 0) {rc = NO_SHADOW_INIT_ERROR;}
				else if(strncmp_P(rw_buf, PSTR("S_RD2F"), 6) == 0) {rc = WRONG_PARAMETER_ERROR;}
				else if(strncmp_P(rw_buf, PSTR("S_RD3F"), 6) == 0) {rc = SUBSCRIBE_ERROR;}
				else if(strncmp_P(rw_buf, PSTR("S_RD4F"), 6) == 0) {rc = SUBSCRIBE_TIMEOUT;}
				else if(strncmp_P(rw_buf, PSTR("S_RDFF"), 6) == 0) {rc = SHADOW_REGISTER_DELTA_CALLBACK_GENERIC_ERROR;}
				else rc = GENERIC_ERROR;
			}
		}
	}
	return rc;
}

IoT_Error_t aws_iot_mqtt_client::shadow_unregister_delta_func(const char* thingName) {
	IoT_Error_t rc = NONE_ERROR;
	if(thingName == NULL) {rc = NULL_VALUE_ERROR;}
	else if(strlen(thingName) >= MAX_BUF_SIZE) {rc = OVERFLOW_ERROR;}
	else {
		exec_cmd("2\n", false, false);

		exec_cmd("s_ud\n", false, false);

		snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%s\n"), thingName);
		exec_cmd(rw_buf, true, false);

		// Unsubscribe to a topic never subscribed, ignore
		if(strncmp_P(rw_buf, PSTR("S_UD T"), 6) == 0) {rc = NONE_ERROR;}
		else {
			char* saveptr;
			char* p;
			p = strtok_r(rw_buf, " ", &saveptr); // 'S_UD'
			p = strtok_r(NULL, " ", &saveptr); // ino_id
			int ino_id = -1;
			if(p != NULL) {ino_id = is_num(p) ? atoi(p) : -1;}
			if(ino_id >= 0 && ino_id < MAX_SUB) {
				sub_group[ino_id].is_used = false;
				sub_group[ino_id].callback = NULL;
			}
			else if(strncmp_P(rw_buf, PSTR("S_UD1F"), 6) == 0) {rc = NO_SHADOW_INIT_ERROR;}
			else if(strncmp_P(rw_buf, PSTR("S_UD2F"), 6) == 0) {rc = WRONG_PARAMETER_ERROR;}
			else if(strncmp_P(rw_buf, PSTR("S_UD3F"), 6) == 0) {rc = UNSUBSCRIBE_ERROR;}
			else if(strncmp_P(rw_buf, PSTR("S_UD4F"), 6) == 0) {rc = UNSUBSCRIBE_TIMEOUT;}
			else if(strncmp_P(rw_buf, PSTR("S_UDFF"), 6) == 0) {rc = SHADOW_UNREGISTER_DELTA_CALLBACK_GENERIC_ERROR;}
			else rc = GENERIC_ERROR;
		}
	}
	return rc;
}

IoT_Error_t aws_iot_mqtt_client::shadow_get(const char* thingName, message_callback cb, unsigned int timeout) {
	IoT_Error_t rc = NONE_ERROR;
	if(thingName == NULL) {rc = NULL_VALUE_ERROR;}
	else if(strlen(thingName) >= MAX_BUF_SIZE) {rc = OVERFLOW_ERROR;}
	else {
		// find unused slots for new subscribe
		int i = find_unused_subgroup();	    
		if(i < MAX_SUB) {
			exec_cmd("4\n", false, false);

			exec_cmd("sg\n", false, false);

			snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%s\n"), thingName);
			exec_cmd(rw_buf, false, false);

			snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), i);
			exec_cmd(rw_buf, false, false);

			snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), timeout);
			exec_cmd(rw_buf, true, false);
	        
			if(strncmp_P(rw_buf, PSTR("SG T"), 4) == 0) {
				sub_group[i].is_used = true;
				sub_group[i].is_shadow_gud = true;
				sub_group[i].callback = cb;
			}
			else {
				if(strncmp_P(rw_buf, PSTR("SG1F"), 4) == 0) {rc = NO_SHADOW_INIT_ERROR;}
				else if(strncmp_P(rw_buf, PSTR("SG2F"), 4) == 0) {rc = WRONG_PARAMETER_ERROR;}
				else if(strncmp_P(rw_buf, PSTR("SG3F"), 4) == 0) {rc = SUBSCRIBE_ERROR;}
				else if(strncmp_P(rw_buf, PSTR("SG4F"), 4) == 0) {rc = SUBSCRIBE_TIMEOUT;}
				else if(strncmp_P(rw_buf, PSTR("SG5F"), 4) == 0) {rc = PUBLISH_ERROR;}
				else if(strncmp_P(rw_buf, PSTR("SG6F"), 4) == 0) {rc = PUBLISH_TIMEOUT;}
				else if(strncmp_P(rw_buf, PSTR("SG7F"), 4) == 0) {rc = PUBLISH_QUEUE_FULL;}
				else if(strncmp_P(rw_buf, PSTR("SG8F"), 4) == 0) {rc = PUBLISH_QUEUE_DISABLED;}
				else if(strncmp_P(rw_buf, PSTR("SGFF"), 4) == 0) {rc = SHADOW_GET_GENERIC_ERROR;}
				else rc = GENERIC_ERROR;
			}
		}
		else {rc = OUT_OF_SKETCH_SUBSCRIBE_MEMORY;}
	}
	return rc;
}

IoT_Error_t aws_iot_mqtt_client::shadow_update(const char* thingName, const char* payload, unsigned int payload_len, message_callback cb, unsigned int timeout) {
	IoT_Error_t rc = NONE_ERROR;
	if(thingName == NULL || payload == NULL) {rc = NULL_VALUE_ERROR;}
	else if(strlen(thingName) >= MAX_BUF_SIZE || payload_len >= MAX_BUF_SIZE) {rc = OVERFLOW_ERROR;}
	else {
		// find unused slots for new subscribe
		int i = find_unused_subgroup();
		if(i < MAX_SUB) {
			exec_cmd("5\n", false, false);

			exec_cmd("su\n", false, false);

			snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%s\n"), thingName);
			exec_cmd(rw_buf, false, false);

			snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%s\n"), payload);
			exec_cmd(rw_buf, false, false);

			snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), i);
			exec_cmd(rw_buf, false, false);

			snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), timeout);
			exec_cmd(rw_buf, true, false);

			if(strncmp_P(rw_buf, PSTR("SU T"), 4) == 0) {
				sub_group[i].is_used = true;
				sub_group[i].is_shadow_gud = true;
				sub_group[i].callback = cb;
			}
			else {
				if(strncmp_P(rw_buf, PSTR("SU1F"), 4) == 0) {rc = NO_SHADOW_INIT_ERROR;}
				else if(strncmp_P(rw_buf, PSTR("SU2F"), 4) == 0) {rc = WRONG_PARAMETER_ERROR;}
				else if(strncmp_P(rw_buf, PSTR("SU3F"), 4) == 0) {rc = SHADOW_UPDATE_INVALID_JSON_ERROR;}
				else if(strncmp_P(rw_buf, PSTR("SU4F"), 4) == 0) {rc = SUBSCRIBE_ERROR;}
				else if(strncmp_P(rw_buf, PSTR("SU5F"), 4) == 0) {rc = SUBSCRIBE_TIMEOUT;}
				else if(strncmp_P(rw_buf, PSTR("SU6F"), 4) == 0) {rc = PUBLISH_ERROR;}
				else if(strncmp_P(rw_buf, PSTR("SU7F"), 4) == 0) {rc = PUBLISH_TIMEOUT;}
				else if(strncmp_P(rw_buf, PSTR("SU8F"), 4) == 0) {rc = PUBLISH_QUEUE_FULL;}
				else if(strncmp_P(rw_buf, PSTR("SU9F"), 4) == 0) {rc = PUBLISH_QUEUE_DISABLED;}
				else if(strncmp_P(rw_buf, PSTR("SUFF"), 4) == 0) {rc = SHADOW_UPDATE_GENERIC_ERROR;}
				else rc = GENERIC_ERROR;
			}
		}	        
		else {rc = OUT_OF_SKETCH_SUBSCRIBE_MEMORY;}
	}
	return rc;
}

IoT_Error_t aws_iot_mqtt_client::shadow_delete(const char* thingName, message_callback cb, unsigned int timeout) {
	IoT_Error_t rc = NONE_ERROR;
	if(thingName == NULL) {rc = NULL_VALUE_ERROR;}
	else if(strlen(thingName) >= MAX_BUF_SIZE) {rc = OVERFLOW_ERROR;}
	else {
		// find unused slots for new subscribe
		int i = find_unused_subgroup();
		if(i < MAX_SUB) {
			exec_cmd("4\n", false, false);

			exec_cmd("sd\n", false, false);

			snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%s\n"), thingName);
			exec_cmd(rw_buf, false, false);

			snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), i);
			exec_cmd(rw_buf, false, false);

			snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), timeout);
			exec_cmd(rw_buf, true, false);
			if(strncmp_P(rw_buf, PSTR("SD T"), 4) == 0) {
				sub_group[i].is_used = true;
				sub_group[i].is_shadow_gud = true;
				sub_group[i].callback = cb;
			}
			else {
				if(strncmp_P(rw_buf, PSTR("SD1F"), 4) == 0) {rc = NO_SHADOW_INIT_ERROR;}
				else if(strncmp_P(rw_buf, PSTR("SD2F"), 4) == 0) {rc = WRONG_PARAMETER_ERROR;}
				else if(strncmp_P(rw_buf, PSTR("SD3F"), 4) == 0) {rc = SUBSCRIBE_ERROR;}
				else if(strncmp_P(rw_buf, PSTR("SD4F"), 4) == 0) {rc = SUBSCRIBE_TIMEOUT;}
				else if(strncmp_P(rw_buf, PSTR("SD5F"), 4) == 0) {rc = PUBLISH_ERROR;}
				else if(strncmp_P(rw_buf, PSTR("SD6F"), 4) == 0) {rc = PUBLISH_TIMEOUT;}
				else if(strncmp_P(rw_buf, PSTR("SD7F"), 4) == 0) {rc = PUBLISH_QUEUE_FULL;}
				else if(strncmp_P(rw_buf, PSTR("SD8F"), 4) == 0) {rc = PUBLISH_QUEUE_DISABLED;}
				else if(strncmp_P(rw_buf, PSTR("SDFF"), 4) == 0) {rc = SHADOW_DELETE_GENERIC_ERROR;}
				else rc = GENERIC_ERROR;
			}
		}
		else {rc = OUT_OF_SKETCH_SUBSCRIBE_MEMORY;}
	}
	return rc;
}

// Send a request for a certain value according to key names and JSON identifier
// Value coming back will be stored in an external buffer as a string provided by the user
// desired
IoT_Error_t aws_iot_mqtt_client::getDesiredValueByKey(const char* JSONIdentifier, const char* key, char* externalJSONBuf, unsigned int bufSize) {
	return getJSONValueLoop(JSONIdentifier, key, externalJSONBuf, bufSize, DESIRED_SECTION);
}

// reported
IoT_Error_t aws_iot_mqtt_client::getReportedValueByKey(const char* JSONIdentifier, const char* key, char* externalJSONBuf, unsigned int bufSize) {
	return getJSONValueLoop(JSONIdentifier, key, externalJSONBuf, bufSize, REPORTED_SECTION);
}

// delta
IoT_Error_t aws_iot_mqtt_client::getDeltaValueByKey(const char* JSONIdentifier, const char* key, char* externalJSONBuf, unsigned int bufSize) {
	return getJSONValueLoop(JSONIdentifier, key, externalJSONBuf, bufSize, DELTA_SECTION);
}

// general
IoT_Error_t aws_iot_mqtt_client::getValueByKey(const char* JSONIdentifier, const char* key, char* externalJSONBuf, unsigned int bufSize) {
	return getJSONValueLoop(JSONIdentifier, key, externalJSONBuf, bufSize, GENERAL_SECTION);
}

IoT_Error_t aws_iot_mqtt_client::getJSONValueLoop(const char* JSONIdentifier, const char* key, char* externalJSONBuf, unsigned int bufSize, KV_access_t accessType) {
	IoT_Error_t rc = NONE_ERROR;
	int chunk_cnt = 0;
	while(true) {
		exec_cmd("4\n", false, false);

		exec_cmd("j\n", false, false);
		
		snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%s\n"), JSONIdentifier);
		exec_cmd(rw_buf, false, false);

		switch(accessType) {
			case DESIRED_SECTION:
				snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("state\"desired\"%s\n"), key);
				break;
			case REPORTED_SECTION:
				snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("state\"reported\"%s\n"), key);
				break;
			case DELTA_SECTION:
				snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("state\"%s\n"), key);
				break;
			case GENERAL_SECTION:
				snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%s\n"), key);
				break;
			default:
				snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%s\n"), key);
		}
		exec_cmd(rw_buf, false, false);

		int isFirst = chunk_cnt == 0 ? 1 : 0;
		chunk_cnt++;
		if(isFirst == 1) {snprintf_P(externalJSONBuf, bufSize, PSTR("%s"), "");} // Clear the external buffer
		snprintf_P(rw_buf, MAX_BUF_SIZE, PSTR("%d\n"), isFirst);
		exec_cmd(rw_buf, true, false);

		if(strncmp_P(rw_buf, PSTR("J0F"), 3) == 0) {break;} // End of JSON value string transmission
		else if(strncmp_P(rw_buf, PSTR("J1F"), 3) == 0) {
			rc = NO_SET_UP_ERROR;
			break;
		}
		else if(strncmp_P(rw_buf, PSTR("J2F"), 3) == 0) {
			rc = JSON_FILE_NOT_FOUND;
			break;
		}
		else if(strncmp_P(rw_buf, PSTR("J3F"), 3) == 0) {
			rc = JSON_KEY_NOT_FOUND;
			break;
		}
		else if(strncmp_P(rw_buf, PSTR("JFF"), 3) == 0) {
			rc = JSON_GENERIC_ERROR;
			break;
		}
		else if(rw_buf[0] != 'J') {
			rc = GENERIC_ERROR;
			break;
		}
		// Accumulate the incoming JSON chunks below this line
		char* saveptr, *p;
		p = strtok_r(rw_buf, " ", &saveptr); // J
		p += (strlen(p) + 1); // Get the rest of the JSON chunk
		if(p != NULL) {
			if(strlen(p) + strlen(externalJSONBuf) > bufSize) {
				rc = OVERFLOW_ERROR;
				break;
			}
			else {strcat(externalJSONBuf, p);} // Concatinate the JSON value string
		}
		else {
			rc = JSON_GENERIC_ERROR;
			break;
		}
	}
	return rc;
}

// Exec command and get feedback into rw_buf
void aws_iot_mqtt_client::exec_cmd(const char* cmd, bool wait, bool single_line) {
	// Write cmd
	int cnt = Serial1.write(cmd) + 1;
	timeout_flag = false;
	int timeout_sec = 0;
	// step1: forget the echo
	while(timeout_sec < CMD_TIME_OUT && cnt != 0) {
		if(Serial1.read() != -1) {cnt--;}
		else { // only start counting the timer when the serial1 is keeping us waiting...
			delay(5); // echo comes faster than python runtime client. Decreasing delay to avoid latency issue
			timeout_sec++;
		}
	}
	timeout_flag = timeout_sec == CMD_TIME_OUT; // Update timeout flag
	int ptr = 0;
	if(!timeout_flag) { // step 1 clear
		timeout_flag = false;
		timeout_sec = 0;
		// step2: waiting
		delay(6);
		if(wait) {
			while(timeout_sec < CMD_TIME_OUT && !Serial1.available()) {
				delay(50); // 50 ms
				timeout_sec++;
			}
		}
		timeout_flag = timeout_sec == CMD_TIME_OUT; // Update timeout flag
		if(!timeout_flag) { // step 2 clear
			// read feedback
		    // will read all the available data in Serial1 but only store the message with the limit of MAX_BUF_SIZE
			bool stop_sign = false;
			while(Serial1.available()) {
				int cc = Serial1.read();
				if(cc != -1) {
					if(cc == NEXTLINE_KEY || ptr == MAX_BUF_SIZE - 1) {
						stop_sign = true;
						if(single_line) {break;}
					} // end of feedback
					if(!stop_sign && cc != RETURN_KEY) {
						rw_buf[ptr++] = (char)cc;
					}
				}
			}
		}
	}
	timeout_flag = false; // Clear timeout flag
	rw_buf[ptr] = '\0'; // add terminator in case of garbage data in rw_buf
}

int aws_iot_mqtt_client::find_unused_subgroup() {
	int i = 0;
	for(i = 0; i < MAX_SUB; i++) {
		if(!sub_group[i].is_used) {break;}
	}
	return i; // could be MAX_SUB (Not found)
}

void aws_iot_mqtt_client::clearProtocolOnSerialBegin(long baudrate) {
	Serial1.begin(baudrate);
	while(!Serial1);
	exec_cmd("\n", true, false); // jump over the welcoming prompt for Open WRT
	delay(1000); // in case this is the first boot-up
	int i;
	for(i = 0; i < NUM_ATTEMPT_BEFORE_EXIT; i++) {
		// exit the previous python process and jump over the half-baked protocol communication
		exec_cmd("1\n", false, false);
		exec_cmd("~\n", true, false);
	}
	delay(1500); // delay 1500 ms for all related python script to exit
}

bool aws_iot_mqtt_client::is_num(char* src) {
	bool rc = true;
	if(src == NULL) {rc = false;}
	else {
		char* p = src;
		while(*p != '\0') {
			int currentCheck = (int)(*p);
			if(!isdigit(currentCheck)) {
				rc = false;
				break;
			}
			p++;
		}
	}
	return rc;
}
