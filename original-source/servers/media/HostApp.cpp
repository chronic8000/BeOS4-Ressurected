/*	HostApp.cpp	*/

#include <MediaRoster.h>
#include "trinity_p.h"
#include "HostCmd.h"
#include "HostApp.h"
#include "MLatentManager.h"
#include "MSoundEventManager.h"
#include "TextScrollView.h"
#include "smart_array.h"
#include "AudioMsgs.h"		//	for AUDIO_SERVER_ID
#include "MediaFileInit.h"

#include <OS.h>
#include <Window.h>
#include <View.h>
#include <TranslationUtils.h>
#include <Roster.h>
#include <Screen.h>
#include <MediaFiles.h>
#include <SoundPlayer.h>
#include <Sound.h>
#include <Autolock.h>

#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>


#if NDEBUG 
#define FPRINTF
#else
#define FPRINTF fprintf
#endif

//	to not trigger "grep printf"
#define FPrintf fprintf
#define SPrintf sprintf

#define AUDIO_SERVER FPRINTF
#define DEFAULT FPRINTF

#if DEBUG
int debug_level = 1;
#else
int debug_level = 0;
#endif
bool g_about = false;
const char * about_text = 
"#000000media_addon_server\n"
"©1998-1999 Be Incorporated.\n"
"All rights reserved.\n"
#if DEBUG
"#ff0000DEBUG BUILD "
#endif
"Built " __DATE__ " " __TIME__ "\n"
"\n\n\n\n"
"The add-on host provides valuable\n"
"media add-on hosting services. It\n"
"is not to be disturbed.\n"
"\n\n\n\n\n\n\n\n"
"You really should be spending your\n"
"valuable time more wisely, you know.\n"
"\n\n\n\n\n\n\n\n"
"#ff0000My name is Jon Watte\n"
"and I carry a badge...\n"
"<insert theme from Dragnet here>\n"
;


void _fatal(const char * fmt, status_t err, const char * file, int32 line);
void _fatal(const char * fmt, status_t err, const char * file, int32 line)
{
	FPrintf(stderr, "%s, %d: QUITTING: %x (%s)\n", file, line, err, fmt);
#if DEBUG
	abort();
#else
	exit(1);
#endif
}



_HostApp::_HostApp(
	const char * signature) :
	BApplication(signature)
{
	m_about = NULL;
	m_has_sound = false;

	mLatentManager = new MLatentManager;
	dlog("_HostApp: LatentManager created");
	m_soundEventManager = new MSoundEventManager;
	m_audio_server_start = -1;
}


_HostApp::~_HostApp()
{
	int tries = 0;
	if (m_audio_server_start > 0) {
		volatile sem_id * ptr = &m_audio_server_start;
		while (*ptr > 0) {
			if (tries++ > 10) {
				break;
			}
			release_sem(m_audio_server_start);
			snooze(10000);
		}
	}

	status_t err = mLatentManager->SaveState();
	delete mLatentManager;
	
	delete m_soundEventManager;
}


static status_t audio_server_starter(void * arg)
{
	sem_id * m_audio_server_start = (sem_id *)arg;
	BMessenger msgr(AUDIO_SERVER_ID);
	while (msgr.IsValid()) {
		AUDIO_SERVER(stderr, "Trying to quit audio_server\n");
		BMessage quit(B_QUIT_REQUESTED);
		BMessage reply;
		msgr.SendMessage(&quit, &reply);
		if (acquire_sem_etc(*m_audio_server_start, 1, B_TIMEOUT, 300000) < B_OK) {
			return 0;
		}
	}
	team_id team = 0;
	int tries = 0;
	while (be_roster->Launch(AUDIO_SERVER_ID, (BMessage *)NULL, &team) < B_OK) {
		AUDIO_SERVER(stderr, "Trying to start audio_server\n");
		if (tries++ > 10) {
			break;
		}
		if (acquire_sem_etc(*m_audio_server_start, 1, B_TIMEOUT, 1000000) < B_OK) {
			return 0;
		}
	}
	sem_id del = *m_audio_server_start;
	*m_audio_server_start = -1;
	delete_sem(del);
	return B_OK;
}

void
_HostApp::ReadyToRun()
{
	BApplication::ReadyToRun();
	set_thread_priority(find_thread(NULL), 20);
	status_t err = B_OK;
	(void)BMediaRoster::Roster(&err);
	if (err != B_OK) {
		dlog("Error finding the media server: %x (%s)", err, strerror(err));
		PostMessage(B_QUIT_REQUESTED);
		return;
	}
	
	RegisterCodecs();
	
	mLatentManager->LoadState();
	mLatentManager->SetNotify(NewFlavor, this);
	mLatentManager->DiscoverAddOns();
	mLatentManager->AutoStartNodes();
	mLatentManager->StartDefaultNodes();

	/* connect the mixer to the sound card */
	m_has_sound = (HookupAudio() >= B_OK);

	((_BMediaRosterP *)BMediaRoster::Roster())->SetDefaultHook(DefaultChanged, this);

	// +em: do we need an external API to control which events get preloaded?
	m_soundEventManager->Preroll("Beep");
	m_soundEventManager->Preroll("Mouse Down");
	m_soundEventManager->Preroll("Mouse Up");
	m_soundEventManager->Preroll("Key Down");
	m_soundEventManager->Preroll("Key Repeat");
	m_soundEventManager->Preroll("Key Up");
	m_soundEventManager->Preroll("Window Open");
	m_soundEventManager->Preroll("Window Close");
	m_soundEventManager->Preroll("Window Activated");
	m_soundEventManager->Preroll("Window Minimized");
	m_soundEventManager->Preroll("Window Restored");
	m_soundEventManager->Preroll("Window Zoomed");
	
	BMessage startsound(MEDIA_BEEP);
	startsound.AddString("be:media_type", BMediaFiles::B_SOUNDS);
	startsound.AddString("be:media_name", "Startup");
	PostMessage(&startsound);

	m_audio_server_start = create_sem(0, "audio_server start");
	resume_thread(spawn_thread(audio_server_starter, "audio_server_starter", B_NORMAL_PRIORITY, &m_audio_server_start));
}


void
_HostApp::MessageReceived(
	BMessage * message)
{
	switch (message->what) {
	case ADDON_HOST_QUIT: {
			//	What do we do with running Nodes?
			//	They should be given a chance to clean up first.
			dlog("_HostApp: ADDON_HOST_QUIT");
			//	Kill the audio server
			BMessenger msgr(AUDIO_SERVER_ID);
			if (msgr.IsValid()) {
				BMessage quit(B_QUIT_REQUESTED);
				BMessage reply;
				msgr.SendMessage(&quit, &reply);
			}
			//	Kill ourselves
			Quit();
		}
		break;
	case MEDIA_QUERY_LATENTS: {
			dlog("_HostApp: MEDIA_QUERY_LATENTS");
			QueryLatents(message);
		}
		break;
	case MEDIA_ACQUIRE_NODE_REFERENCE: {
			dlog("_HostApp: MEDIA_ACQUIRE_NODE_REFERENCE");
			GetNodeFor(message);
		}
		break;
	case MEDIA_INSTANTIATE_PERSISTENT_NODE: {
			dlog("_HostApp: MEDIA_INSTANTIATE_PERSISTENT_NODE");
			InstantiateDormantNode(message);
		}
		break;
	case MEDIA_SNIFF_FILE: {
			dlog("_HostApp: MEDIA_SNIFF_FILE");
			SniffRef(message);
		}
		break;
	case MEDIA_BEEP: {
			dlog("_HostApp: MEDIA_BEEP");
			DoBeep(message);
		} break;
	case MEDIA_SOUND_EVENT_CHANGED: {
			RefreshBeep(message);
		} break;
	case MEDIA_GET_DORMANT_FLAVOR: {
			dlog("_HostApp: MEDIA_GET_DORMANT_FLAVOR");
			GetDormantFlavor(message);
		} break;
	case MEDIA_GET_DORMANT_FILE_FORMATS: {
			dlog("_HostApp: MEDIA_GET_DORMANT_FILE_FORMATS");
			GetDormantFileFormats(message);
		} break;
	case MEDIA_GET_LATENT_INFO: {
			GetLatentInfo(message);
		} break;
	case NEW_FLAVOR_MSG: {
			int32 addon;
			if (!message->FindInt32("be:addon_id", &addon)) {
				mLatentManager->RescanFlavors(addon);
				((_BMediaRosterP *)BMediaRoster::CurrentRoster())->Broadcast(*message, B_MEDIA_FLAVORS_CHANGED);
			}
		} break;
	default: {
			dlog("_HostApp: unknown message");
			BApplication::MessageReceived(message);
		}
		break;
	}
}


bool
_HostApp::QuitRequested()
{
	bool shortcut = false;
	if (!CurrentMessage()->FindBool("shortcut", &shortcut) && shortcut)
		return false;
	BMessenger msgr(B_MEDIA_SERVER_SIGNATURE);
	if (!msgr.IsValid()) {
		return true;
	}
	msgr.SendMessage(B_QUIT_REQUESTED);
	return false;	/* we only accept quit requests from media_server if it's running */
}


void
_HostApp::AboutRequested()
{
	if (m_about->Lock()) {	
		m_about->Activate(true);
		m_about->Unlock();
		return;
	}
	m_about = new BWindow(BRect(0,0,383,127), "About addon_host", B_TITLED_WINDOW, 
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE);
	BView * bg = new BView(m_about->Bounds(), "background", B_FOLLOW_NONE, B_WILL_DRAW);
	m_about->AddChild(bg);
	bg->SetViewColor(255,255,255);
#if defined(_PR3_COMPATIBLE_)
	BBitmap * picture = BTranslationUtils::GetBitmap("addon_about.tga");
	if (picture)
		bg->SetViewBitmap(picture, B_FOLLOW_TOP | B_FOLLOW_LEFT, 0);
#endif
	BRect r = bg->Bounds();
	r.right -= 150;
	r.bottom -= 20;
	TextScrollView * tv = new TextScrollView(r, "about text", B_FOLLOW_NONE);
	bg->AddChild(tv);
	tv->SetText(about_text);
	tv->MoveTo(139,10);
	{
		BScreen scrn;
		r = scrn.Frame();
	}
	m_about->MoveTo((r.Width()-m_about->Frame().Width())/2, 
		(r.Height()-m_about->Frame().Height())/3);
	m_about->Show();
}



void
_HostApp::QueryLatents(
	BMessage * message)
{
	BMessage reply;
	int32 max_hits = 0;
	const media_format * input = NULL;
	const media_format * output = NULL;
	const char * name = NULL;
	uint64 req_kind = 0;
	uint64 deny_kind = 0;
	ssize_t size;
	status_t err = message->FindInt32("be:max_hits", &max_hits);
	if (err < B_OK) goto error;
	if (B_OK > message->FindData("be:input_format", B_RAW_TYPE, (const void **)&input, &size)) 
		input = NULL;
	if (B_OK > message->FindData("be:output_format", B_RAW_TYPE, (const void **)&output, &size)) 
		output = NULL;
	if (B_OK > message->FindString("be:name", &name)) name = NULL;
	if (B_OK > message->FindInt64("be:require_kinds", (int64*)&req_kind)) req_kind = 0;
	if (B_OK > message->FindInt64("be:deny_kinds", (int64*)&deny_kind)) deny_kind = 0;
	err = mLatentManager->QueryLatents(input, output, name, reply, max_hits, 
		req_kind, deny_kind);
error:
	reply.AddInt32("error", err);
	message->SendReply(&reply);
}


void
_HostApp::GetNodeFor(
	BMessage * message)
{
	debugger("_HostApp::GetNodeFor should not be called\n");

	media_node_id node;
	media_node clone;
	status_t error = message->FindInt32("media_node_id", &node);
	if (error == B_OK) {
		if (node < 0) {
			status_t error = mLatentManager->InstantiateLatent(*message, clone);
		}
		else {
			FPRINTF(stderr, "_HostApp::GetNodeFor called for positive node ID\n");
			error = B_MEDIA_BAD_NODE;
		}
	}
	BMessage reply;
	if (error == B_OK) {
		error = reply.AddData("media_node", B_RAW_TYPE, &clone, sizeof(clone));
	}
	reply.AddInt32("error", error);
	message->SendReply(&reply);
}


void
_HostApp::InstantiateDormantNode(
	BMessage * message)
{
	media_addon_id addon;
	int32 flavor;
	status_t error = message->FindInt32("be:_addon", &addon);
	if (error == B_OK)
		error = message->FindInt32("be:_flavor_id", &flavor);

	media_node node;
	if (error == B_OK) {
		/* find add-on */
		latent_info * latent = NULL;
		error = mLatentManager->FindLatent(addon, &latent);
		if (error == B_OK) {
			BMessage empty;
			str_ref path(latent->path);
			error = mLatentManager->InstantiateLatent(path.str(), flavor, empty, node);
		}
	}
	
	BMessage reply;
	if (error == B_OK)
		reply.AddData("be:_media_node", B_RAW_TYPE, &node, sizeof(node));

	reply.AddInt32("error", error);
	message->SendReply(&reply);
}


void
_HostApp::SniffRef(
	BMessage * message)
{
	entry_ref file;
	status_t err;
	BMessage reply;
	int sniffed = 0;
	uint64 kinds;
	const char * type;
//	err = message->FindMessage("be:_initial_reply", &reply);
	if ((err = message->FindInt64("be:kinds", (int64*)&kinds)) == B_OK) {
		if ((err = message->FindRef("refs", sniffed, &file)) >= B_OK) {
			err = mLatentManager->SniffRef(file, kinds, reply, "be:latent_sniffs", 
				"be:sniff_mime");
			sniffed++;
		}
		else if ((err = message->FindString("be:mime_type", sniffed, &type)) >= B_OK) {
			err = mLatentManager->SniffType(type, kinds, reply, "be:type_sniffs");
			sniffed++;
		}
	}
	if ((err == B_BAD_INDEX) && (sniffed > 0)) {
		err = B_OK;
	}
	reply.AddInt32("error", err);
	message->SendReply(&reply);
}

void
_HostApp::GetDormantFlavor(
	BMessage * message)
{
	BMessage reply;
	status_t error;
	dormant_node_info * dormant;
	ssize_t size;
	if ((error = message->FindData("be:_dormant", B_RAW_TYPE, (const void **)&dormant, &size) >= B_OK)) {
		dormant_flavor_info info;
		char path[PATH_MAX];
		error = mLatentManager->GetFlavorFor(*dormant, &info, path, sizeof(path));
		if (error >= B_OK) {
			error = reply.AddFlat("be:_flavor", &info);
		}
		if (error >= B_OK) {
			error = reply.AddString("be:_path", path);
		}
	}
	reply.AddInt32("error", error);
	message->SendReply(&reply);
}


void
_HostApp::GetDormantFileFormats(
	BMessage * message)
{
	BMessage reply;
	status_t error;
	int32 rcnt = 0;
	int32 rused = 5;
	int32 wcnt = 0;
	int32 wused = 5;
	media_file_format * rfmts = NULL;
	media_file_format * wfmts = NULL;
	const dormant_node_info * info;
	ssize_t size;
	error = message->FindData("be:dormant_node", B_RAW_TYPE, (const void **)&info, &size);
	if (error >= B_OK) do {
		delete[] rfmts;
		delete[] wfmts;
		rcnt = rused+5;
		wcnt = wused+5;
		rfmts = new media_file_format[rcnt];
		wfmts = new media_file_format[wcnt];
		rused = 0;
		wused = 0;
		error = mLatentManager->GetFileFormatsFor(*info, rfmts, rcnt, &rused,
			wfmts, wcnt, &wused);
	} while (rused > rcnt || wused > wcnt);
	if (error >= B_OK) for (int ix=0; ix<rused; ix++) {
		error = reply.AddData("be:read_formats", B_RAW_TYPE, &rfmts[ix], sizeof(media_file_format));
		if (error < B_OK) break;
	}
	if (error >= B_OK) for (int ix=0; ix<wused; ix++) {
		error = reply.AddData("be:write_formats", B_RAW_TYPE, &wfmts[ix], sizeof(media_file_format));
		if (error < B_OK) break;
	}
	delete[] rfmts;
	delete[] wfmts;
	reply.AddInt32("error", error);
	message->SendReply(&reply);
}

void 
_HostApp::GetLatentInfo(BMessage *message)
{
	flavor_info *flinfo = 0;
	status_t err;
	int32 addonID;
	int32 flavorID;
	BMessage reply;
	dormant_flavor_info flattenableInfo;
	if (message->FindInt32("be:_addon_id", &addonID) != B_OK ||
		message->FindInt32("be:_flavor_id", &flavorID) != B_OK) {
		reply.AddInt32("error", B_NAME_NOT_FOUND);
		goto done;	
	}

	latent_info *info;
	err = mLatentManager->FindLatent(addonID, &info);
	if (err != B_OK) {
		reply.AddInt32("error", err);
		goto done;
	}
	
	for (int ix = 0; ix < info->flavors->size(); ix++) {
		if ((*info->flavors)[ix].internal_id == flavorID) {
			if (!(*info->flavors)[ix].enabled) {
				reply.AddInt32("error", B_MEDIA_ADDON_DISABLED);
				goto done;
			}
		
			flinfo = &(*info->flavors)[ix];
			break;
		}
	}

	if (flinfo == 0) {
		reply.AddInt32("error", B_MEDIA_BAD_FORMAT);
		goto done;	
	}
	
	reply.AddString("be:_addon_path", info->path.str());

	flattenableInfo = *flinfo;
	reply.AddFlat("be:_flavor_info", &flattenableInfo);

done:

	message->SendReply(&reply);
}

// +em: play_tokens suck.
void
_HostApp::DoBeep(
	BMessage * message)
{
	status_t err = B_OK;
	sem_id token = -1;
	float gain = 1.0;
	entry_ref file;

	if (!m_has_sound) {
		err = B_MEDIA_SYSTEM_FAILURE;
		goto send_reply;
	}
	
	message->FindFloat("be:_volume", &gain);

	if (message->FindRef("be:file", &file) >= B_OK) {
		err = m_soundEventManager->Play(file, gain, &token);
	}
	else {
		const char * type = NULL;
		const char * name = NULL;
		if (message->FindString("be:media_type", &type)) {
			type = BMediaFiles::B_SOUNDS;
		}
		if (message->FindString("be:media_name", &name)) {
			name = "Beep";
		}
		if (debug_level > 0) {
			FPRINTF(stderr, "type %s name %s\n", type, name);
		}
		if (strcmp(type, BMediaFiles::B_SOUNDS)) {
			if (debug_level > 0) FPrintf(stderr, "!!! unsupported type.\n");
			err = B_BAD_TYPE;
			goto send_reply;
		}
		
		// +em: should caller have control over whether event is persistent?
		
		err = m_soundEventManager->Play(name, &token, false);
	}

send_reply:
	BMessage reply;
	if (err >= B_OK) {
		if (debug_level > 0) FPrintf(stderr, "returning play_token %d\n", token);
		reply.AddInt32("be:play_token", token);
	}
	else {
		reply.AddInt32("error", err);
	}
	message->SendReply(&reply);
}

void 
_HostApp::RefreshBeep(BMessage *message)
{
	const char* event;
	status_t err = message->FindString("be:media_name", &event);
	if (err == B_OK) {
		m_soundEventManager->EventChanged(event);
	}
}


status_t
_HostApp::HookupAudio()
{
	status_t err = B_ERROR;
	media_node mixer;
	media_node soundcard;

	FPRINTF(stderr, "HookupAudio()\n");

	//	force reference to all the defaults
	BMediaRoster * r = BMediaRoster::CurrentRoster();
	media_node ainput, vinput, voutput;
	(void)r->GetAudioInput(&ainput);
	(void)r->GetVideoInput(&vinput);
	(void)r->GetVideoOutput(&voutput);

	int32 input_id = -1;
	BString input_name;

	if ((B_OK <= BMediaRoster::Roster()->GetAudioMixer(&mixer)) &&
		(B_OK <= BMediaRoster::Roster()->GetAudioOutput(&soundcard, &input_id, &input_name))) {
		if ((mLatentManager->SoundCard() != soundcard) || (m_mixer != mixer)) {
			FPRINTF(stderr, "Change in mixer/sound card detected\n");
			media_input sc_in;
			media_output mix_out;
			int32 count = 0;
			//	disconnect old stuff
			if (mLatentManager->SoundCard() != media_node::null && m_mixer != media_node::null) {
				//	find outbound mixer connection
				smart_array<media_output> outputs(8);
				smart_array<media_input> inputs(16);
				smart_array<media_output> disco(8);
				int32 mix_out = 0;
				int32 card_in = 0;
				int32 disco_cnt = 0;
				err = BMediaRoster::Roster()->GetConnectedOutputsFor(m_mixer,
					outputs, 8, &mix_out);
				//	find incoming sound card connection
				if (err == B_OK) {
					if (mix_out != 1) {
						FPRINTF(stderr, "Warning: %d != 1 outgoing connection from audio mixer!\n", count);
					}
					count = 0;
					err = BMediaRoster::Roster()->GetConnectedInputsFor(mLatentManager->SoundCard(),
						inputs, 16, &card_in);
				}
				//	match up the connections
				if (err == B_OK) {
					if (card_in != 1) {
						FPRINTF(stderr, "Warning: %d != 1 outgoing connection from audio mixer!\n", count);
					}
					for (int iy=0; iy<mix_out; iy++) {
						for (int ix=0; ix<card_in && iy<mix_out; ix++) {
							if ((inputs[ix].destination == outputs[iy].destination) &&
								(inputs[ix].source == outputs[iy].source)) {
								if (disco_cnt < disco.capacity()) {
									disco.operator[](disco_cnt++) = outputs[iy];
									iy++;	// we have this; look for next (if any)
									ix=-1;	// re-start loop
									continue;
								}
							}
						}
					}
					//	disconnect the match(es) we found
					for (int ix=0; ix<disco.size(); ix++) {
						status_t e = BMediaRoster::Roster()->Disconnect(m_mixer.node,
							disco[ix].source, mLatentManager->SoundCard().node, disco[ix].destination);
						if (e < B_OK) {
							err = e;
						}
					}
				}
			}
			if (err < B_OK) {
				FPRINTF(stderr, "_HostApp: error disconnecting sound mixer/card: %x (%s)\n",
					err, strerror(err));
			}
			count = 40;
			std::vector<media_input> many_inputs(40);
			if (B_OK <= BMediaRoster::Roster()->GetFreeInputsFor(soundcard, &many_inputs[0],
				20, &count, B_MEDIA_RAW_AUDIO) && (count >= 1)) {
				FPRINTF(stderr, "Hookup sound card free inputs count is %d\n", count);
				//	see if we can find an input that matches our saved criteria (if any)
				bool found_name = false;
				bool found_id = false;
				if ((input_id > 0) || (input_name.Length() > 0)) {
					for (int ix=0; ix<count; ix++) {
						if (!strcmp(input_name.String(), many_inputs[ix].name)) {
							sc_in = many_inputs[ix];
							found_name = true;
							if (found_id) break;
						}
						if (many_inputs[ix].destination.id == input_id) {
							if (!found_name) sc_in = many_inputs[ix];
							found_id = true;
							if (found_name) break;
						}
					}
				}
				if ((!found_name) && (!found_id)) {
					sc_in = many_inputs[0];	//	use the first input
				}
				if (B_OK <= BMediaRoster::Roster()->GetFreeOutputsFor(mixer, &mix_out,
					1, &count, B_MEDIA_RAW_AUDIO) && count >= 1) {
					FPRINTF(stderr, "Hookup mixer free outputs is %d\n", count);
					media_format format = sc_in.format;
					int32 prude = 0;
again:
					format.u.raw_audio.channel_count = 2;
					if (B_OK <= BMediaRoster::Roster()->Connect(mix_out.source, sc_in.destination,
						&format, &mix_out, &sc_in)) {
						if (debug_level > 0) FPrintf(stderr, "_HostApp: Connect() mixer to sound card worked\n");
						mLatentManager->SetSoundCard(soundcard);
						m_mixer = mixer;
						err = B_OK;
						media_node timenode;
						if (BMediaRoster::Roster()->GetTimeSource(&timenode) < B_OK) {
							if (debug_level > 0) FPrintf(stderr, "Using SystemTimeSource\n");
							BMediaRoster::Roster()->GetSystemTimeSource(&timenode);
						}
						if (debug_level > 0) FPrintf(stderr, "TIMING: souncard id %d, timenode id %d\n", soundcard.node, timenode.node);
						// hippo changed to SeekTimeSource
						//BMediaRoster::Roster()->SeekNode(timenode, -50000, BTimeSource::RealTime());
						BMediaRoster::Roster()->SeekTimeSource(timenode, -50000, BTimeSource::RealTime());
						
						BMediaRoster::Roster()->SetTimeSourceFor(soundcard.node, timenode.node);
						BMediaRoster::Roster()->SetTimeSourceFor(mixer.node, timenode.node);
						BTimeSource * the_timesource = BMediaRoster::Roster()->MakeTimeSourceFor(soundcard);
						ASSERT(the_timesource != NULL);
						ASSERT(the_timesource->ID() == timenode.node);
						if (the_timesource) {
							/** all is OK */
							bigtime_t real = BTimeSource::RealTime()+50000;
							bigtime_t then = the_timesource->PerformanceTimeFor(real);
							if (soundcard.kind & B_TIME_SOURCE) {
								// hippo changed to SeekTimeSource
								//BMediaRoster::Roster()->StartNode(soundcard, real);
								BMediaRoster::Roster()->StartTimeSource(soundcard, real);
							}
							else {
								BMediaRoster::Roster()->StartNode(soundcard, then);
							}
							BMediaRoster::Roster()->StartNode(mixer, then);
						}
					}
					else {
						if (prude++ < 1){
							format = mix_out.format;
							goto again;
						}
						if (debug_level > 0) {
							FPrintf(stderr, "_HostApp: Cannot Connect() mixer to sound card\n");
							char str[100];
							string_for_format(mix_out.format, str, 100);
							FPrintf(stderr, "...mixer output format: %s\n", str);
							string_for_format(sc_in.format, str, 100);
							FPrintf(stderr, "...sound card input format: %s\n", str);
						}
					}
				}
				else {
					if (debug_level > 0) FPrintf(stderr, "_HostApp: Audio Mixer has no free outputs\n");
				}
			}
			else {
				if (debug_level > 0) FPrintf(stderr, "_HostApp: Sound Card has no free inputs\n");
			}
		}
		else {
			err = B_OK;	/* no change */
		}
	}
	else {
		if (debug_level > 0) FPrintf(stderr, "_HostApp: Can't find sound card or mixer to connect\n");
	}
	return err;
}

static int
argument(
	const char ** args,
	int count)
{
	count = count;
	int ret = 0;
	if (!strcmp(*args, "--help") || !strcmp(*args, "-?")) {
usage:
		FPrintf(stderr, "addon_host: please let media_server start addon_host.\n");
		exit(1);
	}
	else if (!strcmp(*args, "--about")) {
		g_about = true;
		ret = 0;	/* used no extra args */
	}
	else if (!strcmp(*args, "--debug")) {
		/* nothing here */
		debug_level++;
		media_debug = true;
		ret = 0;
	}
	else {
		goto usage;
	}
	return ret;
}




sem_id quit_sem;

static void
die_gracefully(
	int sig)
{
	signal(sig, die_gracefully);
	release_sem(quit_sem);
}

static status_t
send_quit(
	void *)
{
	signal(SIGINT, die_gracefully);
	signal(SIGHUP, die_gracefully);
	acquire_sem(quit_sem);
	be_app->PostMessage(ADDON_HOST_QUIT);
	dlog("_HostApp send_quit() triggered");
	return 0;
}


static void
change_streams(
	int * argc,
	char *** argv)
{
	char logname[256];
	int fd;
	if (*argc > 2 && !strcmp((*argv)[1], "--log")) {
		strcpy(logname, (*argv)[2]);
		(*argc) -= 2;
		(*argv) += 2;
	}
	else {
		time_t now;
		time (&now);
		SPrintf(logname, "/var/log/log-media_addon_server-%x", now);
	}
	if ((fd = open(logname, O_RDWR|O_CREAT|O_TRUNC)) < 0) {
		FPrintf(stderr, "cannot create log file %s\n", logname);
		return;
	}
	fflush(stdout);
	fflush(stderr);
	close(1);
	close(2);
	dup(fd);
	dup(fd);
	close(fd);
#if !defined(_SERVER_LS_)
#define _SERVER_LS_ "Not a debug version."
#endif
	FPrintf(stderr, "Server built %s %s\n%s\n", __DATE__, __TIME__, _SERVER_LS_);
}

int
main(
	int argc,
	char ** argv)
{

	setpgid(0,0);
	// change_streams(&argc, &argv);
	thread_id thread;
	quit_sem = create_sem(0, "quit_sem");
	resume_thread(thread = spawn_thread(send_quit, "send_quit", B_LOW_PRIORITY, NULL));
	signal(SIGHUP, die_gracefully);
	signal(SIGINT, die_gracefully);

	setpgid(0, 0);

	while (argv[1]) {
		argv++;
		argc--;
		int delta = argument((const char **)argv, argc);
		argv += delta;
		argc -= delta;
	}
	dlog("_HostApp: Through arguments");
	_HostApp app("application/x-vnd.Be.addon-host");
	dlog("_HostApp: Starting app");
	if (g_about) {
		app.PostMessage(B_ABOUT_REQUESTED);
	}

	image_info iinfo;
	int32 cookie = 0;
	while (get_next_image_info(0, &cookie, &iinfo) == B_OK) {
		if (iinfo.type == B_APP_IMAGE) {
			media_realtime_init_image(iinfo.id, B_MEDIA_REALTIME_AUDIO |
				B_MEDIA_REALTIME_VIDEO);
		} else if (strcmp(iinfo.name, "/boot/beos/system/lib/libroot.so") == 0) {
			media_realtime_init_image(iinfo.id, B_MEDIA_REALTIME_AUDIO |
				B_MEDIA_REALTIME_VIDEO);
		} else if (strcmp(iinfo.name, "/boot/beos/system/lib/libmedia.so") == 0) {
			media_realtime_init_image(iinfo.id, B_MEDIA_REALTIME_AUDIO |
				B_MEDIA_REALTIME_VIDEO);
		} else if (strcmp(iinfo.name, "/boot/beos/system/lib/libbe.so") == 0) {
			media_realtime_init_image(iinfo.id, B_MEDIA_REALTIME_AUDIO |
				B_MEDIA_REALTIME_VIDEO);
		}
	}

	app.Run();
	dlog("_HostApp: Done");

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	delete_sem(quit_sem);
	status_t status;
	wait_for_thread(thread, &status);	/* wait for signal thread */

	/* now, kill all threads running for clean-up */
	thread_info tinfo;
	thread_id me = find_thread(NULL);
	team_id mine;
	get_thread_info(me, &tinfo);
	cookie = 0;
	mine = tinfo.team;
	bool sn = false;
	BList threads;

	set_thread_priority(me, 121);
//	media_realtime_init_thread(me, 0x4000, B_MEDIA_REALTIME_AUDIO |
//		B_MEDIA_REALTIME_VIDEO );

	while (!get_next_thread_info(mine, &cookie, &tinfo)) {
		if (tinfo.thread != me) {
			threads.AddItem((void *)tinfo.thread);
		}
	}

	thread_id roster_thread = -1;
	_BMediaRosterP * r = dynamic_cast<_BMediaRosterP *>(BMediaRoster::CurrentRoster());
	if (r != NULL) {
		r->BadMediaAddonsMayCrashHere(kYesIAmTheAddonServer);
	}
	for (int ix=0; ix<threads.CountItems(); ix++) {
		thread_id thread = (thread_id)threads.ItemAt(ix);
		thread_info tinfo;
		if (!get_thread_info(thread, &tinfo)) {
			BLooper * looper = BLooper::LooperForThread(tinfo.thread);
			if (looper != NULL) {
				//	don't quit the roster
				if (looper != r) {
					if (debug_level > 0) FPrintf(stderr, "%.1f: PostMessage(%d, B_QUIT_REQUESTED) [%s]\n",
						system_time()/1000.0, tinfo.thread, tinfo.name);
					looper->PostMessage(B_QUIT_REQUESTED);
				}
				else {
					roster_thread = tinfo.thread;
				}
			}
			else {
				if (debug_level > 0) FPrintf(stderr, "%.1f: send_signal(%d, SIGHUP) [%s]\n", system_time()/1000.0,
						tinfo.thread, tinfo.name);
				send_signal(tinfo.thread, SIGHUP);
			}
			snooze(20000);	/* snooze after each signal */
			sn = true;
		}
	}
	if (sn) {
		snooze(200000);	/* make some more time */
	}

	for (int ix=0; ix<threads.CountItems(); ix++) {
		thread_id thread = (thread_id)threads.ItemAt(ix);
		thread_info tinfo;
		if ((thread != roster_thread) && !get_thread_info(thread, &tinfo)) {
			if (debug_level > 0) FPrintf(stderr, "%.1f: suspend_thread(%d)\n", system_time()/1000.0, tinfo.thread);
			set_thread_priority(tinfo.thread, 1);
			suspend_thread(tinfo.thread);
		}
	}

	for (int ix=0; ix<threads.CountItems(); ix++) {
		thread_id thread = (thread_id)threads.ItemAt(ix);
		thread_info tinfo;
		if ((thread != roster_thread) && !get_thread_info(thread, &tinfo)) {
			if (debug_level > 0) FPrintf(stderr, "%.1f: kill_thread(%d) [%s]\n", system_time()/1000.0,
					tinfo.thread, tinfo.name);
			kill_thread(tinfo.thread);
		}
	}

	if (r != NULL) {
		if (debug_level > 0) FPrintf(stderr, "%.1f: Quitting the roster\n", system_time()/1000.0);
		r->Lock();
		r->Quit();
	}
	if (debug_level > 0) FPrintf(stderr, "All cleanup action has completed\n");
	set_thread_priority(me, 15);
	return 0;
}

void
_HostApp::DefaultChanged(
	int32 what,
	void * cookie)
{
#if 0
	if (what == DEFAULT_AUDIO_MIXER || what == DEFAULT_AUDIO_OUTPUT) {
		FPRINTF(stderr, "Change in audio set-up detected, reconfiguring audio\n");
		((_HostApp *)cookie)->m_has_sound = (((_HostApp *)cookie)->HookupAudio() >= B_OK);
	}
#endif
}

status_t 
_HostApp::NewFlavor(void *cookie, BMediaAddOn *addon)
{
	BMessage msg(NEW_FLAVOR_MSG);
	msg.AddInt32("be:addon_id", addon->AddonID());
	return ((_HostApp *)cookie)->PostMessage(&msg);
}

