#ifndef __DIAGNOSTICS_HOTRELOAD_PROTOCOL_H__
#define __DIAGNOSTICS_HOTRELOAD_PROTOCOL_H__

#include "ds-rt-config.h"

#ifdef ENABLE_PERFTRACING
#include "ds-types.h"
#include "ds-ipc.h"

#undef DS_IMPL_GETTER_SETTER
#ifdef DS_IMPL_HOTRELOAD_PROTOCOL_GETTER_SETTER
#define DS_IMPL_GETTER_SETTER
#endif
#include "ds-getter-setter.h"

/*
* DiagnosticsApplyUpdateCommandPayload
*/

#if defined(DS_INLINE_GETTER_SETTER) || defined(DS_IMPL_HOTRELOAD_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsApplyUpdateCommandPayload {
#else
struct _DiagnosticsApplyUpdateCommandPayload_Internal {
#endif
	uint8_t * incoming_buffer;

	// The protocol buffer is defined as:
	//   string - modulePath (UTF16)
	//   int - metadata delta length
	//   int - il delta length
	//   byte* - metadata delta bytes
	//   byte* - il delta bytes
	// returns
	//   ulong - status

	const ep_char16_t* module_path;
    int32_t metadata_delta_length;
    uint8_t* metadata_delta;
    int32_t il_delta_length;
    uint8_t* il_delta;
};

#if !defined(DS_INLINE_GETTER_SETTER) && !defined(DS_IMPL_HOTRELOAD_PROTOCOL_GETTER_SETTER)
struct _DiagnosticsApplyUpdateCommandPayload {
	uint8_t _internal [sizeof (struct _DiagnosticsApplyUpdateCommandPayload_Internal)];
};
#endif

DS_DEFINE_GETTER(DiagnosticsApplyUpdateCommandPayload *, apply_update_command_payload, const ep_char16_t *, module_path)
DS_DEFINE_GETTER(DiagnosticsApplyUpdateCommandPayload *, apply_update_command_payload, int32_t, metadata_delta_length)
DS_DEFINE_GETTER(DiagnosticsApplyUpdateCommandPayload *, apply_update_command_payload, uint8_t *, metadata_delta)
DS_DEFINE_GETTER(DiagnosticsApplyUpdateCommandPayload *, apply_update_command_payload, int32_t, il_delta_length)
DS_DEFINE_GETTER(DiagnosticsApplyUpdateCommandPayload *, apply_update_command_payload, uint8_t *, il_delta)

DiagnosticsApplyUpdateCommandPayload *
ds_apply_update_command_payload_alloc (void);

void
ds_apply_update_command_payload_free (DiagnosticsApplyUpdateCommandPayload *payload);

/*
 * DiagnosticsHotReloadProtocolHelper.
 */

bool
ds_hotreload_protocol_helper_handle_ipc_message (
	DiagnosticsIpcMessage *message,
	DiagnosticsIpcStream *stream);

#endif /* ENABLE_PERFTRACING */
#endif /* __DIAGNOSTICS_HOTRELOAD_PROTOCOL_H__ */
