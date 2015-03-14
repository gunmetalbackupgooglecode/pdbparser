/**
* misc.cpp  -  miscellaneous routines for pdb parser.
* All parts depend on it.
*
* This is the part of the PDB parser.
* (C) Great, 2010.
* xgreatx@gmail.com
*/

#include <windows.h>
#include <common/assert.h>
#include <stdio.h>

static ULONG nAllocations = 0;
static ULONG nBytes = 0;

// Our tiny allocator. Just LocalAlloc() wrapper.
void* halloc (size_t s)
{
	PVOID p = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, s);
	if (p != NULL)
	{
		++ nAllocations;
		nBytes += s;
	}
	return p;
}

void hfree (void* p)
{
	assert (p != NULL);

	ULONG Size = HeapSize (GetProcessHeap(), 0, p);
	HeapFree (GetProcessHeap(), 0, p);

	--nAllocations;
	nBytes -= Size;
}

void hCheckLeaks()
{
	if (nAllocations != 0)
	{
		printf("not all allocated memory has been freed! %d allocations pending, %d bytes (0x%lx) total\n",
			nAllocations, nBytes, nBytes);
	}
}

// String duplication routine.
char* xstrdup (const char *str)
{
	assert (str != NULL);

	size_t len = strlen(str);
	char *newS = (char*) halloc (len+1);
	if (newS != NULL)
	{
		strcpy (newS, str);
	}

	return newS;
}

//
// This routine searches for substring in a string and returns
//  a pointer AFTER this substring (skips its characters) on success,
// NULL otherwise.
// For example, xstrstrskip("bla123abc","123") returns "abc"
//
const char* xstrstrskip (const char *s1, const char *s2)
{
	const char *p = strstr (s1, s2);
	if (p != NULL)
	{
		p += strlen(s2);
	}
	return p;
}

//
// This routine replaces a single character in the string with an another character.
// The string MUST BE WRITABLE! No copy is being made.
// The return value is a pointer to source string (with replaced characters)
//
char* xstrreplacechar (char *str, char ch1, char ch2)
{
	for (char *sp = str; *sp != 0; sp++)
	{
		if (*sp == ch1) *sp = ch2;
	}
	return str;
}

//
// This routine calculates 32-bit hash value for the specified byte block [pb,pb+cb)
// and returns a modulo with ulMod.
//
ULONG HashPbCb (PBYTE pb, ULONG cb, ULONG ulMod)
{
	ULONG ulHash = 0;

	long cl = cb >> 2;
	PULONG pul = (PULONG)pb;
	PULONG pulMac = pul + cl;
	int dcul = cl & 7;

	switch (dcul)
	{
		do
		{
			dcul = 8;
			ulHash ^= pul[7];
	case 7: ulHash ^= pul[6];
	case 6: ulHash ^= pul[5];
	case 5: ulHash ^= pul[4];
	case 4: ulHash ^= pul[3];
	case 3: ulHash ^= pul[2];
	case 2: ulHash ^= pul[1];
	case 1: ulHash ^= pul[0];
	case 0: ;
		}
		while ((pul += dcul) < pulMac);
	}

	pb = (PBYTE) pul;

	if (cb & 2)
	{
		ulHash ^= *(PUSHORT)pb;
		pb = (PBYTE)((PUSHORT)pb + 1);
	}

	if (cb & 1)
	{
		ulHash ^= *(pb++);
	}

	const ULONG toLowerMask = 0x20202020;
	ulHash |= toLowerMask;
	ulHash ^= (ulHash >> 11);

	return (ulHash ^ (ulHash >> 16)) % ulMod;
}

