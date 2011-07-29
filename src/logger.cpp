/*
    Log system
    Copyright (C) 2011 Thomas Fayoux

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

#include "logger.h"
#include <time.h>

logger::logger()
{
}

logger::~logger()
{
    fclose(m_logFile);
}

int logger::initialize(char* logFileName, int logLevel)
{
    int err = 0;
    m_logLevel = logLevel;
    m_logFile = fopen(logFileName, "a+");
    if (m_logFile == NULL)
    {
        err = 1;
    }
    return err;
}

int logger::write(char* data, int level)
{
    int err = 0;
    if (level <= m_logLevel)
    {
	fprintf(m_logFile, data);
    }
    return err;
}

void logger::initFileLog(char* filename, int major, int minor, int patch)
{
    // Time management
    time_t currentTime;
    time(&currentTime);
    struct tm * timeinfo;
    timeinfo = localtime(&currentTime);
    
    fprintf(m_logFile, "**************************************************************************************\n");
    fprintf(m_logFile, "Addxsub version %d.%d.%d\n", major, minor, patch);
    fprintf(m_logFile, "%s", asctime(timeinfo));
    fprintf(m_logFile, "%s\n", filename);
}

void logger::closeFileLog()
{
    fprintf(m_logFile, "**************************************************************************************\n\n\n");
}