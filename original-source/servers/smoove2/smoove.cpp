
#include <support2/Container.h>
#include <support2/PositionIO.h>
#include <support2/Looper.h>
#include <support2/TextStream.h>
#include <storage2/File.h>
#include <xml2/XMLParser.h>
#include <xml2/XMLRootSplay.h>
#include <stdio.h>
#include <stdlib.h>

using namespace B::Storage2;
using namespace B::XML;

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("usage: %s <XML-file>\n",argv[0]);
		exit(3);
	}

	BContainer::ptr container(new BContainer());
	BXMLParser parser(new BXMLRootSplay(container));
/*
	// This (falsely) triggers one of Dianne's asserts
	BXMLIByteInputSource source(
		new BPositionIO(
			IStorage::ptr(
				new BFile(argv[1],B_READ_ONLY)
			)
		)
	);
*/
	IStorage::ptr file(new BFile(argv[1],B_READ_ONLY));
	BXMLIByteInputSource source(new BPositionIO(file));

	ParseXML(&parser,&source,0);

	BLooper::SetRootObject(container->IValueOutput::AsBinder());
	BLooper::Loop();
}
