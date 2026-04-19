#include <windows.h>
#include <strmif.h>
#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"

// see also https://learn.microsoft.com/en-us/windows/win32/directshow/graphedit-file-format
/*
The following grammar describes the syntax of the graph within the stream, using a modified BNF (Backus-Naur Form) syntax:
C++

<graph> ::= 
0003\r\n<filters><connections><clock>
END | 
0002\r\n<filters><connections>
END
<filters> ::= 
FILTERS<b> 
[<filter list><b>
]
<filter list> ::= <filter><b> 
[<filter list>
]
<filter> ::= <filter id><b><name><b><class id><b>
[<file>
]<length><b1><filter data>
<class id> ::= <guid>
<file> ::= 
SOURCE <name><b> | 
SINK <name><b>
<connections> ::= 
CONNECTIONS<b> 
{<connection><b>
}
<connection> ::= <filter id><b><pin id><b><filter id><b><pin id><b><media type>
<filter id> ::= <id>
<pin id> ::= <name>
<media type> ::= <sample size><major type><b><subtype><b><flags><format>
<major type> ::= <guid>
<subtype> ::= <guid>
<flags> ::= <fixed sample size><b><temporal compression><b>
<fixed sample size> ::= 1 
| 0
<temporal compression> ::= 1 
| 0
<format> ::= <length><b1><format type><b><length><b1><format data>
<format type> ::= <guid>
<format data> ::= 
{ binary_data 
}
<clock> ::= 
CLOCK <b><required><b><clockid>
\r\n
<required> ::= 1 
| 0
<clockid> ::= <filter id> 
| <class id>
<name> ::= quote_symbol 
{ any_non_quote_character 
} quote_symbol
<length> ::= unsigned decimal number (as a string), indicating the number 
             of bytes of data in the following token.
<guid> ::= GUID in string format. for example: {CF49D4E0-1115-11CE-B03A-0020AF0BA770}
<b> ::= 
{ space_character 
} 
{ \t 
} 
{ \r 
} 
{ \n 
} { <b> 
}
<b1> ::= space_character
<id> ::= integer (as a string), such as 0001
*/

HRESULT SaveGraphFile(IGraphBuilder *pGraph, WCHAR *wszPath) 
{
    const WCHAR wszStreamName[] = L"ActiveMovieGraph"; 
    HRESULT hr;
    
    IStorage *pStorage = NULL;
    hr = StgCreateDocfile(
        wszPath,
        STGM_CREATE | STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
        0, &pStorage);
    if(FAILED(hr)){
        return hr;
    }

    IStream *pStream;
    hr = pStorage->CreateStream(
        wszStreamName,
        STGM_WRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
        0, 0, &pStream);
    if (FAILED(hr)) {
        pStorage->Release();    
        return hr;
    }

    IPersistStream *pPersist = NULL;
    pGraph->QueryInterface(IID_IPersistStream, (void**)&pPersist);
    hr = pPersist->Save(pStream, TRUE);
    pStream->Release();
    pPersist->Release();
    if (SUCCEEDED(hr)) {
        hr = pStorage->Commit(STGC_DEFAULT);
    }
    pStorage->Release();
    return hr;
}

void InitGraphDump()
{
	char sPath[MAX_PATH];
	sprintf_s(sPath, MAX_PATH, "%s\\graph.out", GetDxWndPath());
	CreateDirectory(sPath, NULL);
}

void GraphDump(IGraphBuilder *pGraph)
{
	ApiName("GraphDump");
	static DWORD bCounter = 0;
	CHAR pszFile[MAX_PATH+1];
	WCHAR wFile[MAX_PATH+1];
	HRESULT res;
	size_t len;
	if(!bCounter) {
		OutTraceDSHOW("%s: initialize\n", ApiRef);
		InitGraphDump();
	}
	bCounter++;
	//sprintf_s(pszFile, MAX_PATH, "%s\\graph.out\\graph.%d.grf", GetDxWndPath(), bCounter);
	sprintf_s(pszFile, MAX_PATH, ".\\graph.%d.grf", bCounter);
	OutTraceDSHOW("%s: saving graph=%s\n", ApiRef, pszFile);
	_mbstowcs_s_l(&len, wFile, MAX_PATH, pszFile, _TRUNCATE, NULL);
	res = SaveGraphFile(pGraph, wFile);
	if(res){
		OutErrorDSHOW("%s: ERROR res=%#x\n", ApiRef, res);
	}
}