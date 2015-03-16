#ifndef SYMENGLIB_ERR_H
#define SYMENGLIB_ERR_H

#include <types.h>
#include "symenglib_api.h"
#include <exception>
#include <string>

namespace symenglib {

    class SYMENGLIB_API Exception : std::exception {
    public:
        //http://stackoverflow.com/questions/8152720/correct-way-to-inherit-from-stdexception
        /** Constructor (C strings).
         *  @param message C-style string error message.
         *                 The string contents are copied upon construction.
         *                 Hence, responsibility for deleting the \c char* lies
         *                 with the caller. 
         */
        explicit Exception(const char* message) :
          msg_(message) {
        }

        /** Constructor (C++ STL strings).
         *  @param message The error message.
         */
        explicit Exception(const std::string& message) :
        msg_(message) {
        }

        /** Destructor.
         * Virtual to allow for subclassing.
         */
        virtual ~Exception() throw () {
        }

        /** Returns a pointer to the (constant) error description.
         *  @return A pointer to a \c const \c char*. The underlying memory
         *          is in posession of the \c Exception object. Callers \a must
         *          not attempt to free the memory.
         */
        virtual const char* what() const throw () {
            return msg_.c_str();
        }

    protected:
        /** Error message.
         */
        std::string msg_;

    };

    //That file not a PDB file

    class SYMENGLIB_API SYMENGLIB_BAD_PDB_FILE : Exception {
        public:
            SYMENGLIB_BAD_PDB_FILE() : Exception("SYMENGLIB_BAD_PDB_FILE") {}
    };
}





#endif // SYMENGLIB_ERR_H