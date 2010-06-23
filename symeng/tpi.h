#pragma once

#include "msf.h"

VOID TPIDumpTypes (PHDR pHdr);

char *TPILookupTypeName (PHDR pHdr, ULONG typeIndex);
PlfRecord TPILookupTypeRecord (PHDR pHdr, ULONG typeIndex, ULONG *pdBase, ULONG *pdSize);
PHDR TPILoadTypeInfo (MSF *msf);
char* TPIGetSymbolDeclaration (PHDR pHdr, char *szTypeName, char *szVarName);
VOID TPIDumpTypes (PHDR pHdr);
VOID TPIFreeTypeInfo (MSF* msf, PHDR hdr);
