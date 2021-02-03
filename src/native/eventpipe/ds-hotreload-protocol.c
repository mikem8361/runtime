#include "ds-rt-config.h"

#ifdef ENABLE_PERFTRACING
#if !defined(DS_INCLUDE_SOURCE_FILES) || defined(DS_FORCE_INCLUDE_SOURCE_FILES)

#define DS_IMPL_HOTRELOAD_PROTOCOL_GETTER_SETTER
#include "ds-protocol.h"
#include "ds-hotreload-protocol.h"
#include "ds-rt.h"

/*
 * Forward declares of all static functions.
 */
static
uint8_t *
hotreload_apply_update_command_try_parse_payload (
	uint8_t *buffer,
	uint16_t buffer_len);

static
bool
hotreload_protocol_helper_hotreload_apply_update(
	DiagnosticsIpcMessage *message,
	DiagnosticsIpcStream *stream);

static
bool
hotreload_protocol_helper_unknown_command (
	DiagnosticsIpcMessage *message,
	DiagnosticsIpcStream *stream);

/*
* DiagnosticsAppyUpdateCommandPayload
*/

static
uint8_t *
hotreload_apply_update_command_try_parse_payload (
	uint8_t *buffer,
	uint16_t buffer_len)
{
	EP_ASSERT (buffer != NULL);

	uint8_t * buffer_cursor = buffer;
	uint32_t buffer_cursor_len = buffer_len;

	DiagnosticsApplyUpdateCommandPayload *instance = ds_apply_update_command_payload_alloc ();
	ep_raise_error_if_nok (instance != NULL);

	instance->incoming_buffer = buffer;

	if (!ds_ipc_message_try_parse_string_utf16_t (&buffer_cursor, &buffer_cursor_len, &instance->module_path) ||
		!ds_ipc_message_try_parse_int32_t (&buffer_cursor, &buffer_cursor_len, &instance->metadata_delta_length) ||
		!ds_ipc_message_try_parse_int32_t (&buffer_cursor, &buffer_cursor_len, &instance->il_delta_length) ||
		!(buffer_cursor_len <= (uint32_t)(instance->metadata_delta_length + instance->il_delta_length)))
		ep_raise_error ();

    instance->metadata_delta = buffer_cursor;
    instance->il_delta = buffer_cursor + instance->metadata_delta_length;

ep_on_exit:
	return (uint8_t *)instance;

ep_on_error:
	ds_apply_update_command_payload_free (instance);
	instance = NULL;
	ep_exit_error_handler ();
}

DiagnosticsApplyUpdateCommandPayload *
ds_apply_update_command_payload_alloc (void)
{
	return ep_rt_object_alloc (DiagnosticsApplyUpdateCommandPayload);
}

void
ds_apply_update_command_payload_free (DiagnosticsApplyUpdateCommandPayload *payload)
{
	ep_return_void_if_nok (payload != NULL);
	ep_rt_byte_array_free (payload->incoming_buffer);
	ep_rt_object_free (payload);
}

/*
 * DiagnosticsHotReloadProtocolHelper.
 */

static
bool
hotreload_protocol_helper_unknown_command (
	DiagnosticsIpcMessage *message,
	DiagnosticsIpcStream *stream)
{
	DS_LOG_WARNING_1 ("Received unknown request type (%d)\n", ds_ipc_header_get_commandset (ds_ipc_message_get_header_ref (message)));
	ds_ipc_message_send_error (stream, DS_IPC_E_UNKNOWN_COMMAND);
	ds_ipc_stream_free (stream);
	return true;
}

static
bool
hotreload_protocol_helper_apply_update(
	DiagnosticsIpcMessage *message,
	DiagnosticsIpcStream *stream)
{
	EP_ASSERT (message != NULL);
	EP_ASSERT (stream != NULL);

	if (!stream)
		return false;

	bool result = false;
	DiagnosticsApplyUpdateCommandPayload *payload;
	payload = (DiagnosticsApplyUpdateCommandPayload *)ds_ipc_message_try_parse_payload (message, hotreload_apply_update_command_try_parse_payload);

	if (!payload) {
		ds_ipc_message_send_error (stream, DS_IPC_E_BAD_ENCODING);
		ep_raise_error ();
	}

	ds_ipc_result_t ipc_result;
	ipc_result = ds_rt_apply_update(payload);
	if (result != DS_IPC_S_OK) {
		ds_ipc_message_send_error (stream, result);
		ep_raise_error ();
	} else {
		ds_ipc_message_send_success (stream, result);
	}

	result = true;

ep_on_exit:
	ds_apply_update_command_payload_free (payload);
	ds_ipc_stream_free (stream);
	return result;

ep_on_error:
	EP_ASSERT (!result);
	ep_exit_error_handler ();
}

bool
ds_hotreload_protocol_helper_handle_ipc_message (
	DiagnosticsIpcMessage *message,
	DiagnosticsIpcStream *stream)
{
	EP_ASSERT (message != NULL);
	EP_ASSERT (stream != NULL);

	bool result = false;

	switch ((DiagnosticsHotReloadCommandId)ds_ipc_header_get_commandid (ds_ipc_message_get_header_ref (message))) {
    case DS_HOTRELOAD_COMMANDID_APPLY_UPDATE:
		result = hotreload_protocol_helper_apply_update(message, stream);
		break;
	default:
		result = hotreload_protocol_helper_unknown_command (message, stream);
		break;
	}
	return result;
}

#endif /* !defined(DS_INCLUDE_SOURCE_FILES) || defined(DS_FORCE_INCLUDE_SOURCE_FILES) */
#endif /* ENABLE_PERFTRACING */

#ifndef DS_INCLUDE_SOURCE_FILES
extern const char quiet_linker_empty_file_warning_diagnostics_hotreload_protocol;
const char quiet_linker_empty_file_warning_diagnostics_hotreload_protocol = 0;
#endif
