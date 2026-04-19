#include "dxwnd.h"

BYTE GetCharsetFromANSICodepage(UINT ansicp)
{
    BYTE charset = ANSI_CHARSET;

    switch (ansicp){
        case 932: // Japanese
            charset = SHIFTJIS_CHARSET;
            break;
        case 936: // Simplified Chinese
            charset = GB2312_CHARSET;
            break;
        case 949: // Korean
            charset = HANGEUL_CHARSET;
            break;
        case 950: // Traditional Chinese
            charset = CHINESEBIG5_CHARSET;
            break;
        case 1250: // Eastern Europe
            charset = EASTEUROPE_CHARSET;
            break;
        case 1251: // Russian
            charset = RUSSIAN_CHARSET;
            break;
        case 1252: // Western European Languages
            charset = ANSI_CHARSET;
            break;
        case 1253: // Greek
            charset = GREEK_CHARSET;
            break;
        case 1254: // Turkish
            charset = TURKISH_CHARSET;
            break;
        case 1255: // Hebrew
            charset = HEBREW_CHARSET;
            break;
        case 1256: // Arabic
            charset = ARABIC_CHARSET;
            break;
        case 1257: // Baltic
            charset = BALTIC_CHARSET;
            break;
    }

    return charset;
}
