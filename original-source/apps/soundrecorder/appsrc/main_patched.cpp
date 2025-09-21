#include "../../../../beos_universal_system.h"
#include "RecorderApp.h"

int main( void )
{
        new RecorderApp;
        be_app->Run();
        delete be_app;
}
