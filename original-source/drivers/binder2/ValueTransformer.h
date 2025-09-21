/******************************************************************************
/
/	File:			ValueMapTransformer.h
/
/	Description:	Helper class for performing binder transformation of
/					a flattened BValueMap.
/
/	Copyright 2001, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _SUPPORT2_VALUETRANSFORMER_H
#define _SUPPORT2_VALUETRANSFORMER_H

#include <support2/TypeConstants.h>
#include <support2_p/ValueMapFormat.h>

enum {
	B_BINDER_NODE_TYPE		= '_bnd'
};

namespace B {
namespace Support2 {

typedef status_t (*transform_binder_value)(void *userData, type_code* type, uint32* value);
typedef ssize_t (*write_transformed_data)(const void* data, size_t length);

struct value_transformer
{
	value_transformer(
		void *userData,
		transform_binder_value transform,
		write_transformed_data write);
	~value_transformer();
	
	ssize_t transform(void* data, size_t length);
	
private:
	status_t transform_map(void* map, size_t length);
	
	void *						m_userData;
	transform_binder_value		m_transform;
	write_transformed_data		m_write;
};

} }	// namespace B::Support2

#endif /* _SUPPORT2_VALUEMAPTRANSFORMER_H */
