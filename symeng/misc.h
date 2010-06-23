#pragma once

ULONG HashPbCb (PBYTE pb, ULONG cb, ULONG ulMod);

inline ULONG hashSz (PSZ sz)
{
	return HashPbCb ((PBYTE)sz, strlen(sz), (ULONG)-1);
}

void* halloc (size_t s);
void hfree (void* p);
char* xstrdup (const char *s);
const char* xstrstrskip (const char *s1, const char *s2);
char* xstrreplacechar (char *str, char ch1, char ch2);
void hCheckLeaks();
