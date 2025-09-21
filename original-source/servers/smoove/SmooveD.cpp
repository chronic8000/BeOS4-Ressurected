
#include <stdio.h>
#include <string.h>
#include "SmooveD.h"
#include "SmooveDServer.h"

int main()
{	
	FlunkyServer app;
	app.Run();
	return (B_NO_ERROR);
}

FlunkyServer::FlunkyServer() : BApplication(SmooveDSignature, NULL, false)
{
	m_flunkies.AddEnvVar("ADDON_PATH", "smoove");
	m_dispatcher = NULL;//new GDispatcher();
}

void FlunkyServer::ArgvReceived(int32 argc, char **argv)
{
	int32 i=1;
	while (i<argc) {
		if (strcasecmp(argv[i], "--quit") == 0)
			PostMessage(B_QUIT_REQUESTED);
		else
			printf("flunkyd: unrecognized argument \"%s\"\n", argv[i]);
		i++;
	}
}

void FlunkyServer::ReadyToRun()
{
	int32 count,i;
	HandlerHandle *handle;

	m_flunkies.Scan();
	count = m_flunkies.CountAddOns();
	for (i=0;i<count;i++) {
		handle = (HandlerHandle*)m_flunkies.AddOnAt(i);
		handle->Launch(m_dispatcher);
		GHandler *h = handle->Handler();
		(void)h;
	}
	
	PostMessage('done',this);
}

void FlunkyServer::MessageReceived(BMessage	*msg)
{
	int32 i,count;
	HandlerHandle *handle;
	BMessage reply('rply');
//	printf("smooved: "); msg->PrintToStream();

	switch (msg->what) {
		case 'done': rename_thread(find_thread(NULL),"bringiton"); break;
		case bmsgListFlunkies:
		case bmsgPassToFlunky:
		case bmsgFetchFlunky: {
			const char *name = msg->FindString("flunky_name");
			count = m_flunkies.CountAddOns();
			for (i=0;i<count;i++) {
				handle = (HandlerHandle*)m_flunkies.AddOnAt(i);
				GHandler *handler = handle->Handler();
				if (handler) {
					if (msg->what == bmsgListFlunkies)
						reply.AddString("flunkies",handler->Name());
					else if (name && !strcasecmp(name,handler->Name()?handler->Name():"")) {
						if (msg->what == bmsgFetchFlunky) {
							reply.AddMessenger("flunky_messenger",BMessenger(handler));
							msg->SendReply(&reply);
						} else if (msg->what == bmsgPassToFlunky) {
							BMessage m;
							msg->FindMessage("flunky_message",&m);
							handler->PostMessage(m);
						}
						return;
					}
				}
			}
			msg->SendReply(&reply);
		} break;

		default:
			BApplication::MessageReceived(msg);
			return;
	}

}

bool FlunkyServer::QuitRequested()
{
	return true;
}

void HandlerHandle::ImageUnloading(image_id )
{
	m_handler = NULL;
}

void HandlerHandle::Launch(GDispatcher *dispatcher)
{
	void* maker;
	
	image_id image = Open();
	if (image < B_OK) return;

	if (get_image_symbol(image, "return_handler", B_SYMBOL_TYPE_TEXT, &maker) == B_OK)
		m_handler = (*((return_handler_type)maker))(dispatcher);
}
