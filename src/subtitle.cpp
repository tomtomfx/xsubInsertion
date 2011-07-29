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
#include "subtitle.h"

Subtitle::Subtitle(int* err, char* inFile, bool isText, logger* log, unsigned int maxNbChar)
{
    m_subNumber	= 0;
    m_subFile	= NULL;
    char temp[256];

    // log file
    m_log = log;

    // Max number of characters per line
    m_maxNbChar = maxNbChar;
    
    // Open subtitle file
    m_subFile = fopen(inFile, "r");
    if (m_subFile == NULL)
    {
	sprintf(temp, "ERROR : %s cannot be opened...\n", inFile);
        m_log->write(temp, 0);
        *err = 1;
    }

    if (*err == 0 && isText)
    {
        // Compute number of subtitles
        ComputeNumberOfSubtitles();
    }
}

Subtitle::~Subtitle()
{
    if (m_subFile != NULL)
    {
        fclose(m_subFile);
    }
}

void Subtitle::ComputeNumberOfSubtitles()
{
    m_subNumber = 0;
    char* line = NULL;
    char comparison[512] = "";
    line = (char*) malloc(2048 * sizeof(char));
    char previousWasBlank = 1;

    while (line != NULL)
    {
        // char from subtitle number
        sprintf(comparison, "%d", m_subNumber);
        // Get new line and delete last char
        line = fgets(line, 2048, m_subFile);
        if (line == NULL)
        {
            break;
        }
	line[strlen(line)-2] = '\0';
	if (!strcmp(line, "\0"))
	{
	    previousWasBlank = 1;
	    continue;
	}
        int value = 0;
        sscanf(line, "%d", &value);
        if (strchr(line, ':'))
	{
	    continue;
	}
        if ( value > m_subNumber && previousWasBlank/*!strcmp(line, comparison)*/)
        {
            m_subNumber++;
	    previousWasBlank = 0;
        }
    }
    free(line);
}

void Subtitle::ReadSubrip()
{
    char* line	= NULL;
    int a1, a2, a3, a4, b1, b2, b3, b4;
    int currentSub = -1;
    int tmp;
    int state;
    timeCode lastStoptime;
    unsigned int startSpaceSearch = m_maxNbChar - (m_maxNbChar * 1 / 4);

    m_log->write("INFO : Reading subtitles (srt)\n", 3);

    // allocation du buffer de sous titres
    m_txtSub = (TxtSubtitle*) malloc (m_subNumber * sizeof(TxtSubtitle));

    fseek(m_subFile, 0, SEEK_SET);
    line = (char*) malloc(2048 * sizeof(char));

    state=0;
    while ( (line = fgets(line, 2048, m_subFile)) != NULL)
    {
        switch (state)
        {
        case 0:
            if ( *line != '\n' )
            {
                // Scan file with subtitle timing pattern
                if (sscanf(line, "%d:%d:%d%[,.:]%d --> %d:%d:%d%[,.:]%d", &a1, &a2, &a3, (char *)&tmp, &a4, &b1, &b2, &b3, (char *)&tmp, &b4) == 10)
                {
                    currentSub++;
                    /* start and end times in hours:minutes:seconds.hundredths */
                    m_txtSub[currentSub].startTime.hours = a1;
                    m_txtSub[currentSub].startTime.minutes = a2;
                    m_txtSub[currentSub].startTime.secondes = a3;
                    m_txtSub[currentSub].startTime.milliSeconds = a4;
                    if (CompareTimeCode(m_txtSub[currentSub].startTime, lastStoptime) < 0)
                    {
                        m_txtSub[currentSub].startTime.hours = lastStoptime.hours;
                        m_txtSub[currentSub].startTime.minutes = lastStoptime.minutes;
                        m_txtSub[currentSub].startTime.secondes = lastStoptime.secondes;
                        m_txtSub[currentSub].startTime.milliSeconds = lastStoptime.milliSeconds + 10;
                    }
                    m_txtSub[currentSub].stopTime.hours = b1;
                    m_txtSub[currentSub].stopTime.minutes = b2;
                    m_txtSub[currentSub].stopTime.secondes = b3;
                    m_txtSub[currentSub].stopTime.milliSeconds = b4;

                    // Save stop time to check next subtitle starts after the end of the current one
                    lastStoptime.hours = m_txtSub[currentSub].stopTime.hours;
                    lastStoptime.minutes = m_txtSub[currentSub].stopTime.minutes;
                    lastStoptime.secondes = m_txtSub[currentSub].stopTime.secondes;
                    lastStoptime.milliSeconds = m_txtSub[currentSub].stopTime.milliSeconds;
                    state=1;
                }
            }
            break;
        case 1:
            // Delete blank lines between time and text lines.
            if ( *line != '\n' && *line != '\r' )
            {
                state=2;
            }
            else
            {
                break;
            }
        case 2:
            if ( *line == '\n' || *line == '\r')
            {
                state=0;
            }
            else
            {
                // Get text line so copy it to output string
                if (strcmp(line, "") != 0  )
                {
                    int length = strlen(m_txtSub[currentSub].subTxt);
		    if (strlen(line) > m_maxNbChar)
		    {
			for (unsigned int i=startSpaceSearch; i<strlen(line); i++)
			{
			    if(line[i] == ' ')
			    {
				line[i] = '\n';
				m_txtSub[currentSub].nbLines++;
				break;
			    }
			}
		    }
                    for (unsigned int i=0; i<strlen(line); i++)
                    {
                        m_txtSub[currentSub].subTxt[length + i] = line[i];
                    }
                    // Save the number of lines of each subtitle
                    m_txtSub[currentSub].nbLines++;
                }
            }
            break;
        }
    }
    m_log->write("INFO : Subtitle extract done\n", 1);
    free(line);
    //PrintTxtSubStruct();
}

int Subtitle::GetSubtitleByIndex(unsigned int index, TxtSubtitle* txtSub)
{
    if (index < (unsigned int)m_subNumber)
    {
        *txtSub = m_txtSub[index];
        return 1;
    }
    else
    {
        txtSub = NULL;
        return 0;
    }
}

unsigned int Subtitle::GetMaxNbLines()
{
    int maxNbLines = 0;
    for (int i=0; i<m_subNumber; i++)
    {
        if (m_txtSub[i].nbLines > maxNbLines)
        {
            maxNbLines = m_txtSub[i].nbLines;
        }
    }
    return maxNbLines;
}

// Compare time code
int Subtitle::CompareTimeCode(timeCode tc1, timeCode tc2)
{
    int diff = 0;
    diff = (tc2.hours - tc1.hours) * 3600000;
    diff += (tc2.minutes - tc1.minutes) * 60000;
    diff += (tc2.secondes - tc1.secondes) * 1000;
    diff += (tc2.milliSeconds - tc1.milliSeconds);
    if (diff < 0)
    {
        diff = 1;
    }
    else
    {
        diff = 0;
    }
    return diff;
}

// Debug function that print the subtitle in the terminal
void Subtitle::PrintTxtSubStruct()
{
    printf("Print subtitles\n");
    for (int i=0; i<m_subNumber; i++)
    {
        printf("%s", m_txtSub[i].subTxt);
    }
}
