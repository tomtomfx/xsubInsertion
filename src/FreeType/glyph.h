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

#ifndef GLYPH_H
#define GLYPH_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H
#include FT_GLYPH_H

#include "bitmapGlyph.h"

class Glyph
{
  
private:
    // the face of this glyph
    FT_Face m_face;
    // the index of the glyph in the face
    FT_UInt m_index;
    // the horizontal size of the glyph
    int m_hSize;
    // the vertical size of the glyph
    int m_vSize;
    // Flag set to tru ewhen the glyph has been rendered
    bool rendered;
    // the bounds of the enclosing box
    int m_minX, m_minY, m_maxX, m_maxY;
    int m_bearingY;
    unsigned int m_charCode;
    // the glyph image
    FT_Glyph m_glyph;
    // the glyph bitmap
    FT_BitmapGlyph m_bitmap;
    
    void render();
    
public:
  Glyph(FT_Face face, unsigned int charCode);
  ~Glyph();
  
  bool getInfo (BitmapGlyph* bitmapGlyph, int vSize, int hSize);
};

#endif // GLYPH_H
