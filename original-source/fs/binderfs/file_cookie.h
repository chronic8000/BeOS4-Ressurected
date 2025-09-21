#ifndef FILE_COOKIE_H
#define FILE_COOKIE_H

struct buffer_t {
	void	*data;
	size_t	size;
};

enum file_cookie_type {
	FILE_COOKIE_UNUSED = 0,
	FILE_COOKIE_HOST_CONNECTION,
	FILE_COOKIE_CLIENT_CONNECTION,
	FILE_COOKIE_TRANSACTION_STATE,
	FILE_COOKIE_PROPERTY_ITERATOR,
	FILE_COOKIE_BUFFER
};

struct multi_cookie;
struct nspace;
struct transaction_state;

struct file_cookie {
	file_cookie_type               type;
	union	{
		struct {
			transaction_state     *trans;
		} transaction_state;
		struct {
			multi_cookie          *cookie;
		} property_iterator;
		struct {
			uint8                 *buffer;
			int32                  bufferSize;
		} buffer;
	} data;
	
	void clear(nspace *ns);
	
	transaction_state *get_transaction_state()
	{
		return type == FILE_COOKIE_TRANSACTION_STATE ?
			data.transaction_state.trans : NULL;
	}
	
	multi_cookie *get_property_iterator()
	{
		return type == FILE_COOKIE_PROPERTY_ITERATOR ?
			data.property_iterator.cookie : NULL;
	}

	void associate_transaction_state(nspace *ns, transaction_state *pt)
	{
		clear(ns);
		data.transaction_state.trans = pt;
		type = FILE_COOKIE_TRANSACTION_STATE;
	}

	status_t associate_buffer(nspace *ns, buffer_t *buf)
	{
		clear(ns);
		data.buffer.buffer = (uint8*)malloc(buf->size);
		if(data.buffer.buffer == NULL)
			return B_NO_MEMORY;
		memcpy(data.buffer.buffer, buf->data, buf->size);
		data.buffer.bufferSize = buf->size;
		type = FILE_COOKIE_BUFFER;
		return B_OK;
	}

	void associate_property_iterator_state(nspace *ns, multi_cookie *cookie)
	{
		clear(ns);
		data.property_iterator.cookie = cookie;
		type = FILE_COOKIE_PROPERTY_ITERATOR;
	}
};

#endif

