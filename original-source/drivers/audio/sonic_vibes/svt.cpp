#include "sonic_vibes.h"
#include <stdio.h>
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <errno.h>


volatile int quitf = 0;


static void
quit(
	int sig)
{
	quitf = 1;
	if (be_app) {
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
	signal(SIGINT, quit);
}


#define NSAMP 100000


static status_t
pcm(
	void * arg)
{
	char path[200];
	sprintf(path, "/dev/audio/%s", arg);
	int fd = open((char *)path, O_RDWR);
	if (fd < 0) {
		perror("cannot open sonic vibes audio");
		return -1;
	}
	sonic_vibes_audio_format format;
	format.sample_rate = 44100.0;
	format.channels = 2;
	format.format = 0x2;
	format.endian_swap = 0;
	format.buf_header = 0;
	if (ioctl(fd, B_AUDIO_SET_AUDIO_FORMAT, &format, sizeof(format)) < 0) {
		close(fd);
		perror("cannot set audio format");
		return -1;
	}
	if (ioctl(fd, B_AUDIO_GET_AUDIO_FORMAT, &format, sizeof(format)) < 0) {
		close(fd);
		perror("cannot get audio format");
		return -1;
	}
	printf("format: sr %g  ch %d  fo %x  es %d  bh %d\n",
		format.sample_rate, format.channels, format.format, 
		format.endian_swap, format.buf_header);
	int m = format.channels*(format.format&0xf);
	void *buf = (void*)malloc((NSAMP+1)*m);
	if (!buf) {
		close(fd);
		perror("cannot malloc()");
		return -1;
	}
	int c = format.channels;
	/* snooze(1000000.0);	/* wait before we start */
	while (!quitf) {
		float rng, dlt;
		if (format.format == 2) {
			rng = 30000;
			dlt = 0;
		}
		else {
			rng = 110;
			dlt = 128;
		}
		float tone = rand()&0xf;
		tone = tone*tone*4000.0/format.sample_rate + 1.0;
		for (int ix=0; ix<NSAMP; ix++) {
			float w = sin((float)ix/(tone /* +ix*ix/5000000.0 */))*rng;
			if (ix < 1000) {
				w = w*ix/1000.0;
			}
			else if (ix > NSAMP-1000) {
				w = w*(NSAMP-ix)/1000.0;
			}
			w += dlt;
			if (format.format == 2) {
				((short*)buf)[ix*c] = ((short*)buf)[ix*c+1] = w;
			}
			else {
				((uchar*)buf)[ix*c] = ((uchar*)buf)[ix*c+1] = w;
			}
		}
		write(fd, buf, NSAMP*m);
		snooze(300000);
	}
	close(fd);
}


class MixView :
	public BView
{
		int mFd;
		struct mix {
			char name[B_OS_NAME_LENGTH];
			float min;
			float max;
			float cur;
			bool hasMute;
			bool muted;
			BRect area;
		} mSliders[100];
		int mSliderCnt;
public:
		MixView(
			const char * dev);
		~MixView();

		void MouseDown(
			BPoint where);
		void Draw(
			BRect area);
		status_t Error();
		void GetPreferredSize(
			float * width,
			float * height);
};

MixView::MixView(
	const char * name) :
	BView(BRect(0,0,0,0), name, B_FOLLOW_NONE, B_WILL_DRAW)
{
	mFd = open(name, O_RDWR);
	mSliderCnt = 0;
	if (mFd < 0) {
		return;
	}
	sonic_vibes_level_name mname;
	sonic_vibes_level mval;

	mname.selector = mSliderCnt;
	while (ioctl(mFd, B_MIXER_GET_NAME, &mname, sizeof(mname)) >= 0) {
		strcpy(mSliders[mSliderCnt].name, mname.name);
		mval.selector = mSliderCnt;
		ioctl(mFd, B_MIXER_GET_MIN_VALUE, &mval, sizeof(mval));
		mSliders[mSliderCnt].min = mval.value;
		ioctl(mFd, B_MIXER_GET_MAX_VALUE, &mval, sizeof(mval));
		mSliders[mSliderCnt].max = mval.value;
		if (mval.flags & SONIC_VIBES_LEVEL_MUTED) {
			mSliders[mSliderCnt].hasMute = true;
		}
		else {
			mSliders[mSliderCnt].hasMute = false;
		}
		mSliderCnt++;
		mname.selector = mSliderCnt;
	}
	sonic_vibes_level *ptr = (sonic_vibes_level *)calloc(
		sizeof(sonic_vibes_level), mSliderCnt);
	read(mFd, ptr, sizeof(sonic_vibes_level)*mSliderCnt);
#define WIDTH 32
	BRect r(0,0,WIDTH-1,200);
	for (int ix=0; ix<mSliderCnt; ix++) {
		mSliders[ix].area = r;
		r.OffsetBy(WIDTH, 0);
		mSliders[ix].cur = ptr[ix].value;
		mSliders[ix].muted = (ptr[ix].flags & SONIC_VIBES_LEVEL_MUTED);
	}
	free(ptr);
}


MixView::~MixView()
{
	if (mFd >= 0) {
		close(mFd);
	}
	quit(SIGINT);
}


void
MixView::MouseDown(
	BPoint where)
{
}


void
MixView::Draw(
	BRect area)
{
	for (int ix=0; ix<mSliderCnt; ix++) {
		SetHighColor(220,220,200);
		FillRect(mSliders[ix].area, B_SOLID_HIGH);
		SetHighColor(100,100,100);
		BRect m = mSliders[ix].area;
		m.top += 16;
		m.bottom -= 32;
		m.left += (m.IntegerWidth()/2-1);
		m.right = m.left+1;
		FillRect(m, B_SOLID_HIGH);
		SetHighColor(0,0,0);
	
		char str[40];
		sprintf(str, "%+4.1f", mSliders[ix].max);
		DrawString(str, BPoint(mSliders[ix].area.left+2, 
			mSliders[ix].area.top+12));
		sprintf(str, "%+4.1f", mSliders[ix].min);
		DrawString(str, BPoint(mSliders[ix].area.left+2,
			mSliders[ix].area.bottom-20));
		m = mSliders[ix].area;
		m.bottom -= 2;
		m.top = m.bottom - 12;
		m.left += 2;
		m.right -= 2;
		if (mSliders[ix].hasMute) {
			StrokeRect(m);
			m.InsetBy(1,1);
			if (mSliders[ix].muted) {
				SetLowColor(255,128,0);
				SetHighColor(0,0,0);
			}
			else {
				SetLowColor(200,200,200);
				SetHighColor(200,0,0);
			}
			FillRect(m, B_SOLID_LOW);
			DrawString("Mute", BPoint(m.left+1, m.bottom-2));
			SetHighColor(0,0,0);
			SetLowColor(200,200,200);
		}
		else {
			printf("no mute for %s\n", mSliders[ix].name);
		}
		m = mSliders[ix].area;
		m.left += 1;
		m.right -= 1;
		m.top += 16;
		m.bottom -= 32;
		if (mSliders[ix].cur < mSliders[ix].min || 
			mSliders[ix].cur > mSliders[ix].max) {
			printf("%e < %e < %e ? (%s)\n", mSliders[ix].min, 
				mSliders[ix].cur, mSliders[ix].max, mSliders[ix].name);
		}
		m.bottom -= (m.Height()-15.0)*(mSliders[ix].cur-mSliders[ix].min)/
			(mSliders[ix].max-mSliders[ix].min); /* */
		m.top = m.bottom-15.0;
		StrokeRect(m);
		m.InsetBy(1,1);
		SetHighColor(150,150,150);
		FillRect(m, B_SOLID_HIGH);
		SetHighColor(200,0,0);
		SetLowColor(150,150,150);
		/* StrokeLine(BPoint(m.left+1,(m.top+m.bottom)/2), 
			BPoint(m.right-1,(m.top+m.bottom)/2)); /* */
		sprintf(str, "%+4.1f", mSliders[ix].cur);
		DrawString(str, BPoint(m.left+1, m.top+12));
		SetHighColor(0,0,0);
		SetLowColor(255,255,255);
	}
}


status_t
MixView::Error()
{
	if (mFd < 0) {
		return mFd;
	}
	return B_OK;
}

void
MixView::GetPreferredSize(
	float * width,
	float * height)
{
	if (!mSliderCnt) {
		*width = 50;
		*height = 20;
	}
	else {
		*width = mSliders[mSliderCnt-1].area.right;
		*height = mSliders[mSliderCnt-1].area.bottom;
	}
}



static void
MakeWindow(
	const char * driver)
{
	BWindow * w = new BWindow(BRect(100,100,200,300), "Mixer", B_TITLED_WINDOW, 
		0);
#if 0
	char path[200];
	sprintf(path, "/dev/audio/mix/%s", driver);
	MixView  * m = new MixView(path);
	if (m->Error()) {
		perror("cannot do MixView\n");
		delete w;
	}
	else {
		float h, v;
		m->GetPreferredSize(&h, &v);
		printf("w %g  h %g\n", h, v);
		m->ResizeTo(h, v);
		w->ResizeTo(h, v);
		w->AddChild(m);
		w->Show();
	}
#else
	w->Show();
#endif
}


static status_t
mux(
	void * driver)
{
	char path[200];
	sprintf(path, "/dev/audio/mux/%s", driver);
	int fd = open(path, O_RDWR);
	if (fd < 0) {
		perror("cannot open mux");
		return -1;
	}
	while (!quitf) {
		sonic_vibes_routing rte;
		int val;
		if (read(fd, &rte, sizeof(rte)) > 0) {
			sonic_vibes_routing_name rna;
			rna.selector = 0;
			ioctl(fd, B_ROUTING_GET_NAME, &rna, sizeof(rna));
			printf("sel %d [%s]: %d\n", rte.selector, rna.name, rte.value);
			char str[200];
			gets(str);
			if (sscanf(str, "%d", &val) == 1) {
				rte.selector = 0;
				rte.value = val;
				if (write(fd, &rte, sizeof(rte)) != sizeof(rte)) {
					printf("error %x\n", errno);
				}
			}
		}
	}
	close(fd);
	return 0;
}


void
main(
	int argc,
	char * argv[])
{
	status_t status;
	const char * driver;
	if (argc > 2) {
		fprintf(stderr, "usage: svt [driver]\n");
	}
	if (argc == 2) {
		driver = argv[1];
	}
	else {
		driver = "sonic_vibes_1";
	}
	BApplication app("application/x-svt");
	signal(SIGINT, quit);
	thread_id pcm_t = spawn_thread(pcm, "pcm", B_REAL_TIME_PRIORITY, (void*)driver);
	MakeWindow(driver);
	resume_thread(pcm_t);
#if 0
	resume_thread(spawn_thread(mux, "mux", B_NORMAL_PRIORITY, (void*)driver));
#endif
	app.Run();
	wait_for_thread(pcm_t, &status);
	exit(0);
}

