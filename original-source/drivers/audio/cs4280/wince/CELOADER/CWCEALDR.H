//****************************************************************************
//
//    Description: Routines to encapsulate CSDevice routines
//
//    File:  cwcealdr.h
//
//    Copyright(c) 1998 Crystal Semiconductor Corp. All Rights Reserved.
//
//    Date:  02/26/98
//****************************************************************************

int main(int argc, char ** argv);

class CSDevice
{
public:

DWORD SetupMixer(CSDevice *pCSDevice);

    CSDevice();

    ~CSDevice();

    DOSHWInterface
        *m_pHw;    
    PciPnP 
        *m_pPCIDevice;
    Ulong
        m_ulPhysicalBA0,
        m_ulPhysicalBA1;
};

#define NO_ERROR    0
#define ERROR       1

//for SetupMixer()
// INPUT SOURCES
#define MIC         0x000
#define CD          0x101
#define VIDEO       0x202
#define AUX         0x303
#define LINE        0x404
#define STEREO_MIX  0x505
#define MONO_MIX    0x606
#define PHONE       0x707

BOOL AllocateAudioBuffers(CSDevice *pCSDevice );
DWORD  SetupMixer(CSDevice *pCSDevice);
