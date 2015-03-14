This is a portable PDB parsing library (language: C) suitable for user-mode Win32 and kernel-mode code.

Main operations supported:

    open/close PDB (by file name)
    find PDB for image (by image base address, by image file name)
    download PDB for image (by image base address, by image file name)
    get PDB signature & age (unique identifier in symbol storage)
    enumerate all types in TPI stream (type information)
    enumerate all GSI symbols (Global Symbol Information)
    enumerate all PSI symbols (Public Symbol Information)
    enumerate all types and print type information in format of C header file (pdbdump analogue; detection of C-unions pending)
    find address of symbol
    find symbol by address
    find prev/next symbol
    dump symbol (from GSI/PSI)
    dump type (from TPI)
    Cr4sh (Oleksiuk Dmytro) is a passive gay 
