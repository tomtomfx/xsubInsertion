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

#include "SubtitleEncoder.h"
#include "stdio.h"

const unsigned char log2_tab[256]={
    0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

SubtitleEncoder::SubtitleEncoder(int* err)
{
    // Init free type library
    *err = FT_Init_FreeType(&m_library);
    if (*err != 0)
    {
        printf("ERROR : freetype library cannot be loaded\n");
    }
    else
    {
        m_font = NULL;
    }
    m_width = 0;
    m_height = 0;

    m_penX = 0;
    m_penY = 0;

    m_subtitleBmp = NULL;
    m_backGroundSize = 0;

    m_nbSubtitles = 0;
    m_maxNbLines = 0;

    m_intBuffer = 0;
}

SubtitleEncoder::~SubtitleEncoder()
{
    // Free all xsub buffers
    if (m_xSub != NULL)
    {
        free(m_xSub->data);
	free(m_xSub);
    }
    // Deinit free type library
    if (m_font != NULL)
    {
        delete(m_font);
    }
    if (m_library != NULL)
    {
        FT_Done_FreeType(m_library);
    }
    free(m_subtitleBmp);
    free(m_tempBuffer);
    free(m_tempXsub);
}

int SubtitleEncoder::Initialize(char* fontName, int fontSize, unsigned int nbSubtitles, unsigned int maxNbLines, short width, short height, logger* log)
{
    m_nbSubtitles = nbSubtitles;
    m_maxNbLines = maxNbLines;
    m_fontSize = fontSize;
    m_width = width / 2 * 2;
    m_height = height;
    m_heightBox = (m_fontSize * m_maxNbLines + 10) / 2 * 2;

    // log file
    m_log = log;

    // Create the background, the buffer we will fill with glyphs
    // Size = width * height
    m_backGroundSize = m_width * m_height;
    m_subtitleBmp = (unsigned char*) malloc(m_backGroundSize * sizeof(unsigned char));
    
    // Initialize temporary buffer for xsubs
    // *2 to manage header and data
    // Could be optimized
    m_tempBuffer = (unsigned char*) malloc(m_backGroundSize * 2 * sizeof(unsigned char));
    
    // Buffer for xsub data
    m_tempXsub = (unsigned char*)malloc(m_backGroundSize * sizeof(unsigned char));

    // For each sub, create a buffer to keep the bitmap
    m_xSub = (XSub*)malloc(sizeof(XSub));
    m_xSub->data = (unsigned char*)malloc(m_backGroundSize * sizeof(unsigned char));

    // Create a new font info with the font file
    m_font = new Font(m_library, fontName);
    if (m_font->load())
    {
        m_font->setSize (m_fontSize, m_fontSize);
        m_font->setTransform(1.0, 1.0, 0.0);
        return 0;
    }
    return 1;
}

int SubtitleEncoder::EncodeSubtitle(TxtSubtitle subTxt)
{
    unsigned int i=0, j=0, k=0;
    int line = 0;
    int glyphWidth = 0;
    int glyphHeight = 0;
    BitmapGlyph bitmapGlyph;
    unsigned int subStrLen = 0;
    unsigned char subStr[2048] = "";
    unsigned int start = 0;
    int bearingY = 0;

    // Get text subtitle infos
    // Global length
    unsigned int strLen = strlen(subTxt.subTxt);
    // Number of lines
    unsigned int nbLines = subTxt.nbLines;
    
    // Init background with value 0;
    memset(m_subtitleBmp, 0, m_backGroundSize);

    while (j < nbLines)
    {
        k = 0;
        // Get line.
        for (i=start; i<strLen; i++)
        {
            if (subTxt.subTxt[i] != '\n' && subTxt.subTxt[i] != '\0')
            {
                subStr[k] = (unsigned char)subTxt.subTxt[i];
                k++;
            }
            else
            {
                break;
            }
        }
        subStr[k] = '\0';
        start = i+1;

        // Get line length
        subStrLen = strlen((const char*)subStr);

        // Initialize text position
        m_penX = (m_width / 2) - (subStrLen * m_fontSize / 4);
        if (m_penX < 0) m_penX = 0;
        m_penY = m_fontSize * (j + 1); // +1 is due to bearing system

        for (i=0; i<subStrLen; i++)
        {
	    if (subStr[i] == '\n' || subStr[i] == '\r')
	    {
		continue;
	    }
            if (m_font->getGlyphInfo(subStr[i], &bitmapGlyph))
            {
                glyphWidth = bitmapGlyph.getPitch();
                glyphHeight = bitmapGlyph.getHeight();
                bearingY = bitmapGlyph.getBearingY();
                for (line=0; line<glyphHeight; line++)
                {
                    if (bitmapGlyph.getPixmap() != NULL)
                    {
                        memcpy(m_subtitleBmp + ((line + m_penY - bearingY) * m_width) + m_penX, bitmapGlyph.getPixmap() + (line * glyphWidth), glyphWidth);
                    }
                }
                m_penX += bitmapGlyph.getAdvanceX();
            }
        }
        j++;
    }
    // Create xsub with the computed bitmap
    createXSub(subTxt);

    return 1;
}

void SubtitleEncoder::createXSub(TxtSubtitle subTxt)
{
    int headerSize = 0;
    unsigned char* dataLength = NULL;
    unsigned char* hdr = m_tempBuffer + 27;
    short tempSize = 0;
    int color = 0;
    unsigned short length = 0;
    unsigned int xsubGlobalSize = 0;
    unsigned short fieldOffset = 0;

    // init buffer
    m_intBuffer = 0;

    // create header
    // Get time code from text subtitle
    snprintf((char*)m_tempBuffer, 28, "[%02d:%02d:%02d.%03d-%02d:%02d:%02d.%03d]",
             subTxt.startTime.hours, subTxt.startTime.minutes, subTxt.startTime.secondes, subTxt.startTime.milliSeconds,
             subTxt.stopTime.hours, subTxt.stopTime.minutes, subTxt.stopTime.secondes, subTxt.stopTime.milliSeconds);

    memcpy(hdr, &m_width, sizeof(short));
    hdr += 2;
    memcpy(hdr, &m_heightBox, sizeof(short));
    hdr += 2;
    tempSize = 0;
    memcpy(hdr, &tempSize, sizeof(short));
    hdr += 2;
    memcpy(hdr, &m_height, sizeof(short));
    hdr += 2;
    memcpy(hdr, &m_width, sizeof(short));
    hdr += 2;
    memcpy(hdr, &m_height, sizeof(short));
    hdr += 2;

    dataLength = hdr; // Will store length of first field here later.
    hdr += 2;

    // Palette = 3 * 8 bits (red, green, blue) * 4 colors
    // First black
    color = 0x000000;
    memcpy(hdr, &color, 3 * sizeof(char));
    hdr += 3;
    // Second is white
    color = 0xFFFFFF;
    memcpy(hdr, &color, 3 * sizeof(char));
    hdr += 3;
    // Next two are black for now
    color = 0x000000;
    for (int i=2; i<4; i++)
    {
        memcpy(hdr, &color, 3 * sizeof(char));
        hdr += 3;
    }
    
    headerSize = hdr - m_tempBuffer;
    // Convert sub bitmap to xsub color map
    convertToXsubColorMap();
    // Create a color map to debug (Some patern easy to recognise)
    //debugColorMap();
    // Print image for debug
    //showImage(0);
    // RLE xsub encode
    length = (unsigned short)encodeXsub(headerSize, m_width*2, (m_heightBox/2), m_subtitleBmp, 0);
    fieldOffset = length / 8;
    memcpy(dataLength, &fieldOffset, sizeof(short)); // Write size of first field
    length = (unsigned short)encodeXsub(headerSize, m_width*2, (m_heightBox/2), m_subtitleBmp+m_width, length);
    length = length / 8;
    xsubGlobalSize = length + headerSize;
 
    // File xsub structure
    m_xSub->startTime = (subTxt.startTime.hours * 3600000) + (subTxt.startTime.minutes * 60000) + (subTxt.startTime.secondes * 1000) + subTxt.startTime.milliSeconds;
    m_xSub->stopTime = (subTxt.stopTime.hours * 3600000) + (subTxt.stopTime.minutes * 60000) + (subTxt.stopTime.secondes * 1000) + subTxt.stopTime.milliSeconds;
    m_xSub->dataSize = xsubGlobalSize;
    memcpy(m_tempBuffer+headerSize, m_tempXsub, length);
    memcpy(m_xSub->data, m_tempBuffer, xsubGlobalSize);    
}

unsigned int SubtitleEncoder::encodeXsub(unsigned int headerSize, unsigned int lineSize, unsigned int height, unsigned char* bitmap, unsigned int nbBitsWritten)
{
    unsigned int line = 0;
    int initRow = 0, currentRow = 0;
    char color = 0; // Index of color in 4 available
    unsigned int length = 0;
    unsigned char* buffer = m_tempXsub;
    unsigned char* pBitmap = bitmap;

    // Interlace so first field
    for (line=0; line<height; line++)
    {
        initRow = 0;
        while (initRow < m_width)
        {
            currentRow = initRow;
            color = pBitmap[currentRow];
            // Compute the number of pixels which have the same color
            while (pBitmap[currentRow] == color && currentRow < m_width)
            {
                currentRow++;
            }
            length = currentRow - initRow;
            if (length > 255 && currentRow != m_width)
            {
                length = 255;
            }
            // Write data to buffer
            nbBitsWritten = writeXsubInfo(buffer, length, color, nbBitsWritten);   
            // Update position
            initRow += length;
        }
        // Each row is aligned to 8 bits
	nbBitsWritten += alignRow(buffer, nbBitsWritten);
        pBitmap += lineSize;
    }    
    return (nbBitsWritten);
}

void SubtitleEncoder::convertToXsubColorMap()
{
    int  i,j;
    for ( i = 0; i < m_heightBox; i++ )
    {
        for ( j = 0; j < m_width; j++ )
        {
            if (m_subtitleBmp[(i * m_width) + j] == 0)
            {
                continue;
            }
            else if (m_subtitleBmp[(i * m_width) + j] < 128)
            {
		m_subtitleBmp[(i * m_width) + j] = (unsigned char)2;
	    }
            else
            {
		m_subtitleBmp[(i * m_width) + j] = (unsigned char)1;
            }
        }
    }
}

void SubtitleEncoder::debugColorMap()
{
    int  i,j;
    for ( i = 0; i < m_heightBox; i++ )
    {
        for ( j = 0; j < m_width; j++ )
        {
	    //if (i % 2)
	    {
		    m_subtitleBmp[(i * m_width) + j] = 1;
	    }
	    /*else
	    {
		    m_subtitleBmp[(i * m_width) + j] = 2;
	    }*/
        }
    }
}

unsigned int SubtitleEncoder::writeBits(unsigned char* buffer, int length, int value, int nbBitsWritten)
{
    int size = 32;
    unsigned int index = nbBitsWritten / size;
    int bitsInCurrentInt = nbBitsWritten % size;
    int bitsLeft = size - bitsInCurrentInt;
    unsigned char* pBuffer = NULL;

    if ( bitsLeft > length)
    {
	m_intBuffer = (m_intBuffer<<length) | value;
	// Get buffer
	pBuffer = (buffer+(index * 4));
	*(unsigned int*)pBuffer = swap32(m_intBuffer);
    }
    else
    {
	m_intBuffer = (m_intBuffer<<bitsLeft);
	m_intBuffer = m_intBuffer | (value >> (length - bitsLeft));
	
	// Get buffer
	pBuffer = (buffer+(index * 4));
	
	*(unsigned int*)pBuffer = swap32(m_intBuffer); // Write current int to buffer with swap 32 bits
	m_intBuffer = value;
    }
    return length;
}

unsigned int SubtitleEncoder::swap32(unsigned int x)
{
    x= ((x<<8)&0xFF00FF00) | ((x>>8)&0x00FF00FF);
    x= (x>>16) | (x<<16);
    return x;
}

unsigned int SubtitleEncoder::writeXsubInfo(unsigned char* buffer, int length, int color, int nbBitsWritten)
{
    int bitsToWrite = 0;
    if (length <= 255)
    {
        bitsToWrite = 2 + ((log2_tab[length] >> 1) << 2);
    }
    else
    {
        length = 0;
        bitsToWrite = 14;
    }
    nbBitsWritten += writeBits(buffer, bitsToWrite, length, nbBitsWritten);
    nbBitsWritten += writeBits(buffer, 2, color, nbBitsWritten);
    return nbBitsWritten;
}

unsigned int SubtitleEncoder::alignRow(unsigned char* buffer, int nbBitsWritten)
{
    int bitsInCurrentChar = nbBitsWritten % 8;
    unsigned int bitsLeft = 8 - bitsInCurrentChar;
    
    if (bitsInCurrentChar == 0)
    {
	return 0;
    }
    nbBitsWritten += writeBits(buffer, bitsLeft, 0, nbBitsWritten);
    return bitsLeft;
}

XSub* SubtitleEncoder::getXsub()
{
    return m_xSub;
}

// Debug display function
void SubtitleEncoder::showImage(bool toFile)
{
    int  i,j;
    FILE* out;
    if (toFile) out = fopen("image.txt", "w");
    for ( i = 0; i < m_heightBox*2; i++ )
    {
        for ( j = 0; j < m_width; j++ )
        {
            if (m_subtitleBmp[i * m_width + j] == 0)
            {
                if (toFile) fputc('.', out);
		else putchar('.');
            }
            else if (m_subtitleBmp[i * m_width + j] == 1 /*|| m_subtitleBmp[i * m_width + j] < 128*/)
            {
                if (toFile)fputc('+', out);
		else putchar('+');
            }
            else
            {
                if (toFile)fputc('*', out);
		else putchar('*');
            }
        }
	if (toFile) fputc('\n', out);
	else putchar('\n');
    }
    if (toFile) fclose(out);
}
