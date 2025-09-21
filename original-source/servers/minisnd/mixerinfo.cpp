
#include "mixerinfo.h"
#include "snddev.h"	//	for g_debug

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#define MIXERINFO 1

// ahh fsk it!
using namespace std;

template<class C> class G : public C {
public:
	G() { memset(this, 0, sizeof(*this)); info_size = sizeof(*this); }
};

template<class C> class H : public C {
public:
	H() { memset(this, 0, sizeof(*this)); }
};



//#pragma mark --- mixerinfo ---

mixerinfo::mixerinfo() : m_mixer(0)
{
//puts(__PRETTY_FUNCTION__);
}



mixerinfo::mixerinfo(const mixerinfo &other) : m_mixer(0), m_values(other.m_values)
{
}

mixerinfo &
mixerinfo::operator=(const mixerinfo &other)
{
#if MIXERINFO
	m_mixer = other.m_mixer;
	m_values = other.m_values;
#endif
	return *this;
}

mixerinfo::~mixerinfo()
{
//puts(__PRETTY_FUNCTION__);
}

status_t 
mixerinfo::get(int fd, int32 mixer)
{
//puts(__PRETTY_FUNCTION__);

#if MIXERINFO
	status_t err = 0;
	m_mixer = 0;
	m_values.clear();
	G<game_get_mixer_infos> ggmis;
	H<game_mixer_info> gmi;
	ggmis.info = &gmi;
	ggmis.in_request_count = 1;
	gmi.mixer_id = mixer;
	if (ioctl(fd, GAME_GET_MIXER_INFOS, &ggmis) < 0) {
		err = errno;
		fprintf(stderr, "error: ioctl(GAME_GET_MIXER_INFOS): %s\n", strerror(err));
		return err;
	}
	vector<game_mixer_control> controls;
	controls.resize(gmi.control_count);
	G<game_get_mixer_controls> ggmcs;
	ggmcs.control = &controls[0];
	ggmcs.in_request_count = gmi.control_count;
	ggmcs.mixer_id = mixer;
	if (ioctl(fd, GAME_GET_MIXER_CONTROLS, &ggmcs) < 0) {
		err = errno;
		fprintf(stderr, "error: ioctl(GAME_GET_MIXER_CONTROLS): %s\n", strerror(err));
		return err;
	}
	G<game_get_mixer_control_values> ggmcvs;
	ggmcvs.in_request_count = gmi.control_count;
	if (g_debug & 8) fprintf(stderr, "mixerinfo::get(): %ld controls\n", gmi.control_count);
	m_values.resize(ggmcvs.in_request_count);
	ggmcvs.values = &m_values[0];
	ggmcvs.mixer_id = mixer;
	for (int ix=0; ix<gmi.control_count; ix++) {
		ggmcvs.values[ix].control_id = ggmcs.control[ix].control_id;
		ggmcvs.values[ix].mixer_id = mixer;	//	should be unnecessary
	}
	if (ioctl(fd, GAME_GET_MIXER_CONTROL_VALUES, &ggmcvs) < 0) {
		err = errno;
		fprintf(stderr, "error: ioctl(GAME_GET_MIXER_CONTROL_VALUES)\n");
		return err;
	}
	m_mixer = mixer;
#endif
	return B_OK;
}

status_t 
mixerinfo::set(int fd)
{
//puts(__PRETTY_FUNCTION__);

#if MIXERINFO
	if (m_mixer <= 0) {
		return B_BAD_VALUE;
	}
	G<game_set_mixer_control_values> gsmcvs;
	gsmcvs.mixer_id = m_mixer;
	gsmcvs.in_request_count = m_values.size();
	gsmcvs.values = &m_values[0];
	status_t err = B_OK;
	if (ioctl(fd, GAME_SET_MIXER_CONTROL_VALUES, &gsmcvs) < 0) {
		err = errno;
		fprintf(stderr, "error: ioctl(GAME_SET_MIXER_CONTROL_VALUES)\n");
		return err;
	}
#endif
	return B_OK;
}

ssize_t 
mixerinfo::save_size()
{
//puts(__PRETTY_FUNCTION__);

#if MIXERINFO
	return sizeof(m_mixer)+sizeof(int)+sizeof(game_mixer_control_value)*m_values.size();
#else
	return 0;
#endif
}

ssize_t 
mixerinfo::save(void *data, size_t max_size)
{
//puts(__PRETTY_FUNCTION__);

#if MIXERINFO
	ssize_t t = save_size();
	if (g_debug & 8) fprintf(stderr, "mixerinfo save_size is %ld\n", t);
	if (t > max_size) return B_NO_MEMORY;
	char * s = (char *)data;
	memcpy(s, &m_mixer, sizeof(m_mixer));
	s += sizeof(m_mixer);
	int c = m_values.size();
	memcpy(s, &c, sizeof(c));
	s += sizeof(c);
	memcpy(s, &m_values[0], sizeof(game_mixer_control_value)*c);
	ASSERT(s <= (((char *)data)+max_size));
	if (g_debug & 8) fprintf(stderr, "info: mixerinfo::save() returns %ld bytes (%ld)\n", t, s-(char*)data);
	return t;
#else
	return 0;
#endif
}

ssize_t 
mixerinfo::load(const void *data, size_t size)
{
//puts(__PRETTY_FUNCTION__);

#if MIXERINFO
	if (size < sizeof(m_mixer)+sizeof(int)) {
		if (g_debug & 8) fprintf(stderr, "warning: mixerinfo::load() gets bad data\n");
		return B_BAD_VALUE;
	}
	int32 mix;
	int c;
	const char * l = (const char *)data;
	memcpy(&mix, l, sizeof(mix));
	l += sizeof(mix);
	memcpy(&c, l, sizeof(c));
	l += sizeof(c);
	if (c > 200 || c < 0) {
		if (g_debug & 8) fprintf(stderr, "error: mixerinfo::load() gets %d controls?\n", c);
		return B_BAD_VALUE;
	}
	if (size < sizeof(m_mixer)+sizeof(int)+sizeof(game_mixer_control_value)*c) {
		if (g_debug & 8) fprintf(stderr, "warning: mixerinfo::load() gets truncated data\n");
		return B_BAD_VALUE;
	}
	if (g_debug & 8) fprintf(stderr, "mixerinfo::load() %d controls\n", c);
	m_values.resize(c);
	memcpy(&m_values[0], l, sizeof(game_mixer_control_value)*c);
	m_mixer = mix;
	return sizeof(m_mixer)+sizeof(int)+sizeof(game_mixer_control_value)*c;
#else
	return 0;
#endif
}


//#pragma mark --- mixerstate ---


mixerstate::mixerstate() : ok(false)
{
//puts(__PRETTY_FUNCTION__);
//#if MIXERINFO
//	m_mixers.resize(2);
//#endif
}


mixerstate::~mixerstate()
{
//puts(__PRETTY_FUNCTION__);
}

status_t 
mixerstate::get(int fd)
{
//puts(__PRETTY_FUNCTION__);

#if MIXERINFO
	G<game_get_info> ggi;
	status_t err;
	if (ioctl(fd, GAME_GET_INFO, &ggi) < 0) {
		err = errno;
		fprintf(stderr, "error: ioctl(GAME_GET_INFO)\n");
		return err;
	}
	if (g_debug & 8) fprintf(stderr, "mixerstate::get(): mixer count: %d\n", ggi.mixer_count);
	m_mixers.resize(ggi.mixer_count);
	for (int ix=0; ix<ggi.mixer_count; ix++) {
		err = m_mixers[ix].get(fd, GAME_MAKE_MIXER_ID(ix));
		if (err < 0) {
			m_mixers.resize(0);
			fprintf(stderr, "mixers(%d)::get(): %s\n", ix, strerror(err));
			return err;
		}
	}
	ok = true;
#endif
	return B_OK;
}

status_t 
mixerstate::set(int fd)
{
//puts(__PRETTY_FUNCTION__);

#if MIXERINFO
	if (!ok) return B_BAD_VALUE;
	status_t err;
	for (int ix=0; ix<m_mixers.size(); ix++) {
		err = m_mixers[ix].set(fd);
		if (err < 0) return err;
	}
#endif
	return B_OK;
}

ssize_t 
mixerstate::save_size()
{
//puts(__PRETTY_FUNCTION__);

#if MIXERINFO
	ssize_t t = sizeof(int);
	if (!ok) return B_BAD_VALUE;
	for (int ix=0; ix<m_mixers.size(); ix++) {
		t += m_mixers[ix].save_size();
	}
	return t;
#else
	return 0;
#endif
}

ssize_t 
mixerstate::save(void *data, size_t max_size)
{
//puts(__PRETTY_FUNCTION__);

#if MIXERINFO
	ssize_t ss = save_size();
	if (ss > max_size) {
		fprintf(stderr, "mixerstate::save(): no memory 1\n");
		return B_NO_MEMORY;
	}
	char * s = (char *)data;
	int c = m_mixers.size();
	memcpy(s, &c, sizeof(c));
	s += sizeof(c);
	status_t err;
	for (int ix=0; ix<m_mixers.size(); ix++) {
		ss = m_mixers[ix].save_size();
		err = m_mixers[ix].save(s, ss);
		if (err < 0) {
			fprintf(stderr, "mixerstate::save(%d): %s\n", ix, strerror(err));
			return err;
		}
		s += err;
	}
	ASSERT(s <= (((char *)data)+max_size));
	if (g_debug & 8) fprintf(stderr, "info: mixerstate::save() returns %ld bytes written\n", s-(char *)data);
	return s-(char *)data;
#else
	return 0;
#endif
}

ssize_t 
mixerstate::load(const void *data, size_t size)
{
//puts(__PRETTY_FUNCTION__);

#if MIXERINFO
	ok = false;
	if (size < sizeof(int)) {
		fprintf(stderr, "error: mixerstate::load(): bad data\n");
		return B_BAD_VALUE;
	}
	int c;
	const char * l = (const char *)data;
	const char * e = l+size;
	memcpy(&c, l, sizeof(int));
	l += sizeof(int);
	if ((c < 0) || (c > 16)) {
		fprintf(stderr, "error: mixerstate::load() sees %d mixers???\n", c);
		return B_BAD_VALUE;
	}
	if (g_debug & 8) fprintf(stderr, "mixerstate::load() sees %d mixers.\n", c);
	m_mixers.resize(c);
	status_t err;
	for (int ix=0; ix<c; ix++) {
		err = m_mixers[ix].load(l, e-l);
		if (err < 0) {
			fprintf(stderr, "mixerstate::load(%d): %s\n", ix, strerror(err));
			return err;
		}
		l += err;
	}
	ok = true;
	if (g_debug & 8) fprintf(stderr, "mixerstate::load(): completed %ld bytes\n", l-(const char *)data);
	return l - (const char *)data;
#else
	return 0;
#endif
}

