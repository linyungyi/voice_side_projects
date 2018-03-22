// cTTS.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "tlttsapi.h"

int main(int argc, char* argv[])
{
    TTS_Create("10.1.14.140",5019);

    int iHandle;
   
    if(!TTS_GetEngine("_Corpus",&iHandle))
    {
        printf("GetEngine err\n");
        return 0;
    }


    if(!TTS_Tts2File(iHandle,"中華電信研究所語音合成","1.wav",WAV_LINEAR16,true)){
        printf("text2file err\n");
    }

    
    if(!TTS_ReleaseEngine(iHandle))
    {
        printf("release Engine err\n");
    }

    TTS_Close();
	return 0;
}
