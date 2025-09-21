
#include "binder_defs.h"
#include "ValueTransformer.h"

namespace B {
namespace Support2 {

value_transformer::value_transformer(
	void *userData,
	transform_binder_value transform,
	write_transformed_data write)
	:	m_userData(userData), m_transform(transform), m_write(write)
{
}

value_transformer::~value_transformer()
{
}

status_t value_transformer::transform_map(void* map, size_t length)
{
	value_map_header* head = reinterpret_cast<value_map_header*>(map);
	if (length < sizeof(value_map_header)) return B_NOT_A_MESSAGE;
	if (head->signature != VALUE_MAP_SIGNATURE) return B_NOT_A_MESSAGE;

	length -= sizeof(value_map_header);
	
	// Iterate through all chunks in the message.
	chunk_header* chunk = reinterpret_cast<chunk_header*>(head+1);
	while (length >= sizeof(chunk_header) && chunk->code != END_CODE) {
		if (chunk->code == OFFSETS_CODE) {
			// Offsets chunk -- if it says there are no binder nodes, then we
			// can just keep the entire message as-is.
			offsets_chunk* off = reinterpret_cast<offsets_chunk*>(chunk);
			if (off->binder_count == 0)
				return B_OK;
		} else if (chunk->code == DATA_CODE) {
			// Data chunk -- if this is a binder node, perform transformation
			// on it.
			data_chunk* data = reinterpret_cast<data_chunk*>(chunk);
			if (data->value.type == B_VALUE_MAP_TYPE) {
				const status_t s = transform_map(data->value_data(), data->value.length);
				if (s < B_OK) return s;
			} else  if (data->value.length == sizeof(int32) &&
						(	data->value.type == B_BINDER_TYPE
							|| data->value.type == B_BINDER_HANDLE_TYPE
							|| data->value.type == B_BINDER_NODE_TYPE ) ) {
				const status_t s = m_transform(
					m_userData,&data->value.type, reinterpret_cast<uint32*>(data->value_data()));
				if (s < B_OK) return s;
			}
		}
		if (length >= chunk->size) {
			length -= chunk->size;
			chunk = reinterpret_cast<chunk_header*>(
				reinterpret_cast<uint8*>(chunk) + chunk_align(chunk->size));
		} else return B_NOT_A_MESSAGE;
	}
	
	return B_OK;
}

ssize_t value_transformer::transform(void* data, size_t length)
{
	// A value starts with two integers, its type and length.
	if (length >= (sizeof(int32)*2)) {
		uint32* head = reinterpret_cast<uint32*>(data);
		if (length >= (head[1]+sizeof(int32)*2)) {
			if (head[0] == B_VALUE_MAP_TYPE) {
				const status_t s = transform_map(head+2, length-(sizeof(int32)*2));
				if (s < B_OK) return s;
			} else if (head[1] == sizeof(int32) &&
						(head[0] == B_BINDER_TYPE ||
						 head[0] == B_BINDER_HANDLE_TYPE ||
						 head[0] == B_BINDER_NODE_TYPE)) {
				const status_t s = m_transform(m_userData,head, head+2);
				if (s < B_OK) return s;
			}
			return m_write(data, head[1]+sizeof(int32)*2);
		}
		return m_write(data, length);
	}
	return B_BAD_VALUE;
}

} }	// namespace B::Support2

