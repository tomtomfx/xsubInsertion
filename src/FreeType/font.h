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

#ifndef FONT_H
#define FONT_H

#include <ft2build.h>
#include FT_FREETYPE_H

#include "bitmapGlyph.h"
#include "glyph.h"

#define MAX_INITIAL_GLYPHS 256
#define MAX_UNICODE_GLYPHS 65536

class Font
{
private:
    // the font name
    char * m_name;
    // a reference to the font library
    FT_Library m_library;
    // the font face object
    FT_Face m_face;
    // the current device resolution in dot per inches (dpi)
    int  m_resolution;  // default resolution in dpi
    // the horizontal size in point of the font
    int m_hPointSize;
    // the vertical size in point of the font
    int m_vPointSize;

    bool m_useTransform;
    FT_Matrix m_matrix;

    // an array for glyph caching, always allocated
    Glyph ** m_glyphs;
    // size of the array for glyph caching
    unsigned int m_glyphsSize;

public:
    Font(FT_Library library, char * name);
    ~Font();

    bool load();
    void setSize (int hPointSize, int vPointSize);
    void setTransform (float scaleX, float scaleY, float angle);
    bool getGlyphInfo (unsigned long charCode, BitmapGlyph* bitmapGlyph);

private:
    bool resizeGlyphs(int minSize);
};

#endif // FONT_H
