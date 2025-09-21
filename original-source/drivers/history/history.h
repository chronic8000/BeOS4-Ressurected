#ifndef _HISTORY_DRIVER_H
#define _HISTORY_DRIVER_H

#include <OS.h>
#include <unistd.h>

#define HISTORY_PUSH     		0x7000
#define HISTORY_PEEK     		0x7001
#define HISTORY_READ     		0x7002
#define HISTORY_FORWARD  		0x7003
#define HISTORY_BACK     		0x7004
#define HISTORY_GLOBAL_PEEK		0x7005
#define HISTORY_GLOBAL_READ		0x7006
#define HISTORY_GLOBAL_WRITE	0x7007
#define HISTORY_CLEAR           0x7008

typedef struct hioctl {
	void *data;
	size_t size;
} hioctl;

#ifdef __cplusplus

inline status_t history_push(int fd, void *data, size_t size) 
{
	hioctl request;
	request.data = data;
	request.size = size;
	return ioctl(fd, HISTORY_PUSH, &request, sizeof(request));
}

inline status_t history_peek(int fd) 
{
	hioctl request;
	status_t res;
	res = ioctl(fd, HISTORY_PEEK, &request, sizeof(request));
	if (res == B_OK)
		return request.size;

	return res;
}

inline status_t history_read(int fd, void *data, size_t size) 
{
	hioctl request;
	status_t res;
	request.data = data;
	request.size = size;
	res = ioctl(fd, HISTORY_READ, &request, sizeof(request));
	if (res == B_OK)
		return request.size;

	return res;
}

inline status_t history_back(int fd) 
{
	hioctl request;
	return ioctl(fd, HISTORY_BACK, &request, sizeof(request));
}

inline status_t history_clear(int fd) 
{
	hioctl request;
	return ioctl(fd, HISTORY_CLEAR, &request, sizeof(request));
}

inline status_t history_forward(int fd) 
{
	hioctl request;
	return ioctl(fd, HISTORY_FORWARD, &request, sizeof(request));
}

inline status_t history_global_peek(int fd) 
{
	hioctl request;
	status_t res;

	res = ioctl(fd, HISTORY_GLOBAL_PEEK, &request, sizeof(request));
	if (res == B_OK)
		return request.size;

	return res;
}

inline status_t history_global_read(int fd, void *data, size_t size) 
{
	hioctl request;
	status_t res;
	request.data = data;
	request.size = size;
	res = ioctl(fd, HISTORY_GLOBAL_READ, &request, sizeof(request));
	if (res == B_OK)
		return request.size;

	return res;
}

inline status_t history_global_write(int fd, void *data, size_t size) 
{
	hioctl request;
	request.data = data;
	request.size = size;
	return ioctl(fd, HISTORY_GLOBAL_WRITE, &request, sizeof(request));
}

#endif

#endif
