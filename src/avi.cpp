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
#include "avi.h"
#include "SubtitleEncoder.h"
#include <stdio.h>

avi::avi(char* filenameIn, char* filenameOut, short* width, short* height, logger* log)
{
    char temp[256];
    // open input avi file
    m_aviFileIn = fopen(filenameIn, "rb");
    if (m_aviFileIn == NULL)
    {
        sprintf(temp, "ERROR : %s cannot be opened\n", filenameIn);
	m_log->write(temp, 0);
    }
    // open output avi file
    m_aviFileOut = fopen(filenameOut, "wb");
    if (m_aviFileOut == NULL)
    {
        sprintf(temp, "ERROR : %s cannot be opened\n", filenameOut);
	m_log->write(temp, 0);
    }
    
    // log file
    m_log = log;
    m_header = NULL;
    m_headerSize = 0;
    m_writtenDataSize = 0;
    m_idxLength = 0;
    m_nbIndexes = 0;
    m_nbSubWritten = 0; 
    
    // Read current header and add 1 xsub stream header
    readHeader();

    *width = m_width;
    *height = m_height;
}

avi::~avi()
{
    if (m_aviFileIn)
    {
        fclose(m_aviFileIn);
    }
    if (m_aviFileOut)
    {
        fclose(m_aviFileOut);
    }
    if (m_header != NULL)
    {
        free(m_header);
        m_header = NULL;
    }
    if (m_indexesList != NULL)
    {
        free(m_indexesList);
        m_indexesList = NULL;
    }
}

void avi::Remux(Subtitle* txtSub, SubtitleEncoder* subEnc, int nbSubtitles)
{
    char dataIn[1024]		= "\0";
    int bytesWritten		= m_firstOffset;

    // allocation of the indexes for xsubs
    m_nbSubtitles = nbSubtitles;
    m_xsubIndexesList = (aviIndex*) malloc(m_nbSubtitles * sizeof (aviIndex));
    
    // Indexes management
    // Find the idx1 flag in the file (search from the end)
    findIdxPos();
    //printf("Idx found\n");
    
    // Read idx1 tag and size of all indexes
    // These datas will be written in the end of remux
    // idx1
    readFile(m_aviFileIn, dataIn, 4);
    // indexes size
    readFile(m_aviFileIn, dataIn, 4);
    m_idxLength = (unsigned char)(dataIn[0]) + ((unsigned char)(dataIn[1])<<8) + ((unsigned char)(dataIn[2])<<16) + ((unsigned char)(dataIn[3])<<24);
    // Save position of first index
    fgetpos(m_aviFileIn, &m_indexPos);
    // Save all indexes in a table
    SaveAllIndexes();
    
    // Read every chunk and write them to output file
    bytesWritten = writeChunks(txtSub, subEnc);
    // Write all indexes to output file
    bytesWritten += writeIndexes();

    m_log->write("INFO : all done successfully\n", 0);
}

void avi::readHeader()
{
    unsigned int i = 0;
    char dataIn[10240] = "\0";
    char dataSize[10] = "\0";
    char* headerSizePos = NULL;
    unsigned int value = 0;
    unsigned int aviheaderSize = 0;
    unsigned int computedAviHeaderSize = 0;
    unsigned int streamHeaderSize = 0;
    unsigned int orgStreamsNumber = 0;
    unsigned int moviSize = 0;
    fpos_t dataPos;
    bool streamHeaderRead = false;
    char videoHeader[10240] = "\0";
    unsigned int videoHeaderSize = 0;
    unsigned int xsubHeaderSize = 0;
    char temp[256];
    
    m_log->write("INFO : Read and write avi headers\n", 4);
    m_header = (char*) malloc (20480 * sizeof(char));

    // RIFF, Global Size, 'AVI ' or 'AVIX', LIST
    readFile(m_aviFileIn, dataIn, 16);
    writeBuffer(m_header, dataIn, &m_headerSize, 16);
    // Headers size
    readFile(m_aviFileIn, dataIn, 4);
    aviheaderSize = GetSize(dataIn); 
    headerSizePos = m_header + m_headerSize;
    writeBuffer(m_header, dataIn, &m_headerSize, 4);
    // hdrl, avih, Avi Main header size
    // Read avi header til flags.
    readFile(m_aviFileIn, dataIn, 24);
    writeBuffer(m_header, dataIn, &m_headerSize, 24);
    dataIn[0] = '\0';
    // Flag management
    readFile(m_aviFileIn, dataIn, 4);
    value = GetSize(dataIn);
    // Be sure that 
    value = 272;
    dataIn[0] = value & 0xFF;
    dataIn[1] = (value>>8) & 0xFF;
    dataIn[2] = (value>>16) & 0xFF;
    dataIn[3] = (value>>24) & 0xFF;  
    writeBuffer(m_header, dataIn, &m_headerSize, 4);
    // Read avi header til streams.
    readFile(m_aviFileIn, dataIn, 8);
    writeBuffer(m_header, dataIn, &m_headerSize, 8);
    dataIn[0] = '\0';
    // Number of streams
    readFile(m_aviFileIn, dataIn, 4);
    orgStreamsNumber = value = GetSize(dataIn);
    // Add one stream to manage xsub
    value += 1;
    dataIn[0] = value & 0xFF;
    dataIn[1] = (value>>8) & 0xFF;
    dataIn[2] = (value>>16) & 0xFF;
    dataIn[3] = (value>>24) & 0xFF;
    writeBuffer(m_header, dataIn, &m_headerSize, 4);
    m_subPid = orgStreamsNumber; // As stream ID starts with ID 0
    // Read the rest of header (56 - 28 = 28)
    readFile(m_aviFileIn, dataIn, 28);
    m_width = GetSize(dataIn+4);
    m_height = GetSize(dataIn+8);
    writeBuffer(m_header, dataIn, &m_headerSize, 28);
    dataIn[0] = '\0';

    // For each stream in the file
    for (i=0; i<orgStreamsNumber; i++)
    {
	// Read LIST
	readFile(m_aviFileIn, dataIn, 4);
        writeBuffer(m_header, dataIn, &m_headerSize, 4);
        // Stream header size
        readFile(m_aviFileIn, dataIn, 4);
	streamHeaderSize = GetSize(dataIn);
        // Read stream header size
        readFile(m_aviFileIn, dataIn, streamHeaderSize);        
        if (!streamHeaderRead && !strncmp(dataIn+12, "vids", 4))
        {
	    // Detect useful size of XSUB header
	    streamHeaderSize = findEndOfXsubHeader(dataIn, streamHeaderSize);
	    	    
            m_timeScale = GetSize(dataIn+32);
            m_frameRate = GetSize(dataIn+36);
            m_sampleSize = GetSize(dataIn+56);
            sprintf(temp, "INFO : Found video stream : timeScale = %u, frameRate = %u, sampleSize = %u\n", m_timeScale, m_frameRate, m_sampleSize);
	    m_log->write(temp, 1);
            streamHeaderRead = true;
	    // Copy video header for xsub header
	    memcpy(videoHeader, dataIn, streamHeaderSize);
	    videoHeaderSize = streamHeaderSize;
        }
        
        dataSize[0] = streamHeaderSize & 0xFF;
	dataSize[1] = (streamHeaderSize>>8) & 0xFF;
	dataSize[2] = (streamHeaderSize>>16) & 0xFF;
	dataSize[3] = (streamHeaderSize>>24) & 0xFF;

        writeBuffer(m_header, dataSize, &m_headerSize, 4);
        writeBuffer(m_header, dataIn, &m_headerSize, streamHeaderSize);
        dataIn[4] = '\0';
    }

    // Copy video buffer change stream Tag to DXSB
    xsubHeaderSize = 116;
    memcpy(dataIn, "LIST", 4);
    writeBuffer(m_header, dataIn, &m_headerSize, 4);
    dataIn[0] = xsubHeaderSize & 0xFF;
    dataIn[1] = (xsubHeaderSize>>8) & 0xFF;
    dataIn[2] = (xsubHeaderSize>>16) & 0xFF;
    dataIn[3] = (xsubHeaderSize>>24) & 0xFF;
    writeBuffer(m_header, dataIn, &m_headerSize, 4);
    // Put divx as format
    char* xsubHeader = videoHeader+16;
    xsubHeader[0] = 'd';
    xsubHeader[1] = 'i';
    xsubHeader[2] = 'v';
    xsubHeader[3] = 'x';
    value = 0;
    xsubHeader = videoHeader+32;
    xsubHeader[0] = value & 0xFF;
    xsubHeader[1] = (value>>8) & 0xFF;
    xsubHeader[2] = (value>>16) & 0xFF;
    xsubHeader[3] = (value>>24) & 0xFF;
    xsubHeader = videoHeader+36;
    xsubHeader[0] = value & 0xFF;
    xsubHeader[1] = (value>>8) & 0xFF;
    xsubHeader[2] = (value>>16) & 0xFF;
    xsubHeader[3] = (value>>24) & 0xFF;    
    // wdith
    value = m_width;
    xsubHeader = videoHeader+64;
    xsubHeader[0] = value & 0xFF;
    xsubHeader[1] = (value>>8) & 0xFF;
    // height
    value = m_height;
    xsubHeader = videoHeader+66;
    xsubHeader[0] = value & 0xFF;
    xsubHeader[1] = (value>>8) & 0xFF;
    // size
    value = 40; // default size of bitmap header
    xsubHeader = videoHeader+72;
    xsubHeader[0] = value & 0xFF;
    xsubHeader[1] = (value>>8) & 0xFF;
    xsubHeader[2] = (value>>16) & 0xFF;
    xsubHeader[3] = (value>>24) & 0xFF;    
    xsubHeader = videoHeader+76;
    xsubHeader[0] = value & 0xFF;
    xsubHeader[1] = (value>>8) & 0xFF;
    xsubHeader[2] = (value>>16) & 0xFF;
    xsubHeader[3] = (value>>24) & 0xFF;    
    // 4 bits depth
    value = 4;
    xsubHeader = videoHeader+90;
    xsubHeader[0] = value & 0xFF;
    xsubHeader[1] = (value>>8) & 0xFF;
    // Put DXSB as stream type
    xsubHeader = videoHeader+92;
    xsubHeader[0] = 'D';
    xsubHeader[1] = 'X';
    xsubHeader[2] = 'S';
    xsubHeader[3] = 'B';
    // image size = w * h / 2
    value = m_width * m_height / 2;
    xsubHeader = videoHeader+96;
    xsubHeader[0] = value & 0xFF;
    xsubHeader[1] = (value>>8) & 0xFF;
    xsubHeader[2] = (value>>16) & 0xFF;
    xsubHeader[3] = (value>>24) & 0xFF;
    writeBuffer(m_header, videoHeader, &m_headerSize, xsubHeaderSize);

    // Update global header size
    unsigned int start = 0;
    aviheaderSize = m_headerSize - 20; // 16 = header RIFF, AVI, LIST and size
    //aviheaderSize += xsubHeaderSize + 8;
    dataIn[0] = aviheaderSize & 0xFF;
    dataIn[1] = (aviheaderSize>>8) & 0xFF;
    dataIn[2] = (aviheaderSize>>16) & 0xFF;
    dataIn[3] = (aviheaderSize>>24) & 0xFF;
    writeBuffer(headerSizePos, dataIn, &start, 4);

    writeFile(m_aviFileOut, m_header, m_headerSize);

    // Reach the start of movi
    dataIn[1] = '\0';
    bool startReached = false;
    while (startReached == false)
    {
        readFile(m_aviFileIn, dataIn, 4);
        if (compare(dataIn, "LIST", 1))
        {
            readFile(m_aviFileIn, dataIn, 4);
	    streamHeaderSize = GetSize(dataIn);
            // FourCC movi
            readFile(m_aviFileIn, dataIn, 4);
            if (compare(dataIn, "movi", 4))
            {
                startReached = true;
                fseek(m_aviFileIn, -12, SEEK_CUR);
            }
            else
	    {
		fseek(m_aviFileIn, -12, SEEK_CUR);
		// LIST
		readFile(m_aviFileIn, dataIn, 4);
		writeFile(m_aviFileOut, dataIn, 4);
		// Size
		readFile(m_aviFileIn, dataIn, 4);
		streamHeaderSize = GetSize(dataIn);
		writeFile(m_aviFileOut, dataIn, 4);
		readFile(m_aviFileIn, dataIn, streamHeaderSize);
		writeFile(m_aviFileOut, dataIn, streamHeaderSize);
		m_headerSize += streamHeaderSize + 8;
	    }
        }
        if (startReached == false)
        {
            fseek(m_aviFileIn, -3, SEEK_CUR);
        }
    }

    // LIST
    readFile(m_aviFileIn, dataIn, 4);
    writeFile(m_aviFileOut, dataIn, 4);
    // Stream movi size
    readFile(m_aviFileIn, dataIn, 4);
    moviSize = GetSize(dataIn);
    fgetpos(m_aviFileOut, &m_moviSizePosition);
    writeFile(m_aviFileOut, dataIn, 4);
    // movi
    readFile(m_aviFileIn, dataIn, 4);
    writeFile(m_aviFileOut, dataIn, 4);

    fgetpos(m_aviFileIn, &dataPos);
    m_dataOffset = dataPos.__pos;
    m_firstOffset = m_headerSize + (3 * 4);
}

unsigned int avi::findEndOfXsubHeader(char* videoHeader, unsigned int videoHeaderSize)
{
    int seekVal		= -4;
    int size        = 0;
    char dataIn[1024]	= "\0";
    unsigned int xsubHeaderSize  = 0;
    bool foundJunk = false;
    // Detect end of useful information
    // Search for fourcc codes
    for (int i=0; i<videoHeaderSize; i++)
    {
        if (compare(videoHeader+i, "JUNK", 4) || compare(videoHeader+i, "vprp", 4))
        {
            xsubHeaderSize = i; // -1 to delete J
            foundJunk = true;
            break;
        }
    }
    if (foundJunk)
    {
        return xsubHeaderSize;
    }
    else
        return videoHeaderSize;
}

unsigned int avi::writeChunks(Subtitle* txtSub, SubtitleEncoder* subEnc)
{
    char dataIn[10]			= "\0";
    char* dataRead			= NULL;
    unsigned int nbVideoFrames		= 0;
    double currentTime			= 0;
    unsigned int currentSubIndex	= 0;
    TxtSubtitle currentTxtSub;
    XSub* currentXSub;
    bool endOfSubtitleReached		= false;
    unsigned int dataSize		= 0;
    fpos_t currentPos;
    
    // Get first subtitle
    if (!txtSub->GetSubtitleByIndex(currentSubIndex, &currentTxtSub))
    {
	endOfSubtitleReached = true;
    }
    else
    {
	subEnc->EncodeSubtitle(currentTxtSub);
	currentXSub = subEnc->getXsub();
    }

    m_log->write("INFO : Write data chunks\n", 4);

    // Compute offset from the beginning of the file
    int offset = m_dataOffset - m_indexesList[0].offset;

    for (unsigned int i=0; i<m_nbIndexes; i++)
    {
        // Go to current chunk
        fseek(m_aviFileIn, m_indexesList[i].offset + offset, SEEK_SET);
        // Read chunk ID
        readFile(m_aviFileIn, dataIn, 4);

        // To know when to add the xsubs chunks and interleave them we hav to compute the current time
        // To do that we increment time with each video frame knowing the frame rate.
        if (dataIn[2] == 'd')
        {
            nbVideoFrames++;
            currentTime = (double)nbVideoFrames * (double)m_timeScale / (double)m_frameRate;
            // Get it in milliseconds
            currentTime *= 1000;
            //printf("Time = %f\n",currentTime);
        }
        
        if (!endOfSubtitleReached && currentTime >= currentXSub->startTime)
	{
	    char dataTemp[10] = "\0";
	    // Write chunk ID
	    sprintf(dataTemp, "%02dsb", m_subPid);
	    writeFile(m_aviFileOut, dataTemp, 4); // Write to file 
	    memcpy(m_xsubIndexesList[currentSubIndex].id, dataTemp, 4*sizeof(char)); // Write to index
	    // Write chunk size
	    dataTemp[0] = currentXSub->dataSize & 0xFF;
	    dataTemp[1] = (currentXSub->dataSize>>8) & 0xFF;
	    dataTemp[2] = (currentXSub->dataSize>>16) & 0xFF;
	    dataTemp[3] = (currentXSub->dataSize>>24) & 0xFF;
	    writeFile(m_aviFileOut, dataTemp, 4);
	    // Write chunk data
	    writeFile(m_aviFileOut, (char*)currentXSub->data, currentXSub->dataSize);
	    // Add a 0 if size is even
	    if (currentXSub->dataSize % 2)
	    {
		char zero = 0;
		writeFile(m_aviFileOut, &zero, 1);
	    }
	    // Save xsub index	    
	    m_xsubIndexesList[currentSubIndex].flags = 0;
	    m_xsubIndexesList[currentSubIndex].length = currentXSub->dataSize + (currentXSub->dataSize%2);
	    m_xsubIndexesList[currentSubIndex].offset = m_writtenDataSize + m_firstOffset;	    
	    // Update global size for indexes
	    m_writtenDataSize += currentXSub->dataSize + (currentXSub->dataSize%2) + 8;
	    // Update movi size
	    dataSize += currentXSub->dataSize + (currentXSub->dataSize%2) + 8;
	    currentSubIndex++;
	    m_nbSubWritten++;
	    if (!txtSub->GetSubtitleByIndex(currentSubIndex, &currentTxtSub))
	    {
		endOfSubtitleReached = true;
	    }
	    else
	    {
		subEnc->EncodeSubtitle(currentTxtSub);
		currentXSub = subEnc->getXsub();
	    }
	}
	// Write chunk ID
	writeFile(m_aviFileOut, dataIn, 4);
        // Read chunk length
        readFile(m_aviFileIn, dataIn, 4);
        writeFile(m_aviFileOut, dataIn, 4);
        // Alloc dataRead buffer to the good size
        dataRead = (char*) malloc((m_indexesList[i].length + (m_indexesList[i].length%2)) * sizeof(char));
        // Copy chunk data from in to out
        readFile(m_aviFileIn, dataRead, m_indexesList[i].length + (m_indexesList[i].length%2));
        writeFile(m_aviFileOut, dataRead, m_indexesList[i].length + (m_indexesList[i].length%2));
        // Update written size and use it to compute offsets
        m_indexesList[i].offset = m_writtenDataSize + m_firstOffset;
        m_writtenDataSize += m_indexesList[i].length + (m_indexesList[i].length%2) + 8;
	// Update movi size
	dataSize += m_indexesList[i].length + (m_indexesList[i].length%2) + 8;
        // De-Alloc dataRead
        free(dataRead);
    }
    // Re write movi size in file
    fgetpos(m_aviFileOut, &currentPos);
    fsetpos(m_aviFileOut, &m_moviSizePosition);
    dataSize += 4; // movi flag
    dataIn[0] = dataSize & 0xFF;
    dataIn[1] = (dataSize>>8) & 0xFF;
    dataIn[2] = (dataSize>>16) & 0xFF;
    dataIn[3] = (dataSize>>24) & 0xFF;
    writeFile(m_aviFileOut, dataIn, 4);
    fsetpos(m_aviFileOut, &currentPos);
    
    return m_writtenDataSize;
}

int avi::SaveAllIndexes()
{
    unsigned int bytesRead = 0;
    char dataIn[10] = "\0";
    unsigned int maxNbIndexes = 5000;
    m_indexesList = (aviIndex*) malloc (maxNbIndexes * sizeof(aviIndex));
    aviIndex* tmpIndexesList = NULL;
    unsigned int oldMaxNbIndexes = maxNbIndexes;
    m_log->write("INFO : Get all indexes\n", 3);
    // Go to next index position
    fsetpos(m_aviFileIn, &m_indexPos);
    while (bytesRead < m_idxLength)
    {
        if (m_nbIndexes >= maxNbIndexes)
        {
            oldMaxNbIndexes = maxNbIndexes;
            tmpIndexesList = (aviIndex*) malloc (oldMaxNbIndexes * sizeof(aviIndex));
            memcpy(tmpIndexesList, m_indexesList, oldMaxNbIndexes * sizeof(aviIndex));
            free(m_indexesList);
            maxNbIndexes += 5000;
            m_indexesList = (aviIndex*) malloc (maxNbIndexes * sizeof(aviIndex));
            memcpy(m_indexesList, tmpIndexesList, oldMaxNbIndexes * sizeof(aviIndex));
            free(tmpIndexesList);
        }
        // chunck ID
        readFile(m_aviFileIn, m_indexesList[m_nbIndexes].id, 4);
        // first chunck flags
        readFile(m_aviFileIn, dataIn, 4);
        m_indexesList[m_nbIndexes].flags = GetSize(dataIn);
        // first chunck offset
        readFile(m_aviFileIn, dataIn, 4);
        m_indexesList[m_nbIndexes].offset = GetSize(dataIn);
        // first chunck length
        readFile(m_aviFileIn, dataIn, 4);
        m_indexesList[m_nbIndexes].length = GetSize(dataIn);
        m_nbIndexes++;
        bytesRead += 16;
        //printf("%s\t%d\t%d\t%d\t%d\n", m_indexesList->id, m_indexesList->flags, m_indexesList->length, m_indexesList->offset, m_nbIndexes);
    }
    return 1;
}

int avi::findIdxPos()
{
    int seekVal		= -4;
    bool foundIdx	= false;
    char dataIn[1024]	= "\0";
    // Go to end of file and search for "idx1" to the beginning
    // Works but need optimization
    while (!foundIdx)
    {
        if (fseek(m_aviFileIn, seekVal, SEEK_END))
        {
            //printf("idx1 not found in this file\n");
            return -1;
        }
        else
        {
            readFile(m_aviFileIn, dataIn, 4);
            if (compare(dataIn, "idx1", 4))
            {
                //printf("%s\t%d\n", dataIn, seekVal);
                fseek(m_aviFileIn, seekVal, SEEK_END);
                foundIdx = true;
            }
            if (!foundIdx)
            {
                seekVal--;
            }
        }
    }
    //printf("idx1 found in this file\n");
    return 0;
}

unsigned int avi::writeIndexes()
{
    char id[5] = "idx1";
    unsigned int size = (m_nbIndexes * 16) + (m_nbSubWritten * 16);
    unsigned int xsubIndex = 0;
    unsigned int bytesWritten = 0;

    m_log->write("INFO : Write indexes\n", 4);

    // Write index headers
    writeFile(m_aviFileOut, id, 4);
    writeFile(m_aviFileOut, (char*)&(size), 4);
    bytesWritten += 8;
    // Write indexes to output file
    for (unsigned int i=0; i<m_nbIndexes; i++)
    {
	while(i==0 && m_xsubIndexesList[xsubIndex].offset < m_indexesList[i].offset)
	{
	    writeFile(m_aviFileOut, (char*)&(m_xsubIndexesList[xsubIndex]), 16);
	    bytesWritten += 16;
	    xsubIndex++;	    
	}
	if( (i!=0) && (xsubIndex < m_nbSubtitles) && (m_xsubIndexesList[xsubIndex].offset > m_indexesList[i-1].offset) && (m_xsubIndexesList[xsubIndex].offset < m_indexesList[i].offset) )
	{
	    writeFile(m_aviFileOut, (char*)&(m_xsubIndexesList[xsubIndex]), 16);
	    bytesWritten += 16;
	    xsubIndex++;
	}
        writeFile(m_aviFileOut, (char*)&(m_indexesList[i]), 16);
	bytesWritten += 16;
    }
    return bytesWritten;
}

// Function to read from a file byte per byte to char*
int avi::readFile(FILE* file, char* buffer, unsigned int size)
{
    unsigned int i=0;
    for (i=0; i<size; i++)
    {
        buffer[i] = fgetc(file);
    }
    return 0;
}

// Function to write to a file byte per byte from char*
void avi::writeFile(FILE* file, char* buffer, unsigned int size)
{
    unsigned int i=0;
    for (i=0; i<size; i++)
    {
        fputc(buffer[i], file);
    }
}

// Function to concatene byte per byte to char*
void avi::writeBuffer(char* out, char* in, unsigned int* start, unsigned int size)
{
    unsigned int i = 0;
    for (i=0; i<size; i++)
    {
        out[*start + i] = in[i];
    }
    *start += size;
}

// Function to compare byte per byte to char*
int avi::compare(char* out, char* in, unsigned int size)
{
    unsigned int i = 0;
    int equal = 1;
    for (i=0; i<size; i++)
    {
        if (out[i] != in[i])
        {
            equal = 0;
            break;
        }
    }
    return equal;
}

unsigned int avi::GetSize(char* dataIn)
{
    unsigned int size = (unsigned char)(dataIn[3]);
    size = (size<<8) + (unsigned char)(dataIn[2]);
    size = (size<<8) + (unsigned char)(dataIn[1]);
    size = (size<<8) + (unsigned char)(dataIn[0]);
    return size;
}
