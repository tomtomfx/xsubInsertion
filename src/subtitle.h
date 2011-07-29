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

#ifndef SUBTITLE_H
#define SUBTITLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"

typedef struct
{
    unsigned int hours;
    unsigned int minutes;
    unsigned int secondes;
    unsigned int milliSeconds;
}timeCode;

typedef struct
{
    timeCode startTime;
    timeCode stopTime;
    int nbLines;
    char subTxt[2048];
    int alignement;
}TxtSubtitle;

class Subtitle
{
public:
    Subtitle(int* err, char* inFile, bool isText, logger* log, unsigned int m_maxNbChar);
    ~Subtitle();

    void ReadSubrip();
    void PrintTxtSubStruct();
    
    int GetNumberOfSubtitles() {
        return m_subNumber;
    }
    
    int GetSubtitleByIndex(unsigned int index, TxtSubtitle* txtSub);
    unsigned int GetMaxNbLines();

private:
    void ComputeNumberOfSubtitles();
    int CompareTimeCode(timeCode tc1, timeCode tc2);

    FILE* m_subFile;

    int m_subNumber;
    TxtSubtitle* m_txtSub;

    // log file
    logger* m_log;
    
    // Max number of characters per line
    unsigned int m_maxNbChar;

};

#endif // SUBTITLE_H
