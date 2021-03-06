/*LICENSE_START*/
/*
 *  Copyright (C) 2014  Washington University School of Medicine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*LICENSE_END*/

#ifndef CIFTIMATRIX_H
#define CIFTIMATRIX_H

// For the sake of simplicity and straightforwardness,
// CiftiMatrix will just be it's own specialized animal for now.
// later it will be heavily based on a generalized version of Nifti Matrix,
// but I'd rather not hold up Cifti's release for it.  The interface will
// stay largely the same regardless of the underlying code.
#include "CaretMutex.h"
#include "CaretPointer.h"
#include "NiftiEnums.h"
#include "CiftiFileException.h"
#include "QFile"

using namespace std;
namespace caret {

enum CacheEnum {
    ON_DISK,
    IN_MEMORY
};

//WARNING: this is a dumb shallow copy object!

class CiftiMatrix
{
public:
    CiftiMatrix();
    CiftiMatrix(const AString &fileNameIn, const CacheEnum e=IN_MEMORY, const AString &cacheFile = NULL);
    ~CiftiMatrix();
    void init();
    void deleteCache();

    void setup(vector <int64_t> &dimensions, const int64_t &offsetIn=0, const CacheEnum &e=IN_MEMORY, const bool &needsSwapping=false) throw (CiftiFileException);
    void setMatrixFile(const AString &fileNameIn, const AString &cacheFile = NULL);
    void getMatrixFile(AString &fileNameOut);
    void getMatrixDimensions(vector <int64_t> &dimensions) const;
    //void setCaching(const CacheEnum &e);
    void getCaching(CacheEnum &e) const;
    void getMatrixOffset(int64_t &offsetOut);
    void getNeedsSwapping(bool &needsSwappingOut);
    bool getNeedsSwapping() const { return m_needsSwapping; }//this is always valid for whatever file it is reading from

    //Matrix IO
    void getRow(float * rowOut,const int64_t &rowIndex, const bool& tolerateShortRead = false) const throw (CiftiFileException);
    void setRow(float * rowIn, const int64_t &rowIndex) throw (CiftiFileException);
    void getColumn(float * columnOut, const int64_t &columnIndex) const throw (CiftiFileException);
    void setColumn(float * columnIn, const int64_t &columnIndex) throw (CiftiFileException);
    void getMatrix(float *matrixOut) throw (CiftiFileException);
    void setMatrix(float *matrixIn) throw (CiftiFileException);

    //Flush Cache
    //void flushCache() throw (CiftiFileException);
    //Write to a new file
    void writeToNewFile(const AString &fileNameIn, const int64_t &offsetIn) throw (CiftiFileException);
    //Gets the file handle to the cache so that CiftiFile and write the header and xml
    QFile *getCacheFile();
protected:
    void copyMatrix(QFile *output, QFile *input, const bool& needsSwapping);
    void updateCache();
    void copyHelper(const CiftiMatrix& rhs);
    CacheEnum m_caching;
    CaretArray<float> m_matrix;
    vector <int64_t> m_dimensions;//ideally just two, but can take the standard
    //1,1,1,1,M,N nifti matrix and convert it if needed.
    int64_t m_matrixOffset;//the beginning of the matrix in the file
    AString m_fileName;
    mutable CaretPointer<QFile> m_file;
    mutable CaretPointer<QFile> m_readFile;
    mutable CaretPointer<QFile> m_cacheFile;
    mutable CaretMutex m_fileMutex;//mutex to lock to avoid concurrent calls to file access functions
    bool m_needsSwapping;
    bool m_beenInitialized;
    AString m_cacheFileName;
    
    bool matrixChanged;
    std::vector<bool> rowsChanged;
};

}
#endif // CIFTIMATRIX_H
