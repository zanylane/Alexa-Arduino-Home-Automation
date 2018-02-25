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
 
#ifndef aws_iot_error_h
#define aws_iot_error_h

typedef enum {
	NONE_ERROR = 0,
	GENERIC_ERROR = -1,
	NULL_VALUE_ERROR = -2,
	OVERFLOW_ERROR = -3,
	OUT_OF_SKETCH_SUBSCRIBE_MEMORY = -4,
	SERIAL1_COMMUNICATION_ERROR = -5,
	SET_UP_ERROR = -6,
	NO_SET_UP_ERROR = -7,
	WRONG_PARAMETER_ERROR = -8,
	CONFIG_GENERIC_ERROR = -9,
	CONNECT_SSL_ERROR = -10,
	CONNECT_ERROR = -11,
	CONNECT_TIMEOUT = -12,
	CONNECT_CREDENTIAL_NOT_FOUND = -13,
	CONNECT_GENERIC_ERROR = -14,
	PUBLISH_ERROR = -15,
	PUBLISH_TIMEOUT = -16,
	PUBLISH_GENERIC_ERROR = -17,
	SUBSCRIBE_ERROR = -18,
	SUBSCRIBE_TIMEOUT = -19,
	SUBSCRIBE_GENERIC_ERROR = -20,
	UNSUBSCRIBE_ERROR = -21,
	UNSUBSCRIBE_TIMEOUT = -22,
	UNSUBSCRIBE_GENERIC_ERROR = -23,
	DISCONNECT_ERROR = -24,
	DISCONNECT_TIMEOUT = -25,
	DISCONNECT_GENERIC_ERROR = -26,
	SHADOW_INIT_ERROR = -27,
	NO_SHADOW_INIT_ERROR = -28,
	SHADOW_GET_GENERIC_ERROR = -29,
	SHADOW_UPDATE_GENERIC_ERROR = -30,
	SHADOW_UPDATE_INVALID_JSON_ERROR = -31,
	SHADOW_DELETE_GENERIC_ERROR = -32,
	SHADOW_REGISTER_DELTA_CALLBACK_GENERIC_ERROR = -33,
	SHADOW_UNREGISTER_DELTA_CALLBACK_GENERIC_ERROR = -34,
	YIELD_ERROR = -35,
	WEBSOCKET_CREDENTIAL_NOT_FOUND = -36,
	JSON_FILE_NOT_FOUND = -37,
	JSON_KEY_NOT_FOUND = -38,
	JSON_GENERIC_ERROR = -39,
	PUBLISH_QUEUE_FULL = -40,
	PUBLISH_QUEUE_DISABLED = -41
} IoT_Error_t;

#endif

