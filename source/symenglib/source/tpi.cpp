/**
* tpi.cpp  -  miscellaneous routines for Type Information (TPI) parsing.
*  With these routines you can construct your own c-style header file with
*  structures, unions, prototypes, typedefs, etc... :)
*
* This is the part of the PDB parser.
* (C) Great, 2010.
* xgreatx@gmail.com
*
* -= Read this please =-
*
* You can notice, that some parts of this file look like.. win_pdbx sources
* (from Sven B. Schreiber). His software was provided "as is" (and is being provided), blabla...,
*  with the respect to GPL.
* I did not just copied his code and pasted here. The majority of the code was reassembled by me,
*  some parts I have been written manually, and now it is a union of my code and his code.
* I will not begin to lie and I will not tell you, that this code is completely mine.
* But you should realize, that this code is not completely his too.
* Under the terms of GNU GPL this code is released and can be used by anyone (but CANNOT be sold).
*
* Without any warranties.
* With no responsibilities. All implied or express, including, but not limited to, the implied warranties of
* merchantability and fitness for a particular purpose are disclaimed.
* Not the author of the PDB parser, neither the author of orignial code (Sven B. Schreiber) shall be liable
* for any direct, indirect, incidental, special, exemplary, or consequential damages caused by this software
* or by impossibility to use it.
*
* - - -
* 
*  Для тех, блядь, кто понимает по-русски. Я несколько дней ебался с тем, чтобы кривой код этого ебучего урода,
*  наконец, заработал. И слава всем богам (кому вы там молитесь?), что это, вроде как, работает, и даже выводит
*  в си-стиле заголовок с описаниями структур, юниононов и прочей хуйни.
* Короче, от меня большой и пламенный привет Шрайберу (или как он читается правильно?) и громкое его посылание
* на стопицот букв русского матерного (да-да!). Свэн, миленький! Мало написать. Надо сделать это юзабельно =\\\
*   [c] Great, 3 марта 2010 года.
*/

#include <windows.h>
#include <stdio.h>
#include "msf.h"
#include "misc.h"


PBYTE TPIRecordValue (PBYTE pbData, PDWORD pdValue)
{
	WORD wValue;
	DWORD dValue = -1;
	PBYTE pbText = NULL;

	if (pbData != NULL)
	{
		if ((wValue = *(PWORD)pbData) < LF_NUMERIC)
		{
			dValue = wValue;
			pbText = pbData + sizeof(WORD);
		}
		else
		{
			switch (wValue)
			{
			case LF_CHAR:
				dValue = (LONG) (*(PCHAR)(pbData + sizeof(WORD)));
				pbText = pbData + sizeof(WORD) + sizeof(CHAR);
				break;

			case LF_SHORT:
				dValue = (LONG) (*(PSHORT)(pbData + sizeof(WORD)));
				pbText = pbData + sizeof(WORD) + sizeof(SHORT);
				break;

			case LF_USHORT:
				dValue = (LONG) (*(PUSHORT)(pbData + sizeof(WORD)));
				pbText = pbData + sizeof(WORD) + sizeof(USHORT);
				break;

			case LF_LONG:
				dValue = (LONG) (*(PLONG)(pbData + sizeof(WORD)));
				pbText = pbData + sizeof(WORD) + sizeof(LONG);
				break;

			case LF_ULONG:
				dValue = (LONG) (*(PULONG)(pbData + sizeof(WORD)));
				pbText = pbData + sizeof(WORD) + sizeof(ULONG);
				break;
			}
		}
	}

	if (pdValue != NULL) *pdValue = dValue;
	return pbText;
}

PBYTE WINAPI TPIMethodValue (CV_fldattr_t attr,
													PDWORD       pdData,
													PDWORD       pdValue)
{
	DWORD dValue = -1;
	PBYTE pbText = NULL;

	if (pdData != NULL)
	{
		switch (attr.mprop)
		{
		case CV_MTintro:
		case CV_MTpureintro:
			{
				dValue = *pdData;
				pbText = (PBYTE) (pdData + 1);
				break;
			}
		default:
			{
				pbText = (PBYTE) pdData;
				break;
			}
		}
	}
	if (pdValue != NULL) *pdValue = dValue;
	return pbText;
}

// -----------------------------------------------------------------

#define _T(S) (S)

// -----------------------------------------------------------------

VOID WINAPI TPIDisplayArray (PlfArray pla,
													DWORD    dBase,
													DWORD    dSize)
{
	DWORD dBytes;
	PBYTE pbName = TPIRecordValue (pla->data, &dBytes);

	printf (_T("// array   %08lX %08lX %08lX\n"),
		pla->elemtype, pla->idxtype, dBytes);
	return;
}

// -----------------------------------------------------------------

VOID WINAPI TPIDisplayBitfield (PlfBitfield plb,
														 DWORD       dBase,
														 DWORD       dSize)
{
	printf (_T("// bitfield (%08lX) %02lX : %02lX\n"),
		plb->type, plb->position, plb->length);
	return;
}

// -----------------------------------------------------------------

VOID WINAPI TPIDisplayClass (PlfClass plc,
													DWORD    dBase,
													DWORD    dSize)
{
	DWORD dBytes;
	PBYTE pbName = TPIRecordValue (plc->data, &dBytes);

	printf (_T("// class   %08lX %08lX %04hX %08lX %08lX [%hs]\n"),
		plc->field, dBytes, plc->count,
		plc->derived, plc->vshape, pbName);
	return;
}

// -----------------------------------------------------------------

VOID WINAPI TPIDisplayPointer (PlfPointer plp,
														DWORD      dBase,
														DWORD      dSize)
{
	printf (_T("// pointer %08lX\n"),
		plp->body.utype);
	return;
}

// -----------------------------------------------------------------

VOID WINAPI TPIDisplayProc (PHDR pHdr, PlfProc plp,
												 DWORD   dBase,
												 DWORD   dSize)
{
	printf (_T("// proc    %08lX %08lX %04lX %02lX\n"),
		plp->rvtype, plp->arglist, plp->parmcount,
		plp->calltype);

	return;
}

// -----------------------------------------------------------------

VOID WINAPI TPIDisplayMFunc (PlfMFunc plmf,
													DWORD    dBase,
													DWORD    dSize)
{
	printf (_T("// mfunc   %08lX %08lX %04lX %02lX %08lX %08lX %08lX\n"),
		plmf->rvtype, plmf->arglist, plmf->parmcount,
		plmf->calltype, plmf->classtype, plmf->thistype,
		plmf->thisadjust);
	return;
}

// -----------------------------------------------------------------

VOID WINAPI TPIDisplayArgList (PlfArgList plal,
														DWORD      dBase,
														DWORD      dSize)
{
	DWORD i;

	printf (_T("// arglist %08lX"),
		plal->count);

	for (i = 0; i < plal->count; i++)
	{
		printf (_T(" %08lX"), plal->arg [i]);
	}
	printf (_T("\n"));
	return;
}

// -----------------------------------------------------------------

VOID WINAPI TPIDisplayVTShape (PlfVTShape plvts,
														DWORD      dBase,
														DWORD      dSize)
{
	DWORD i;
	BYTE  b;

	printf (_T("// vtshape %08lX"),
		plvts->count);

	for (i = 0; i < plvts->count; i++)
	{
		b = plvts->desc [i/2];
		printf (_T(" %lX"), (i & 1 ? b & 0xF : b >> 4));
	}
	printf (_T("\n"));
	return;
}

// -----------------------------------------------------------------

char *TPIpFindTypeName (PHDR pHdr, ULONG typeIndex);

char *TPILookupTypeName (PHDR pHdr, ULONG typeIndex)
{
	switch (typeIndex)
	{
	case T_HRESULT: return "HRESULT";
	case T_32PHRESULT: return "PHRESULT";
	case T_VOID: return "VOID";
	case T_32PVOID:
	case T_PVOID: return "PVOID";

	case T_RCHAR:
	case T_CHAR: return "CHAR";
	case T_32PRCHAR:
	case T_32PCHAR:
	case T_PRCHAR:
	case T_PCHAR: return "PCHAR";

	case T_UCHAR: return "UCHAR";
	case T_32PUCHAR:
	case T_PUCHAR: return "PUCHAR";
	case T_WCHAR: return "WCHAR";
	case T_32PWCHAR:
	case T_PWCHAR: return "PWCHAR";
	case T_SHORT: return "SHORT";
	case T_32PSHORT:
	case T_PSHORT: return "PSHORT";
	case T_USHORT: return "USHORT";
	case T_32PUSHORT:
	case T_PUSHORT: return "PUSHORT";
	case T_LONG: return "LONG";
	case T_32PLONG:
	case T_PLONG: return "PLONG";
	case T_ULONG: return "ULONG";
	case T_32PULONG:
	case T_PULONG: return "PULONG";
	case T_REAL32: return "FLOAT";
	case T_32PREAL32:
	case T_PREAL32: return "PFLOAT";
	case T_REAL64: return "DOUBLE";
	case T_32PREAL64:
	case T_PREAL64: return "PDOUBLE";
	case T_QUAD: return "LONGLONG";
	case T_32PQUAD:
	case T_PQUAD: return "PLONGLONG";
	case T_UQUAD: return "ULONGLONG";
	case T_32PUQUAD:
	case T_PUQUAD: return "PULONGLONG";
	case T_INT4: return "INT";
	case T_32PINT4:
	case T_PINT4: return "PINT";
	case T_UINT4: return "UINT";
	case T_32PUINT4:
	case T_PUINT4: return "PUINT";
	default: {
		char *type = TPIpFindTypeName (pHdr, typeIndex);
		if (type == NULL)
			type = "(null.)";
		return type;
					 }
	}
}

#define SKIP(a,b) ((PUCHAR)(a) + (SIZE_T)(b))
#define _tsizeA(s) strlen((char*)(s))

VOID WINAPI TPIDisplayFieldList (PHDR pHdr, PlfFieldList plfl,
															DWORD        dBase,
															DWORD        dSize)
{
	PlfSubRecord plsr;
	DWORD        dValue, dOffset, i, n;
	PBYTE        pbName, pbNext;

	//printf (_T("\n"));

	for (i = 0; dSize - i >= lfFieldList_; i += n)
	{
		plsr = (PlfSubRecord) SKIP (&plfl->SubRecord, i);

		//printf (_T("        %04hX"), plsr->leaf);
		printf (_T("        "));

		switch (plsr->leaf)
		{
		case LF_ENUMERATE:
			{
				pbName = TPIRecordValue (plsr->Enumerate.value,
					&dValue);

				// 				printf (_T(" const  %08lX [%hs]\n"),
				// 					dValue, pbName);
				printf("  %hs = %d, // 0x%lx\n", pbName, dValue, dValue);

				n = ((DWORD_PTR) pbName - (DWORD_PTR) plsr) +
					_tsizeA (pbName) + 1;
				break;
			}
		case LF_MEMBER:
			{
				pbName = TPIRecordValue (plsr->Member.offset,
					&dOffset);

				char *type = TPILookupTypeName (pHdr, plsr->Member.index);

				// 				if (type)
				// 					printf (_T(" field  %08lX (%s) [%hs]\n"),
				// 						dOffset, type, pbName);
				// 				else
				// 					printf (_T(" field  %08lX (%08lX) [%hs]\n"),
				// 					dOffset, plsr->Member.index, pbName);

				if (type)
				{
					if (strchr (type, '%') != NULL)
					{
						char szDeclaration[256];
						wsprintf (szDeclaration, type, pbName);
						printf (_T(" /*+0x%08lX*/ /*type %08lX*/ %hs;\n"),
							dOffset, plsr->Member.index, szDeclaration);
					}
					else
					{
						printf (_T(" /*+0x%08lX*/ /*type %08lX*/ %s %hs;\n"),
							dOffset, plsr->Member.index, type, pbName);
					}
				}
				else
				{
					printf (_T(" /*+0x%08lX*/ TYPE_%08lX %hs;\n"),
						dOffset, plsr->Member.index, pbName);
				}

				n = ((DWORD_PTR) pbName - (DWORD_PTR) plsr) +
					_tsizeA (pbName) + 1;
				break;
			}
		case LF_BCLASS:
			{
				pbNext = TPIRecordValue (plsr->BClass.offset,
					&dOffset);

				printf (_T(" bclass %08lX (%08lX)\n"),
					dOffset, plsr->BClass.index);

				n = (DWORD_PTR) pbNext - (DWORD_PTR) plsr;
				break;
			}
		case LF_VFUNCTAB:
			{
				printf (_T(" vfunction table (%08lX)\n"),
					plsr->VFuncTab.type);

				n = lfVFuncTab_;
				break;
			}
		case LF_ONEMETHOD:
			{
				pbName = TPIMethodValue (plsr->OneMethod.attr,
					plsr->OneMethod.vbaseoff,
					&dValue);

				printf (_T(" single %08lX (%08lX) [%hs]\n"),
					dValue, plsr->OneMethod.index, pbName);

				n = ((DWORD_PTR) pbName - (DWORD_PTR) plsr) +
					_tsizeA (pbName) + 1;
				break;
			}
		case LF_METHOD:
			{
				printf (_T(" method %08lX (%08lX) [%hs]\n"),
					plsr->Method.count,
					plsr->Method.mList,
					plsr->Method.Name);

				n = ((DWORD_PTR) plsr->Method.Name - (DWORD_PTR) plsr) +
					_tsizeA (plsr->Method.Name) + 1;
				break;
			}
		case LF_NESTTYPE:
			{
				printf (_T(" nested typedef  (%08lX) [%hs]\n"),
					plsr->NestType.index, plsr->NestType.Name);

				n = ((DWORD_PTR) plsr->NestType.Name - (DWORD_PTR) plsr) +
					_tsizeA (plsr->NestType.Name) + 1;
				break;
			}
		default:
			{
				printf (_T(" member ###\n"));
				n = 0;
				break;
			}
		}
		if (!(n = (n + (sizeof(DWORD) - 1)) & (0 - sizeof(DWORD)))) break;
	}
	return;
}

// -----------------------------------------------------------------

PlfRecord TPILookupTypeRecord (PHDR pHdr, ULONG typeIndex, ULONG *pdBase, ULONG *pdSize);

VOID WINAPI TPIDisplayStructure (PHDR pHdr, PlfStructure pls,
															DWORD        dBase,
															DWORD        dSize)
{
	DWORD dBytes;
	PBYTE pbName = TPIRecordValue (pls->data, &dBytes);

	// 	printf (_T(" struct  %08lX %08lX %04hX %08lX %08lX [%hs]\n"),
	// 		pls->field, dBytes, pls->count,
	// 		pls->derived, pls->vshape, pbName);

	if (pls->field == 0)
	{
		printf (" struct %hs; // size 0x%lx fieldset 0x%lx count 0x%lx derived 0x%lx\n", pbName,
			dBytes, pls->field, pls->count, pls->derived);
	}
	else
	{
		printf(" struct %hs {\n", pbName);

		PlfRecord plr = TPILookupTypeRecord (pHdr, pls->field, &dBase, &dSize);
		if (plr != NULL)
		{
			TPIDisplayFieldList (pHdr, &plr->FieldList, dBase, dSize);
		}

		printf(" }; // size 0x%lx fieldcount %d\n", dBytes, pls->count);
	}

	return;
}

// -----------------------------------------------------------------

VOID WINAPI TPIDisplayUnion (PHDR pHdr, PlfUnion plu,
													DWORD    dBase,
													DWORD    dSize)
{
	DWORD dBytes;
	PBYTE pbName = TPIRecordValue (plu->data, &dBytes);

	// 	printf (_T("// union   %08lX %08lX %04hX [%hs]\n"),
	// 		plu->field, dBytes, plu->count, pbName);

	if (plu->field == 0)
	{
		printf("union %hs; // field %08lX dBytes %08lX count %04hX\n", pbName, plu->field, dBytes, plu->count);
	}
	else
	{
		printf("union %hs {\n", pbName);

		PlfRecord plr = TPILookupTypeRecord (pHdr, plu->field, &dBase, &dSize);
		if (plr != NULL)
		{
			TPIDisplayFieldList (pHdr, &plr->FieldList, dBase, dSize);
		}

		printf(" }; // size 0x%lx fieldcount %d\n", dBytes, plu->count);
	}

	return;
}

// -----------------------------------------------------------------

VOID WINAPI TPIDisplayEnum (PHDR pHdr, PlfEnum ple,
												 DWORD   dBase,
												 DWORD   dSize)
{
	// 	printf (_T("// enum    %08lX %08lX %04hX [%hs]\n"),
	// 		ple->field, ple->utype, ple->count, ple->Name);
	if (ple->field == 0)
	{
		printf("enum %hs; // field %08lX utype %08lX count %04hX\n", ple->Name, ple->field, ple->utype, ple->count);
	}
	else
	{
		printf("enum %hs {\n", ple->Name);

		PlfRecord plr = TPILookupTypeRecord (pHdr, ple->field, &dBase, &dSize);
		if (plr != NULL)
		{
			TPIDisplayFieldList (pHdr, &plr->FieldList, dBase, dSize);
		}

		printf(" }; // utype 0x%lx fieldcount %d\n", ple->utype, ple->count);

	}
	return;
}

// -----------------------------------------------------------------

VOID WINAPI TPIDisplayRecord (PlfRecord plr,
													 DWORD     dBase,
													 DWORD     dSize)
{
	printf (_T("// ???\n"));
	return;
}

char *staticprintf (char *format, ...)
{
	static char buffer[512];
	va_list va;

	va_start (va, format);
	_vsnprintf (buffer, sizeof(buffer), format, va);

	return buffer;
}

PlfRecord TPILookupTypeRecord (PHDR pHdr, ULONG typeIndex, ULONG *pdBase, ULONG *pdSize)
{
	ULONG dTypes = pHdr->tiMac - pHdr->tiMin;
	ULONG dBase = pHdr->cbHdr;
	PVOID pData = ((PUCHAR)pHdr + dBase);
	PlfRecord plr;

	for (ULONG i=0; i<dTypes; i++)
	{
		ULONG dSize = *(PWORD)pData;
		dBase += sizeof(WORD);
		plr = (PlfRecord)((PUCHAR)pHdr + dBase);

		if (pHdr->tiMin + i == typeIndex)
		{
			if (pdBase) *pdBase = dBase;
			if (pdSize) *pdSize = dSize;
			return plr;
		}

		dBase += dSize;
		pData = SKIP (pHdr, dBase);
	}

	return NULL;
}

char* szCallConvention (BYTE c)
{
	switch (c)
	{
	case CV_CALL_NEAR_C: return "__cdecl";
	case CV_CALL_NEAR_PASCAL: return "__pascal";
	case CV_CALL_NEAR_FAST: return "__fastcall";
	case CV_CALL_NEAR_STD: return "__stdcall";
	case CV_CALL_THISCALL: return "__thiscall";
	default: return staticprintf("CV_CALL_%04lX", c);
	}
}

char* TPIGetSymbolDeclaration (PHDR pHdr, char *szTypeName, char *szVarName)
{
	char *declataion;
	char *varName = xstrdup (szVarName);
	char *typeName = xstrdup (szTypeName);
	if (strchr (typeName, '%') != NULL)
	{
		declataion = staticprintf (typeName, varName);
	}
	else
	{
		declataion = staticprintf ("%s %s", typeName, varName);
	}
	hfree (varName);
	hfree (typeName);
	return declataion;
}

char* TPIpFindTypeName (PHDR pHdr, ULONG typeIndex)
{
	ULONG dTypes = pHdr->tiMac - pHdr->tiMin;
	ULONG dBase = pHdr->cbHdr;
	PVOID pData = ((PUCHAR)pHdr + dBase);
	PlfRecord plr;

	for (ULONG i=0; i<dTypes; i++)
	{
		ULONG dSize = *(PWORD)pData;
		dBase += sizeof(WORD);
		plr = (PlfRecord)((PUCHAR)pHdr + dBase);

		if (pHdr->tiMin + i == typeIndex)
		{
			switch (plr->leaf)
			{
			case LF_ARRAY:
				ULONG Size;
				TPIRecordValue (plr->Array.data, &Size);
				return staticprintf ("%s %%s [%d /*bytes*/]", TPILookupTypeName(pHdr, plr->Array.elemtype), Size);
			case LF_BITFIELD:
				return staticprintf ("%s %%s : %d /*pos_%d*/", TPILookupTypeName(pHdr, plr->Bitfield.type), plr->Bitfield.length, plr->Bitfield.position);
			case LF_CLASS:
				return (char*)TPIRecordValue (plr->Class.data, NULL);
			case LF_STRUCTURE:
				return (char*)TPIRecordValue (plr->Structure.data, NULL);
			case LF_UNION:
				return (char*)TPIRecordValue (plr->Union.data, NULL);
			case LF_ENUM:
				return (char*)plr->Enum.Name;
			case LF_POINTER:
				{
					char *type = xstrdup (TPILookupTypeName (pHdr, plr->Pointer.body.utype));
					char *ptr;

					if (strchr (type, '%') != NULL)
					{
						ptr = staticprintf(type, "*%s");
					}
					else
					{
						ptr = staticprintf("%s *", type);
					}

					hfree (type);
					return ptr;
				}
			case LF_PROCEDURE:
				// build procedure prototype
				char *proto, *rettype, *arguments, *convention;
				rettype = xstrdup (TPILookupTypeName (pHdr, plr->Proc.rvtype));
				arguments = xstrdup (TPILookupTypeName (pHdr, plr->Proc.arglist));
				convention = xstrdup (szCallConvention (plr->Proc.calltype));
				proto = staticprintf ("%s (%s %%s) /*args_%04lX*/ (%s)", rettype, convention, plr->Proc.arglist, arguments);
				hfree (convention);
				hfree (arguments);
				hfree (rettype);
				return proto;
			case LF_MFUNCTION:
				return NULL;
			case LF_ARGLIST:
				// build argument list.
				{
					char *arglist = (char*) halloc (512);
					if (arglist)
					{
						arglist[0] = 0;

						for (DWORD i=0; i<plr->ArgList.count; i++)
						{
							if (i > 0) strcat (arglist, ", ");

							char *type = TPILookupTypeName (pHdr, plr->ArgList.arg[i]);
							if (strchr (type, '%') != NULL)
							{
								wsprintf (arglist + strlen(arglist), type, "");
							}
							else
							{
								wsprintf (arglist + strlen(arglist), "%s", type);
							}
						}

						char *ret = staticprintf ("%s", arglist);
						hfree (arglist);
						return ret;
					}
				}
				return NULL;
			case LF_VTSHAPE:
				return NULL;
			case LF_FIELDLIST:
				return NULL;
			default:
				return NULL;
			}
		}

		dBase += dSize;
		pData = SKIP (pHdr, dBase);
	}

	return staticprintf("TYPE_%08lX", typeIndex);
}

VOID TPIDumpTypes (PHDR pHdr)
{
	printf("TPI dump\n");

	ULONG dTypes = pHdr->tiMac - pHdr->tiMin;
	ULONG dBase = pHdr->cbHdr;
	PVOID pData = ((PUCHAR)pHdr + dBase);
	PlfRecord plr;

	printf ("TPI version: %lu\nIndex range: %lX..%lX\nType count: %lu\n",
		pHdr->vers, pHdr->tiMin, pHdr->tiMac - 1, dTypes);

	for (ULONG i=0; i<dTypes; i++)
	{
		ULONG dSize = *(PWORD)pData;
		dBase += sizeof(WORD);
		plr = (PlfRecord)((PUCHAR)pHdr + dBase);

		printf("// %6lX: %04hX %08lX\n", pHdr->tiMin + i, plr->leaf, dBase - sizeof(WORD));

		switch (plr->leaf)
		{
		case LF_ARRAY:
			{
				TPIDisplayArray (&plr->Array,
					dBase, dSize);
				break;
			}
		case LF_BITFIELD:
			{
				TPIDisplayBitfield (&plr->Bitfield,
					dBase, dSize);
				break;
			}
		case LF_CLASS:
			{
				TPIDisplayClass (&plr->Class,
					dBase, dSize);
				break;
			}
		case LF_STRUCTURE:
			{
				TPIDisplayStructure (pHdr, &plr->Structure,
					dBase, dSize);
				break;
			}
		case LF_UNION:
			{
				TPIDisplayUnion (pHdr, &plr->Union,
					dBase, dSize);
				break;
			}
		case LF_ENUM:
			{
				TPIDisplayEnum (pHdr, &plr->Enum,
					dBase, dSize);
				break;
			}
		case LF_POINTER:
			{
				TPIDisplayPointer (&plr->Pointer,
					dBase, dSize);
				break;
			}
		case LF_PROCEDURE:
			{
// 				TPIDisplayProc (&plr->Proc,
// 					dBase, dSize);
				char *type = TPILookupTypeName (pHdr, pHdr->tiMin + i);
				char funcName[256];
				char typeName[256];
				sprintf (funcName, "FUNCTION_%04lX", pHdr->tiMin + i);
				sprintf(typeName, type, funcName);
				printf("%s;\n", typeName);
				break;
			}
		case LF_MFUNCTION:
			{
				TPIDisplayMFunc (&plr->MFunc,
					dBase, dSize);
				break;
			}
		case LF_ARGLIST:
			{
				TPIDisplayArgList (&plr->ArgList,
					dBase, dSize);
				break;
			}
		case LF_VTSHAPE:
			{
				TPIDisplayVTShape (&plr->VTShape,
					dBase, dSize);
				break;
			}
		case LF_FIELDLIST:
			{
// 					TPIDisplayFieldList (&plr->FieldList,
// 						dBase, dSize);
				break;
			}
		default:
			{
				TPIDisplayRecord (plr,
					dBase, dSize);
				break;
			}
		}

		dBase += dSize;
		pData = SKIP (pHdr, dBase);
	}
}

PHDR TPILoadTypeInfo (MSF *msf)
{
	ULONG TpiSize;
	PHDR pHdr = (PHDR) MsfReferenceStream (msf, PDB_STREAM_TPI, &TpiSize);

	if (TpiSize >= HDR_ &&
		TpiSize >= pHdr->cbHdr + pHdr->cbGprec &&
		pHdr->tiMac >= pHdr->tiMin)
	{
		return pHdr;
	}

	printf("TPI stream invalid\n");
	return NULL;
}

VOID TPIFreeTypeInfo (MSF* msf, PHDR hdr)
{
	MsfDereferenceStream (msf, PDB_STREAM_TPI);
}
