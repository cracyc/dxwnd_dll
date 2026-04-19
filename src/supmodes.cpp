#include "dxwnd.h"

SupportedRes_Type SupportedSVGARes[15+1]= {
	{320,200},		// 8:5
	{320,240},		// QVGA
	{400,300},		// v2.04.77 addition
	{512,384},		// needed by "Outcast" loading screen
    {640,400},		// 8:5
	{640,480},		// VGA
	{720,480},		// 3:2
	{800,600},		// SVGA
	{1024,768},		// XGA
	{1152,864},		// XGA+ v2.04.77 addition
	{1280,800},		// WXGA
	{1280,960},		// v2.04.77 addition
	{1280,1024},	// 5:4
	{1400,1050},	// 4:3 v2.04.77 addition
	{1600,1200},	// UXGA, needed by "LEGO Star Wars" in high res mode
	{0,0}
};

SupportedRes_Type SupportedHDTVRes[12+1]= {
	{640,360},		// nHD
	{720,480},		// DVD
	{720,576},		// DV-PAL
    {854,480},      // FWVGA
	{960,540},		// qHD
    {1024,576},     // WSVGA
	{1280,720},		// HD
    {1366,768},     // FWXGA
	{1440,900},		// Swat 3 hack
	{1440,960},
	{1600,900},		// HD+
	{1920,1080},	// FHD
	{0,0}
};

// holds space for possible VGA + HDTV + Custom resolution + terminator
SupportedRes_Type SupportedRes[15+12+1+1]; 

int SupportedDepths[5]={8,16,24,32,0};
