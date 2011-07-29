/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2010  Thomas Fayoux

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#ifndef AVI_H
#define AVI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"
#include "SubtitleEncoder.h"

typedef struct aviIndex
{
    char id[4];
    unsigned int flags;
    unsigned int offset;
    unsigned int length;
}aviIndex;

class avi
{
public:
    avi(char* filenameIn, char* filenameOut, short* width, short* height, logger* log);
    ~avi();
    void Remux(Subtitle* txtSub, SubtitleEncoder* subEnc, int nbSubtitles);

private:
    // Header
    void readHeader();
    // Indexes management
    int findIdxPos();
    int SaveAllIndexes();
    unsigned int writeIndexes();
    
    // Xsub header
    unsigned int findEndOfXsubHeader(char* videoHeader, unsigned int videoHeaderSize);
    
    // Chunks
    unsigned int writeChunks(Subtitle* txtSub, SubtitleEncoder* subEnc);

    int readFile(FILE* file, char* buffer, unsigned int size);
    void writeFile(FILE* file, char* buffer, unsigned int size);
    void writeBuffer(char* out, char* in, unsigned int* start, unsigned int size);
    int compare(char* out, char* in, unsigned int size);
    unsigned int GetSize(char* dataIn);

    FILE* m_aviFileIn;
    FILE* m_aviFileOut;
    char* m_header;
    unsigned int m_headerSize;
    unsigned int m_dataOffset;
    unsigned int m_writtenDataSize;
    unsigned int m_firstOffset;

    unsigned int m_idxLength;
    unsigned int m_nbIndexes;
    aviIndex* m_indexesList;
    
    unsigned int m_nbSubtitles;
    aviIndex* m_xsubIndexesList;
    unsigned int m_nbSubWritten;
    
    fpos_t m_indexPos;
    fpos_t m_moviSizePosition;

    // Video informations
    unsigned int m_frameRate;
    unsigned int m_timeScale;
    unsigned int m_sampleSize;
    short m_width;
    short m_height;
    
    char m_subPid;
    
    logger* m_log;
};

#endif // AVI_H
