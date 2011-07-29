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

#ifndef SUBTITLEENCODER_H
#define SUBTITLEENCODER_H

#include <ft2build.h>
#include FT_FREETYPE_H

#include "FreeType/font.h"
#include "FreeType/bitmapGlyph.h"

#include "logger.h"
#include "subtitle.h"

typedef struct XSub
{
    long long int startTime;
    long long int stopTime;
    unsigned char* data;
    unsigned int dataSize;
}XSub;

class SubtitleEncoder
{
public:
    SubtitleEncoder(int* err);
    ~SubtitleEncoder();

    int Initialize(char* fontName, int fontSize, unsigned int nbSubtitles, unsigned int maxNbLines, short width, short height, logger* log);
    int EncodeSubtitle(TxtSubtitle subTxt);
    XSub* getXsub();
private:

    // Debug
    void showImage(bool toFile);
    void debugColorMap();
    
    void createXSub(TxtSubtitle subTxt);
    void convertToXsubColorMap();
    unsigned int encodeXsub(unsigned int headerSize, unsigned int lineSize, unsigned int height, unsigned char* bitmap, unsigned int nbBitsWritten);
    unsigned int writeXsubInfo(unsigned char* buffer, int length, int color, int nbBitsWritten);
    unsigned int writeBits(unsigned char* buffer, int length, int value, int nbBitsWritten);
    unsigned int alignRow(unsigned char* buffer, int nbBitsWritten);

    // Utils
    unsigned int swap32(unsigned int x);

    // free type management
    FT_Library	m_library;
    Font* m_font;
    int m_fontSize;

    // Video size
    short m_width;
    short m_height;
    short m_heightBox;

    // Pen
    int m_penX;
    int m_penY;

    // Background
    int m_backGroundSize;
    unsigned char* m_subtitleBmp;

    // XSub management
    unsigned int m_nbSubtitles;
    unsigned int m_maxNbLines;
    XSub* m_xSub;
    unsigned char* m_tempBuffer;
    unsigned char* m_tempXsub;

    // log file
    logger* m_log;

    unsigned int m_intBuffer;
};

#endif // SUBTITLEENCODER_H
