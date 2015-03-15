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

//#include <windows.h>
#include <tpi.h>
//#include "misc.h"

