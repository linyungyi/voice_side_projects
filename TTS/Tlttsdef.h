#ifndef __TLTTSDEF_H__
#define __TLTTSDEF_H__
             


// define TTS type
#define TT_TTSLINK              0
#define TT_PSOLA                1
#define TT_CORPUS               2


// define file-sharing synthesis resuit
#define TS_FAIL                 -2
#define TS_NOT_START            -1
#define TS_DONE                 30000


//define speech format 
#define WAV_LINEAR16            0 
#define WAV_MULAW               1 
#define WAV_ALAW                2
#define RAW_LINEAR16            3
#define RAW_MULAW               4
#define RAW_ALAW                5
#define DLG_ADPCM               6
#define DLG_ADPCM8K             7


#define SENDDATA        0
#define GETDATA         1
#define FINISH          2
#define STOP            3
#define ERR             4
#define CHECKEXIST      5


#endif //__TLTTSDEF_H__