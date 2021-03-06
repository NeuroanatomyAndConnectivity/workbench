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
#ifndef NIFTIFILE_H
#define NIFTIFILE_H

/*
 * Mac does not seem to have off64_t
 * Apparently, off_t is 64-bit on mac: http://sourceforge.net/mailarchive/forum.php?set=custom&viewmonth=&viewday=&forum_name=stlport-devel&style=nested&max_rows=75&submit=Change+View
 */
#ifdef CARET_OS_MACOSX
#define off64_t off_t
#endif // CARET_OS_MACOSX

#ifndef _LARGEFILE64_SOURCE 
#define _LARGEFILE64_SOURCE
#define _LFS64_LARGEFILE 1
#endif

#include <QtCore>
#include "iostream"
#include "NiftiException.h"
#include "NiftiHeaderIO.h"
#include "ByteSwapping.h"
#include "NiftiMatrix.h"
#include "VolumeBase.h"
#include "zlib.h"
#include "NiftiAbstractVolumeExtension.h"
#include <vector>

/** TODOS: there are a BUNCH...
High priority:
  * Support nifti extensions correctly
Medium priority:
  * Get slope intercept scaling working
  * Get the following working NIFTI_TYPE_RGB24, and all integer types

For later:
  * 1.  create class for dealing with and parsing extensions, borrow as much as possible from caret5
  * 2.  when an extension is added, update header to reflect new vox offset - no, regenerate vox offset at writing time
  * 3.  decide how to deal with in place read/write, for now one can edit the volume of a nifti in place, but
        how do we deal with extensions, which would shift the volume offset, simply not allow it for in-place
        edits?
  * 4.  Add more control for writing out different layouts, currently, it's set up to honor the original
  *     layout and byte order when doing in place edits, or write out to the default format, little endian (floats),
        but we may want to add more control
  * 5.  Currently only reads and write a frames at a time, and the underlying matrix only does this, it would be
        good to add functionality for reading a voxel at a time, a row at a time, a slice at a time, or the
        entire time series at a time, this would involve creating separate syntax for the following (the current
        matrix reader writer blurs the lines between concepts a bit):
        a.  setting current "area of interest on disk" (we don't just want to "read" it, since we may be dealing
        with a new volume, in case "read" as it currently is used.
        b.  loading "area of interest" into memory (when reading/updating), not necessary when writing.
        c.  reading/writing to data loaded in memory (specifying component, slice, frame, or time series), when the
            data requested is larger than, or outside the range of "area of interest", should we automatically
            deal with it, throw an exception, or is there another approach?
        d.  setting size of "area of interest", in the case that we automatically load data when requested
            read/write is out of bounds, this will set the size of chunk that is loaded (component, row, slice,
            or frame, in the case of the entire time series, going out of bounds "shouldn't" happnen)
  * 6.  Functions for getting at data should expose different data types than just float, internal storage
        should be more flexible than simply storing floats
  * 7.  For matrix reader, need to set up efficient ways of reading data that isn't packed tightly together,
        and generic ways of controlling how this happens, especially important for cifti.
  * 8.  Think a bit more about data organization.  i.e.  Should a nifti header "know" that it needs to be swapped
        (convenient for deriving matrix layouts from a nifti header), or does that only belong in the reader writer
        for the header?  Try to eliminate areas of unnecessary data duplication to eliminate potential bugs, and
        also add more code to keep things in sync (i.e. matrix offset in NiftiMatrix, and voxoffset in nifti header).
  * 9.  Finally, should headers, matrix readers, and extensions exist outside nifti file, or should all properties
        be expose through NiftiFile, so that they can be synchronized internally?
**/



/// Class for opening, reading, and writing generic Nifti1/2 Data, doesn't support extensions (yet)
namespace caret {
class NiftiFileTest;

class NiftiFile
{
    friend class NiftiFileTest;
public:
    /// Constructor
    NiftiFile(bool usingVolume = false);
    /// Constructor
    NiftiFile(const AString &fileName, bool usingVolume = false);
    /// Open the Nifti File
    virtual void openFile(const AString &fileName);
    /// Write the Nifti File
    virtual void writeFile(const AString &fileName, NIFTI_BYTE_ORDER byteOrder = NATIVE_BYTE_ORDER);
    /// Is the file Compressed?
    bool isCompressed();

    /// Header Functions
    /// set Nifti1Header
    virtual void setHeader(const Nifti1Header &header);
    /// get Nifti1Header
    void getHeader(Nifti1Header &header);
    /// set Nifti2Header
    virtual void setHeader(const Nifti2Header &header);
    /// get Nifti2Header
    void getHeader(Nifti2Header &header);
    /// get NiftiVersion
    int getNiftiVersion();

    /// volume file read/write Functions

    /// Read the entire nifti file into a volume file
    void readVolumeFile(VolumeBase &vol, const AString &filename);
    /// Write the entire Volume File to a nifti file
    void writeVolumeFile(VolumeBase &vol, const AString &filename);

    /// Gets a Nifti1Header from a previously defined volume file
    void getHeaderFromVolumeFile(VolumeBase &vol, Nifti1Header & header);
    /// Gets a Nifti2Header from a previously defined volume file
    void getHeaderFromVolumeFile(VolumeBase &vol, Nifti2Header & header);

    void getLayout(LayoutType &layout) {
        return matrix.getMatrixLayoutOnDisk(layout);
    }

    // For reading the entire Matrix
    /// Gets the Size in bytes of the entire volume matrix
    int64_t getMatrixSize() { return matrix.getMatrixSize();}
    /// Gets the number of individual elements (array size) of the entire volume matrix
    int64_t getMatrixLength() { return matrix.getMatrixLength(); }
    /// sets the entire volume file Matrix
    void setMatrix(float *matrixIn, const int64_t &matrixLengthIn) { matrix.setMatrix(matrixIn, matrixLengthIn); }
    /// gets the entire volume file Matrix
    void getMatrix(float *matrixOut) { matrix.getMatrix(matrixOut); }

    // For reading a frame at a time from previously loaded matrix
    /// Sets the current frame for writing, doesn't load any data from disk, can hand in a frame pointer for speed, writes out previous frame to disk if needed
    void setFrame(const float *matrixIn, const int64_t &matrixLengthIn, const int64_t &timeSlice = 0L, const int64_t &componentIndex=0L)
    { matrix.setFrame(matrixIn, matrixLengthIn, timeSlice, componentIndex); }
    /// Sets the current frame (for writing), doesn't load any data from disk, writes out previous frame to disk if needed
    void getFrame(float *frame, const int64_t &timeSlice = 0L,const int64_t &componentIndex=0L)
    { matrix.getFrame(frame, timeSlice, componentIndex); }

    /// Gets the Size in bytes of the current frame
    int64_t getFrameSize() { return matrix.getFrameSize(); }
    /// Gets the number of individual elements (array size) of the current frame
    int64_t getFrameLength() { return matrix.getFrameLength(); }
    /// returns the number of frames, or the 4th dimension, in the case of 3 dimensional volume files, returns 1
    int64_t getFrameCount() { return matrix.getFrameCount(); }

    /// Gets the Size in bytes of the nifti extension(s)
    //int64_t getExensionSize();
    /// Gets the extensions byte array
    //int8_t * getExtensionBytes();
    /// Sets the extensions byte array

    /// Destructor
    virtual ~NiftiFile();
protected:
    virtual void init();

    AString m_fileName;
    NiftiHeaderIO headerIO;
    NiftiMatrix matrix;
    std::vector<CaretPointer<NiftiAbstractVolumeExtension> > m_extensions;
    bool newFile;
    VolumeBase *m_vol;
    bool m_usingVolume;
};


}

#endif // NIFTIFILE_H

