//-------------------------------------------------------------
//
//	File:		server.cpp
//
//	Written by:	Benoit Schillings, Mathias Agopian
//
//	Copyright 1996-2000, Be Incorporated
//
//-------------------------------------------------------------

#define P	printf

#include <stdio.h>
#include <stdlib.h>

#include <Application.h>
#include <Beep.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <image.h>
#include <Node.h>
#include <StorageDefs.h>

// Needed for signatures
#include <pr_server.h>
#include <print/TransportIO.h>
#include <print/PrinterAddOn.h>

#include "server.h"
#include "DeskbarPrinter.h"
#include "PrintStream.h"

#define IO_BUFFER_SIZE	16384

//-------------------------------------------------------------

BServer::BServer()
	: 	BApplication(PSRV_PRINT_SERVER_SIG),
		fPrintersWatcher(NULL),
		fSpoolWatcher(NULL),
		fJobWatcher(NULL),
		fQuitRequested(false),
		fActiveJobCount(0)
{
	// Create user add-ons path
	BPath user;
	find_directory (B_USER_ADDONS_DIRECTORY, &user);
	user.Append("Print");
	mkdir(user.Path(), 0777);

	// Get/Create the printer directory
	find_directory(B_USER_PRINTERS_DIRECTORY, &fSettingPrinterDirectory);
	mkdir(fSettingPrinterDirectory.Path(), 0777);

	clear_printers();
}

void BServer::ReadyToRun()
{
	StartWatchingPrinters();		
}


//-------------------------------------------------------------

BServer::~BServer()
{
	// Make sure to remove the "Printer Status" icone from the deskbar
	DeskbarPrinter::Remove();
	
	// Stop watching the printer folder
	delete fPrintersWatcher;
	
	// Stop watching the spool folders (ie the jobs)
	delete fSpoolWatcher;

	// Stop watching Jobs (must be last deleted)
	delete fJobWatcher;	

	if (LockLooper())
	{ // Be sure to stop watching everything
		stop_watching(this, Looper());
		UnlockLooper();
	}
}

bool BServer::QuitRequested()
{
	P("PRINT-SERVER: QuitRequested() : fActiveJobCount is %ld\n", fActiveJobCount);

	if (fActiveJobCount > 0)
	{
		// Only quit if the message was sent from another team.
		// This prevents ALT-Q from quitting the print server.
		if (CurrentMessage()->IsSourceRemote() == false)
			return false;
	
		// Mark that we are in the process of quitting
		fQuitRequested = true;
		
		// Ask all printers to cancel their current job
		TPrintTools::CancelAllJobs();
	
		// We can't quit right now
		return false;
	}

	return true;
}

//-------------------------------------------------------------

void BServer::MessageReceived(BMessage *m)
{
	switch (m->what)
	{
		case DeskbarPrinter::MESSENGER:
			m->FindMessenger("messenger", &fDeskbarView);
			UpdateDeskbarStatus();
			break;
		default:
			BApplication::MessageReceived(m);
	}
}

//-------------------------------------------------------------

void BServer::StartWatchingPrinters()
{
	status_t err;

	// Watch the printers
	fPrintersWatcher = new WatchPrinters(this);		// Start watching printers
	fSpoolWatcher = new WatchSpool(this);
	fJobWatcher = new WatchJobs(this);

	// And then watch the jobs for each printers
	BEntry an_entry;
	BDirectory dir(fSettingPrinterDirectory.Path());
	while ((err = dir.GetNextEntry(&an_entry)) >= 0)
	{
		// Start watching this printer (add/remove jobs)
		node_ref nref;
		BNode node(&an_entry);
		node.GetNodeRef(&nref);
		fSpoolWatcher->StartWatching(nref, B_WATCH_DIRECTORY);

		// Start watching the jobs in this printer (attr change)
		LookForOldJobs(&an_entry);
	}
}

void BServer::LookForOldJobs(BEntry *entry)
{
	BDirectory printer(entry);
	BEntry job;
	status_t err;
	
	printer.Rewind();
	while((err = printer.GetNextEntry(&job)) >= 0)
	{
		// TODO: verify the signature

		// Add this job to our list, start monitoring
		entry_ref eref;
		job.GetRef(&eref);
		SpoolThisJob(eref);

		node_ref nref;
		job.GetNodeRef(&nref);
		SpoolJob *the_job = FindJobByNodeRef(&nref);
		if (the_job == NULL)
			continue;

		the_job->UpdateFlags();

		// Check if we must Process it or not
		BNode jobnode(&job);
		if (the_job->IsCancelling())
		{ // This job is in "Cancelling" state (printserver quitted/crashed before the job was really canceled)
			jobnode.WriteAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, PSRV_JOB_STATUS_CANCELLED, strlen(PSRV_JOB_STATUS_CANCELLED)+1);
		}
		else if (the_job->IsProcessing() || the_job->IsKnownState() == false)
		{ // This job is in "Processing" state. That means that the print_server died during printing - don't retry it
			uint32 errorcode = B_ERROR; // We actually don't know what happened
			jobnode.WriteAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, PSRV_JOB_STATUS_FAILED, strlen(PSRV_JOB_STATUS_FAILED)+1);
			jobnode.WriteAttr(PSRV_SPOOL_ATTR_ERRCODE, B_SSIZE_T_TYPE, 0, &errorcode, 4);
		}
		else
		{ // Trick: rewrite the same attribute, to generate a node monitor message
			char jobStatus[32];
			jobnode.ReadAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, jobStatus, sizeof(jobStatus));
			jobnode.WriteAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, jobStatus, strlen(jobStatus)+1);
		}
	}
}
 
//-------------------------------------------------------------

void BServer::clear_printers()
{
	status_t err;
	BEntry an_entry;
	BDirectory dir(fSettingPrinterDirectory.Path());
	while ((err = dir.GetNextEntry(&an_entry)) >= 0)
	{
		BNode node(&an_entry);
		if (node.InitCheck() == B_OK)
			fPrinterManager.set_printer_free(&node);
	}
}


// ------------------------------------------------------------------
// #pragma mark -


status_t BServer::add_on_take_job(BFile *job, BNode *printer)
{
	BMessage *threadData = new BMessage; // deleted in spawned thread...
	threadData->AddPointer("this",	(void *)this);
	threadData->AddPointer("job",	(void *)job);
	threadData->AddPointer("printer",(void *)printer);
	thread_id tid = spawn_thread((thread_func)_take_job_add_on_thread, "add_on_thread", B_NORMAL_PRIORITY, (void *)threadData);
	if (tid < 0)
	{ // There was an error launching the thread??
		// Set an error code to the spool file and set the job to "Failed"
		set_error_code(job, (status_t)tid);
		job->WriteAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, PSRV_JOB_STATUS_FAILED, strlen(PSRV_JOB_STATUS_FAILED)+1);
		delete job;
		delete printer;
		delete threadData;
		return (status_t)tid;
	}
	resume_thread(tid);
	return B_OK;
}

long BServer::_take_job_add_on_thread(BMessage *threadData)
{
	BServer *THIS;
	BFile *job;
	BDirectory *printer;
	threadData->FindPointer("this", (void **)&THIS);
	threadData->FindPointer("job", (void **)&job);
	threadData->FindPointer("printer", (void **)&printer);
	delete threadData;
	status_t err = THIS->take_job_add_on_thread(job, printer);
	delete job;
	delete printer;
	return err;
}

status_t BServer::take_job_add_on_thread(BFile *job, BDirectory *printer_node)
{
	image_id addon_image;
	status_t err = B_ERROR;

	// Mark the job as processing
	job->WriteAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, PSRV_JOB_STATUS_PROCESSING, strlen(PSRV_JOB_STATUS_PROCESSING)+1);

	// Handle the RAW jobs
	spool_header_t header;
	if (job->Read(&header, sizeof(header)) != sizeof(header))
		goto finished;
	err = B_OK;
	if (header.Type() == spool_header_t::RAW) {
		if (header.FirstPage() != -1)
			job->Seek(header.FirstPage(), SEEK_SET);
		BTransportIO transport(printer_node, job, IO_BUFFER_SIZE);
		if ((err = transport.InitCheck()) == B_OK) {
			atomic_add(&fActiveJobCount, 1);
			ssize_t read;
			ssize_t sent = 0;
			ssize_t length = IO_BUFFER_SIZE;
			void *buffer = (char *)malloc(length);
			if (buffer)	{
				do {
					if ((read = job->Read(buffer, length)) > 0)
						sent = transport.Write(buffer, read);
				} while ((read == length) && (sent == read));
				if (read < 0)		err = read;
				else if (sent < 0)	err = sent;
				free(buffer);
			} else {
				err = B_NO_MEMORY;
			}
			atomic_add(&fActiveJobCount, -1);
		}		
		goto finished;
	}

	// ------------------------------------------------------
	// Regular jobs

	job->Seek(0, SEEK_SET);
	addon_image = err = load_driver_addon(printer_node, job);
	if (addon_image >= 0)
	{
		BMessage *(*take_job)(BFile*, BNode*, BMessage*);
		BPrinterAddOn *(*instantiate_printer_addon)(int32, BTransportIO *, BNode *);

		// find the instantiate addon function
		if ((err = get_image_symbol(addon_image, B_INSTANTIATE_PRINTER_ADDON_FUNCTION, B_SYMBOL_TYPE_TEXT, (void **)&instantiate_printer_addon)) == B_OK)
		{ // Instantiate that addon
			BTransportIO transport(printer_node, job, IO_BUFFER_SIZE);
			if ((err = transport.InitCheck()) == B_OK)
			{
				BPrinterAddOn *printerAddOn;
				if ((printerAddOn = instantiate_printer_addon(0, &transport, printer_node))) { // TODO: should not always use index 0
					atomic_add(&fActiveJobCount, 1);
					err = printerAddOn->TakeJob(job);
					atomic_add(&fActiveJobCount, -1);
				}
				else
					err = B_BAD_DRIVER;
				delete printerAddOn;
			}
		}
		else if ((err = get_image_symbol(addon_image, "take_job", B_SYMBOL_TYPE_TEXT, (void**)&take_job)) == B_OK)
		{ // Try the old way of handling print job for compatibility
			BMessage data(B_REFS_RECEIVED);
			data.AddPointer("file", (void *)job);
			data.AddPointer("printer", (void *)printer_node);
			atomic_add(&fActiveJobCount, 1);
			delete (take_job(job, printer_node, &data));
			atomic_add(&fActiveJobCount, -1);
		}
		
		unload_add_on(addon_image);
	}
	else
	{ // Pick an error code
		err = ((err==B_ENTRY_NOT_FOUND) || (err==B_FILE_NOT_FOUND)) ? (B_NO_DRIVER) : (B_BAD_DRIVER);
	}

finished:
	// Set the printer to "free" state
	// This MUST be done before touching the attributes
	fPrinterManager.set_printer_free(printer_node);

	if ((err == B_OK) || (err == B_CANCELED))
	{
		// update the job state to "Completed"
		set_error_code(job, B_OK);
		job->WriteAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, PSRV_JOB_STATUS_COMPLETED, strlen(PSRV_JOB_STATUS_COMPLETED)+1);
	}
	else 
	{
		// Set an error code to the spool file and set the job to "Failed"
		set_error_code(job, err);
		job->WriteAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, PSRV_JOB_STATUS_FAILED, strlen(PSRV_JOB_STATUS_FAILED)+1);
	}

	// NOTE: we could implement here the "server always quit" feature (remove the fQuitRequested test)
	if ((fQuitRequested == true) && (fActiveJobCount <= 0))
		PostMessage(B_QUIT_REQUESTED);

	// beep beep
	if (err == B_OK)	system_beep("Print Job Finished");
	else				system_beep("Print Job Failed");
	return err;
}

void BServer::set_error_code(BNode *node, status_t error)
{
	const int32 e = error;
	if (node->WriteAttr(PSRV_SPOOL_ATTR_ERRCODE, B_SSIZE_T_TYPE, 0, &e, sizeof(e)) != sizeof(e))
		printf("print_server: %s [%08lx]\n", strerror(error), error);
}

image_id BServer::load_driver_addon(BNode *node, BNode *job)
{
	return TPrintTools::LoadDriverAddOn(node, job);
}


//-----------------------------------------------------------------
// #pragma mark -

void BServer::StartJob(SpoolJob *spjob)
{
	if (fQuitRequested == true)
	{ // If we are in the process of quitting, mark this job as "cancelling" and do nothing
		BNode job(&(spjob->entry));
		TPrintTools::CancelJob(&job);
		return;
	}

	// Start printing this job!
	BDirectory *printer = new BDirectory(spjob->printer);
	BFile *job = new BFile(&(spjob->entry), B_READ_ONLY);
	fPrinterManager.set_printer_busy(printer, job);
	add_on_take_job(job, printer);	// takes ownership of spool and printer
}

SpoolJob *BServer::SpoolThisJob(const entry_ref& eref)
{ 
	// If we are quitting, just don't record this job
	if (fQuitRequested == true)
		return NULL;

	// Record this job in the list, and begin to node_monitor it
	SpoolJob *job = new SpoolJob(eref);
	fSpoolJobList.AddItem(job);
	node_ref nref;
	BNode node(&eref);
	node.GetNodeRef(&nref);
	fJobWatcher->StartWatching(nref, B_WATCH_ATTR);
	return job;
}

void BServer::EndJob(SpoolJob *job)
{
	status_t result;
	if ((result = fJobWatcher->StopWatching(job->noderef)) != B_OK)
	{
		P("PRINT-SERVER: EndJob() : StopWatching returned %s\n", strerror(result));
	}

	// Remove the job from the list
	const int32 count = fSpoolJobList.CountItems();
	for (int32 i=0 ; i<count; i++)
	{
		SpoolJob *listjob = static_cast<SpoolJob *>(fSpoolJobList.ItemAt(i));
		if (*listjob == *job)
		{
			fSpoolJobList.RemoveItem(i);
			delete job;
			break;
		}
	}
}

SpoolJob *BServer::GetNextPendingJob(BDirectory * /*printer*/)
{
	// Must find a job that IsReady() and IsPending() on
	// any free printer.
	// In addition the printer of that job must have a transport which port is availlable
	const int32 count = fSpoolJobList.CountItems();
	for (int32 i=0 ; i<count ; i++)
	{
		SpoolJob *job = static_cast<SpoolJob *>(fSpoolJobList.ItemAt(i));
		if (job->IsReady() == false)	continue;
		if (job->IsPending() == false)	continue;
		if (fPrinterManager.is_printer_free(&(job->printer)) == false)	continue;
		return job;
	}
	return NULL;
}

SpoolJob *BServer::FindJobByNodeRef(node_ref *ref)
{
	const int32 count = fSpoolJobList.CountItems();
	for (int32 i=0 ; i<count ; i++)
	{
		SpoolJob *job = static_cast<SpoolJob *>(fSpoolJobList.ItemAt(i));
		if (job->noderef == *ref)
			return job;
	}
	return NULL;
}

status_t BServer::UpdateDeskbarStatus()
{
	BMessage statusMsg(DeskbarPrinter::STATUS);

	uint32 countPending = 0;
	uint32 countCompleted = 0;
	uint32 countFailed = 0;
	uint32 countCancelled = 0;
	uint32 countProcessing = 0;

	const int32 count = fSpoolJobList.CountItems();
	for (int32 i=0 ; i<count ; i++)
	{
		SpoolJob *job = static_cast<SpoolJob *>(fSpoolJobList.ItemAt(i));
		if (job->IsReady() == false)
			continue;
		if (job->IsCompleted()) { countCompleted++; continue; }

		entry_ref eref;
		job->entry.GetRef(&eref);

		if (job->IsPending())
		{
			countPending++;
			statusMsg.AddRef("pending", &eref);
			continue;
		}

		if (job->IsFailed())
		{
			countFailed++;
			statusMsg.AddRef("failed", &eref);
			continue;
		}

		if (job->IsCancelled())
		{
			countCancelled++;
			statusMsg.AddRef("cancelled", &eref);
			continue;
		}

		countProcessing++;	// Other status are counted as processing
		statusMsg.AddRef("processing", &eref);
	}

	// Install or remove the replicant
	if (countProcessing || countFailed)
		DeskbarPrinter::Install(BMessenger(this));
	else
		return DeskbarPrinter::Remove();

	// We didn't get a messenger to our replicant yet - we can't communicate with it
	if (fDeskbarView.IsValid() == false)
		return B_OK; // This is not an error

	// Send the status to the deskbar replicant
//	statusMsg.AddInt32("pending", countPending);
//	statusMsg.AddInt32("failed", countFailed);
//	statusMsg.AddInt32("cancelled", countCancelled);
//	statusMsg.AddInt32("processing", countProcessing);

	return fDeskbarView.SendMessage(&statusMsg);
}

//-----------------------------------------------------------------
// #pragma mark -

SpoolJob::SpoolJob(const entry_ref& eref)
	:	flags(0)
{
	// We need a BEntry
	entry.SetTo(&eref);
	
	// We need a node_ref
	BNode node(&eref);
	node.GetNodeRef(&noderef);

	// We need the parent directory (ie: the printer)
	node_ref dir;
	dir.device = eref.device;
	dir.node = eref.directory;
	printer.SetTo(&dir);
}

SpoolJob::~SpoolJob()
{
	// Delete the file only if job is "Cancelled" or "Completed"
	if (IsCancelled() || IsCompleted())
		entry.Remove();
}

void SpoolJob::UpdateFlags()
{
	BNode job(&entry);
	status_t err;
	char status[32];
	char printer_attr[128];
	if ((err = job.ReadAttr(PSRV_SPOOL_ATTR_PRINTER, B_STRING_TYPE, 0, printer_attr, sizeof(printer_attr))) > 0)
		flags |= HAS_PRINTER;	
	if ((err = job.ReadAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, status, sizeof(status))) > 0)
		flags |= HAS_STATUS;
}

bool SpoolJob::IsReady()
{
	return (flags == HAS_EVERYTHING);
}

bool SpoolJob::TestStatus(const char *state)
{
	BNode job(&entry);
	char status[32];
	status_t err = job.ReadAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, status, sizeof(status));
	if (err < 0)
		return true;	
	return (strncmp(status, state, strlen(state)) == 0);
}

bool SpoolJob::IsPending()
{
	return TestStatus(PSRV_JOB_STATUS_WAITING);
}

bool SpoolJob::IsCompleted()
{
	return TestStatus(PSRV_JOB_STATUS_COMPLETED);
}

bool SpoolJob::IsFailed()
{
	return TestStatus(PSRV_JOB_STATUS_FAILED);
}

bool SpoolJob::IsCancelled()
{
	return TestStatus(PSRV_JOB_STATUS_CANCELLED);
}

bool SpoolJob::IsCancelling()
{
	return TestStatus(PSRV_JOB_STATUS_CANCELLING);
}

bool SpoolJob::IsProcessing()
{
	return TestStatus(PSRV_JOB_STATUS_PROCESSING);
}

bool SpoolJob::IsKnownState()
{
	BNode job(&entry);
	char status[32];
	status_t err = job.ReadAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, status, sizeof(status));
	if (err < 0)
		return true;	
	if (strncmp(PSRV_JOB_STATUS_WAITING, status, strlen(PSRV_JOB_STATUS_WAITING)) == 0)	return true;
	if (strncmp(PSRV_JOB_STATUS_COMPLETED, status, strlen(PSRV_JOB_STATUS_COMPLETED)) == 0)	return true;
	if (strncmp(PSRV_JOB_STATUS_FAILED, status, strlen(PSRV_JOB_STATUS_FAILED)) == 0)	return true;
	if (strncmp(PSRV_JOB_STATUS_CANCELLED, status, strlen(PSRV_JOB_STATUS_CANCELLED)) == 0)	return true;
	if (strncmp(PSRV_JOB_STATUS_CANCELLING, status, strlen(PSRV_JOB_STATUS_CANCELLING)) == 0)	return true;
	if (strncmp(PSRV_JOB_STATUS_PROCESSING, status, strlen(PSRV_JOB_STATUS_PROCESSING)) == 0)	return true;
	return false;
}

bool SpoolJob::operator == (SpoolJob& job)
{
	return (noderef == job.noderef);
}

//-----------------------------------------------------------------
// #pragma mark -

WatchPrinters::WatchPrinters(BServer *server)
	: 	NodeWatcher(server),
		fServer(server)
{
	// Start watching the printer directory
	BPath printers;
	find_directory(B_USER_PRINTERS_DIRECTORY, &printers);
	BNode dir(printers.Path());
	dir.GetNodeRef(&fPrintersRef);
	StartWatching(fPrintersRef, B_WATCH_DIRECTORY);
}

WatchPrinters::~WatchPrinters()
{
	StopWatching(fPrintersRef);
}

void WatchPrinters::EntryCreated(entry_ref& /*eref*/, node_ref& nref)
{ // A printer has been created
	P("\tprinter CREATED\n");
	status_t result = fServer->fSpoolWatcher->StartWatching(nref, B_WATCH_DIRECTORY);
	if (result != B_OK)
		P("StartWatching error: %s\n", strerror(result));
}

void WatchPrinters::EntryRemoved(node_ref& /*parent_eref*/, node_ref& nref)
{ // A printer has been removed
	P("\tprinter REMOVED\n");
	status_t result = fServer->fSpoolWatcher->StopWatching(nref);
	if (result != B_OK)
		P("StopWatching error: %s\n", strerror(result));
}

#if HANDLE_MOVED
void WatchPrinters::EntryMoved(entry_ref& src_eref, entry_ref& dst_eref, node_ref& nref)
{ // A printer has been moved
	node_ref dst_dir;
	dst_dir.device = dst_eref.device;
	dst_dir.node = dst_eref.directory;

	node_ref src_dir;
	src_dir.device = src_eref.device;
	src_dir.node = src_eref.directory;
 	
 	if (src_dir == dst_dir)
 	{
		P("\tprinter renamed - do nothing\n");
		return;
 	}
 	
	if (dst_dir == fPrintersRef)
	{ // A printer has been moved INTO the printers folder
		P("\tprinter MOVED into printer folder\n");
		EntryCreated(dst_eref, nref);
		// Activate the jobs that are in the new spool queue
		BEntry entry(&dst_eref);
		BNode printer(&entry);
		fServer->fPrinterManager.set_printer_free(&printer);	// Not sure of that.
		fServer->LookForOldJobs(&entry);
	}
	else
	{ // A printer has been removed FROM the printers folder
		P("\tprinter REMOVED from printer folder\n");
		node_ref parent_nref;
		parent_nref.device = dst_eref.device;
		parent_nref.node = dst_eref.directory;
		EntryRemoved(parent_nref, nref);
		// TODO: must remove the jobs that were in that spool queue
	}
}
#endif

//-----------------------------------------------------------------
// #pragma mark -

WatchSpool::WatchSpool(BServer *server)
	: 	NodeWatcher(server),
		fServer(server)
{
}

WatchSpool::~WatchSpool()
{
	// StopWatching() the printers that are still watching
	node_ref *nref_p;
	while ((nref_p = static_cast<node_ref *>(fNodesList.FirstItem())))
		StopWatching(*nref_p);
}

status_t WatchSpool::StartWatching(const node_ref& nref, uint32 flags)
{
	status_t result = NodeWatcher::StartWatching(nref, flags);
	if (result != B_OK)
		return result;
	fNodesList.AddItem(new node_ref(nref));
	return B_OK;
}

status_t WatchSpool::StopWatching(const node_ref& nref)
{
	status_t result = NodeWatcher::StopWatching(nref);
	if (result != B_OK)
		return result;

	const int32 count = fNodesList.CountItems();
	for (int i=0 ; i<count ; i++) {
		node_ref *nref_p = static_cast<node_ref *>(fNodesList.ItemAt(i));
		if (*nref_p == nref) {
			fNodesList.RemoveItem(i);
			delete nref_p;
			break;
		}
	}

	return B_OK;
}

void WatchSpool::EntryCreated(entry_ref& eref, node_ref& /*nref*/)
{ // A job has been created
	P("\tjob CREATED\n");
	// Start to watch attributes for this job
	P("SpoolThisJob()\n");
	fServer->SpoolThisJob(eref);
	fServer->UpdateDeskbarStatus();
}

void WatchSpool::EntryRemoved(node_ref& /*parent_eref*/, node_ref& nref)
{ // A job has been removed
	P("\tjob REMOVED\n");

	// Remove it from our list!
	SpoolJob *job = fServer->FindJobByNodeRef(&nref);
	if (job)
	{ // This was a job
		BDirectory curPrinter(job->printer);
		fServer->EndJob(job);
		// Try to launch another job
		if ((job = fServer->GetNextPendingJob(&curPrinter)))
			fServer->StartJob(job);
	}
	fServer->UpdateDeskbarStatus();
}

#if HANDLE_MOVED
void WatchSpool::EntryMoved(entry_ref& src_eref, entry_ref& dst_eref, node_ref& nref)
{ // A job has been moved
	P("\tjob MOVED\n");
//	SpoolJob *job = fServer->FindJobByNodeRef(&nref);
//	if (job)
//	{ // A job has been removed FROM a spool folder
//		P("\tjob REMOVED from a spool folder\n");
//		node_ref parent_nref;
//		parent_nref.device = dst_eref.device;
//		parent_nref.node = dst_eref.directory;
//		EntryRemoved(parent_nref, nref);
//	}
//	else
//	{ // a job has been added INTO a spool folder
//		P("\tjob MOVED into a spool folder\n");
//		EntryCreated(dst_eref, nref);
//	}
//	fServer->UpdateDeskbarStatus();
}
#endif

//-----------------------------------------------------------------
// #pragma mark -

WatchJobs::WatchJobs(BServer *server)
	: 	NodeWatcher(server),
		fServer(server)
{
}

WatchJobs::~WatchJobs()
{
	// Free remaining jobs, Clear the list
	const int32 count = fServer->fSpoolJobList.CountItems();
	for (int32 i=0 ; i<count ; i++)
	{
		SpoolJob *job = static_cast<SpoolJob *>(fServer->fSpoolJobList.RemoveItem(i));
		StopWatching(job->noderef);
		delete job;
	}
}

void WatchJobs::AttributeChanged(node_ref& nref)
{ // Attribute changed on a job
	P("\tattribute CHANGED on a job\n");

	SpoolJob *job = fServer->FindJobByNodeRef(&nref);
	if (job == NULL)
	{
		P("couldn't get job for ref!\n");
		return;
	}

	BDirectory curPrinter(job->printer);
	const bool printer_free = fServer->fPrinterManager.is_printer_free(&curPrinter);

	if (job->IsProcessing())
	{ // The job is still processing
	}
	else
	{
		if (job->IsKnownState() == false)
			return;	// Some weird state. Do nothing.
	
		bool completed	= job->IsCompleted();
		bool waiting	= job->IsPending();
		bool failed		= job->IsFailed();
		bool cancelled	= job->IsCancelled();
		bool cancelling	= job->IsCancelling();
		
		if (cancelling)
		{ // TODO: User asked to cancel this job. Call some new API in the driver to do that.
		}
		else if ((completed) || (cancelled))
		{ 
			// Remove the job from the list only if the printer is free (which mean the job is really finished)
			if (printer_free == false)
				return;
			// Remove the job from our list
			fServer->EndJob(job);
		}
		else if (waiting)
		{ // Set the internal flags that tell if this job is ready
			job->UpdateFlags();
		}
		else if (failed)
		{ 
			if (printer_free == false)
				return;
			// don't remove the job from the list here. Because the use can retry it later.
		}

		if ((job = fServer->GetNextPendingJob(&curPrinter)) != NULL)
			fServer->StartJob(job);
	}	
	fServer->UpdateDeskbarStatus();
}

//-----------------------------------------------------------------
// #pragma mark -

int	main()
{ // The life begins here
	BServer app;
	app.Run();
	return 0;
}

