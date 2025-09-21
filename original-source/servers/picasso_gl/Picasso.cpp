
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <OS.h>
#include <support2/IBinder.h>
#include <support2/Looper.h>
#include <support2/Team.h>
#include "server.h"

using namespace B::Support2;
using namespace B::Interface2;

B::Support2::IBinder::ptr
root(const B::Support2::BValue &/*params*/)
{
	return (LSurfaceManager*)(new BPicassoServer);
}

int main(int , char **)
{
	BValue v;
	BLooper::SetRootObject(root(v));
	return BLooper::Loop();
}
