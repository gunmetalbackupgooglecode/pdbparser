#ifndef SYMENGLIB_MSF_H
#define SYMENGLIB_MSF_H

#include <symenglib/symenglib_api.h>
#include <string>
#include <types.h>

namespace symenglib {

    class SYMENGLIB_API MSF {
    public:
        MSF() {}
        void Open(std::string file);
        void Open(char * file);
        void Close();
    };
}

//
// File starts with version-dependent headers. This structures apply only to the MSF 7.0
//

// 32-byte signature
#define MSF_SIGNATURE_700 "Microsoft C/C++ MSF 7.00\r\n\032DS\0\0"

// MSF File Header
#pragma pack(push, 1)
struct MSF_HDR {
    char szMagic[32]; // 0x00  Signature
    u32 dwPageSize; // 0x20  Number of bytes in the pages (i.e. 0x400)
    u32 dwFpmPage; // 0x24  FPM (free page map) page (i.e. 0x2)
    u32 dwPageCount; // 0x28  Page count (i.e. 0x1973)
    u32 dwRootSize; // 0x2c  Size of stream directory (in bytes; i.e. 0x6540)
    u32 dwReserved; // 0x30  Always zero.
    u32 dwRootPointers[0x49]; // 0x34  Array of pointers to root pointers stream. 
};
#pragma pack(pop)



#endif // SYMENGLIB_MSF_H
