/*LICENSE_START*/
/*
 *  Copyright 1995-2011 Washington University School of Medicine
 *
 *  http://brainmap.wustl.edu
 *
 *  This file is part of CARET.
 *
 *  CARET is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  CARET is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CARET; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*LICENSE_END*/

#include "CaretSparseFile.h"
#include "ByteOrderEnum.h"
#include "ByteSwapping.h"
#include "CaretAssert.h"
#include "FileInformation.h"
#include <QByteArray>
#include <fstream>

using namespace caret;
using namespace std;

const char magic[] = "\0\0\0\0cst\0";

CaretSparseFile::CaretSparseFile()
{
    m_file = NULL;
}

CaretSparseFile::CaretSparseFile(const AString& fileName)
{
    m_file = NULL;
    readFile(fileName);
}

void CaretSparseFile::readFile(const AString& filename) throw (DataFileException)
{
    if (m_file != NULL)
    {
        fclose(m_file);
        m_file = NULL;
    }
    FileInformation fileInfo(filename);
    if (!fileInfo.exists()) throw DataFileException("file doesn't exist");
    m_file = fopen(filename.toLocal8Bit().constData(), "rb");
    if (m_file == NULL) throw DataFileException("error opening file");
    char buf[8];
    if (fread(buf, 1, 8, m_file) != 8) throw DataFileException("error reading from file");
    for (int i = 0; i < 8; ++i)
    {
        if (buf[i] != magic[i]) throw DataFileException("file has the wrong magic string");
    }
    if (fread(m_dims, sizeof(int64_t), 2, m_file) != 2) throw DataFileException("error reading from file");
    if (ByteOrderEnum::isSystemBigEndian())
    {
        ByteSwapping::swapBytes(m_dims, 2);
    }
    if (m_dims[0] < 1 || m_dims[1] < 1) throw DataFileException("both dimensions must be positive");
    m_indexArray.resize(m_dims[1] + 1);
    vector<int64_t> lengthArray(m_dims[1]);
    if (fread(lengthArray.data(), sizeof(int64_t), m_dims[1], m_file) != (size_t)m_dims[1]) throw DataFileException("error reading from file");
    if (ByteOrderEnum::isSystemBigEndian())
    {
        ByteSwapping::swapBytes(lengthArray.data(), m_dims[1]);
    }
    m_indexArray[0] = 0;
    for (int64_t i = 0; i < m_dims[1]; ++i)
    {
        if (lengthArray[i] > m_dims[0] || lengthArray[i] < 0) throw DataFileException("impossible value found in length array");
        m_indexArray[i + 1] = m_indexArray[i] + lengthArray[i];
    }
    m_valuesOffset = 8 + 2 * sizeof(int64_t) + m_dims[1] * sizeof(int64_t);
    int64_t xml_offset = m_valuesOffset + m_indexArray[m_dims[1]] * 2 * sizeof(int64_t);
    if (xml_offset >= fileInfo.size()) throw DataFileException("file is truncated");
    int64_t xml_length = fileInfo.size() - xml_offset;
    if (fseek(m_file, xml_offset, SEEK_SET) != 0) throw DataFileException("error seeking to XML");
    QByteArray myXMLBytes(xml_length, '\0');
    if (fread(myXMLBytes.data(), 1, xml_length, m_file) != (size_t)xml_length) throw DataFileException("error reading from file");
    m_xml.readXML(myXMLBytes);
}

CaretSparseFile::~CaretSparseFile()
{
    if (m_file != NULL) fclose(m_file);
}

void CaretSparseFile::getRow(int64_t* rowOut, const int64_t& index)
{
    CaretAssert(index >= 0 && index < m_dims[1]);
    int64_t start = m_indexArray[index], end = m_indexArray[index + 1];
    int64_t numToRead = (end - start) * 2;
    m_scratchArray.resize(numToRead);
    if (fseek(m_file, m_valuesOffset + start * sizeof(int64_t) * 2, SEEK_SET) != 0) throw DataFileException("failed to seek in file");
    if (fread(m_scratchArray.data(), sizeof(int64_t), numToRead, m_file) != (size_t)numToRead) throw DataFileException("error reading from file");
    if (ByteOrderEnum::isSystemBigEndian())
    {
        ByteSwapping::swapBytes(m_scratchArray.data(), numToRead);
    }
    int64_t curIndex = 0;
    for (int64_t i = 0; i < numToRead; i += 2)
    {
        int64_t index = m_scratchArray[i];
        if (index < 0 || index >= m_dims[0]) throw DataFileException("impossible index value found in file");
        while (curIndex < index)
        {
            rowOut[curIndex] = 0;
            ++curIndex;
        }
        rowOut[index] = m_scratchArray[i + 1];
    }
    while (curIndex < m_dims[0])
    {
        rowOut[curIndex] = 0;
        ++curIndex;
    }
}

void CaretSparseFile::getFibersRow(FiberFractions* rowOut, const int64_t& index)
{
    if (m_scratchRow.size() != (size_t)m_dims[0]) m_scratchRow.resize(m_dims[0]);
    getRow((int64_t*)m_scratchRow.data(), index);
    for (int64_t i = 0; i < m_dims[0]; ++i)
    {
        if (m_scratchRow[i] == 0)
        {
            rowOut[i].zero();
        } else {
             decodeFibers(m_scratchRow[i], rowOut[i]);
        }
    }
}

void CaretSparseFile::decodeFibers(const uint64_t& coded, FiberFractions& decoded)
{
    decoded.fiberFractions.resize(3);
    decoded.totalCount = coded>>32;
    uint32_t temp = coded & ((1LL<<32) - 1);
    const static uint32_t MASK = ((1<<10) - 1);
    decoded.distance = (temp & MASK) * 0.25f;//TODO: max distance!
    decoded.fiberFractions[1] = ((temp>>10) & MASK) / 1000.0f;
    decoded.fiberFractions[0] = ((temp>>20) & MASK) / 1000.0f;
    decoded.fiberFractions[2] = 1.0f - decoded.fiberFractions[0] - decoded.fiberFractions[1];
    if (decoded.fiberFractions[2] < -0.001f || (temp & (3<<30))) throw DataFileException("error decoding value '" + AString::number(coded) + "' from caret sparse trajectory file");
}

void FiberFractions::zero()
{
    totalCount = 0;
    fiberFractions.clear();
    distance = 0.0f;
}

CaretSparseFileWriter::CaretSparseFileWriter(const AString& fileName, const int64_t dimensions[2], const CiftiXML& xml)
{
    m_file = NULL;
    m_finished = false;
    if (dimensions[0] < 1 || dimensions[1] < 1) throw DataFileException("both dimensions must be positive");
    m_xml = xml;
    m_dims[0] = dimensions[0];//CiftiXML doesn't support 3 dimensions yet, so we do this
    m_dims[1] = dimensions[1];
    m_file = fopen(fileName.toLocal8Bit().constData(), "wb");
    if (m_file == NULL) throw DataFileException("error opening file for writing");
    if (fwrite(magic, 1, 8, m_file) != 8) throw DataFileException("error writing to file");
    int64_t tempdims[2] = { m_dims[0], m_dims[1] };
    if (ByteOrderEnum::isSystemBigEndian())
    {
        ByteSwapping::swapBytes(tempdims, 2);
    }
    if (fwrite(tempdims, sizeof(int64_t), 2, m_file) != 2) throw DataFileException("error writing to file");
    m_lengthArray.resize(m_dims[1], 0);//initialize the memory so that valgrind won't complain
    if (fwrite(m_lengthArray.data(), sizeof(uint64_t), m_dims[1], m_file) != (size_t)m_dims[1]) throw DataFileException("error writing to file");//write it to get the file to the correct length
    m_nextRowIndex = 0;
    m_valuesOffset = 8 + 2 * sizeof(int64_t) + m_dims[1] * sizeof(int64_t);
}

void CaretSparseFileWriter::writeRow(const int64_t* row, const int64_t& index)
{
    CaretAssert(index < m_dims[1]);
    CaretAssert(index >= m_nextRowIndex);
    while (m_nextRowIndex < index)
    {
        m_lengthArray[m_nextRowIndex] = 0;
        ++m_nextRowIndex;
    }
    m_scratchArray.clear();
    int64_t count = 0;
    for (int64_t i = 0; i < m_dims[0]; ++i)
    {
        if (row[i] != 0)
        {
            m_scratchArray.push_back(i);
            m_scratchArray.push_back(row[i]);
            ++count;
        }
    }
    m_lengthArray[index] = count;
    if (ByteOrderEnum::isSystemBigEndian())
    {
        ByteSwapping::swapBytes(m_scratchArray.data(), m_scratchArray.size());
    }
    if (fwrite(m_scratchArray.data(), sizeof(int64_t), m_scratchArray.size(), m_file) != m_scratchArray.size()) throw DataFileException("error writing to file");
    m_nextRowIndex = index + 1;
    if (m_nextRowIndex == m_dims[1]) finish();
}

void CaretSparseFileWriter::writeFibersRow(const FiberFractions* row, const int64_t& index)
{
    if (m_scratchRow.size() != (size_t)m_dims[0]) m_scratchRow.resize(m_dims[0]);
    for (int64_t i = 0; i < m_dims[0]; ++i)
    {
        if (row[i].totalCount == 0)
        {
            m_scratchRow[i] = 0;
        } else {
            encodeFibers(row[i], m_scratchRow[i]);
        }
    }
    writeRow((int64_t*)m_scratchRow.data(), index);
}

void CaretSparseFileWriter::finish()
{
    if (m_finished) return;
    m_finished = true;
    while (m_nextRowIndex < m_dims[1])
    {
        m_lengthArray[m_nextRowIndex] = 0;
        ++m_nextRowIndex;
    }
    QByteArray myXMLBytes;
    m_xml.writeXML(myXMLBytes);
    if (fwrite(myXMLBytes.constData(), 1, myXMLBytes.size(), m_file) != (size_t)myXMLBytes.size()) throw DataFileException("error writing to file");
    if (fseek(m_file, 8 + 2 * sizeof(int64_t), SEEK_SET) != 0) throw DataFileException("error seeking in file");
    if (ByteOrderEnum::isSystemBigEndian())
    {
        ByteSwapping::swapBytes(m_lengthArray.data(), m_lengthArray.size());
    }
    if (fwrite(m_lengthArray.data(), sizeof(uint64_t), m_lengthArray.size(), m_file) != m_lengthArray.size()) throw DataFileException("error writing to file");
    int ret = fclose(m_file);
    m_file = NULL;
    if (ret != 0) throw DataFileException("error closing file");
}

CaretSparseFileWriter::~CaretSparseFileWriter()
{
    finish();
}

void CaretSparseFileWriter::encodeFibers(const FiberFractions& orig, uint64_t& coded)
{
    coded = (((uint64_t)orig.totalCount)<<32) | (myclamp(orig.fiberFractions[0] * 1000.0f + 0.5f)<<20) |
            (myclamp(orig.fiberFractions[1] * 1000.0f + 0.5f)<<10) | (myclamp(orig.distance * 4.0f));//TODO: max distance!
}

uint32_t CaretSparseFileWriter::myclamp(const int& x)
{
    if (x >= 1000) return 1000;
    if (x <= 0) return 0;
    return x;
}