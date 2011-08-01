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

#include "config.h"

#include <iostream>
#include <fstream>
#include "subtitle.h"
#include "SubtitleEncoder.h"
#include "avi.h"
#include "logger.h"

using namespace std;

int readConfig(char* configFileName, int* fontSize, char* fontName, char* logFileName, int* logLevel);

int main(int argc, char **argv)
{
    int err = 0;
    unsigned int nbSubtitles = 0;
    unsigned int maxNbLines = 0;
    Subtitle* txtSubs = NULL;
    SubtitleEncoder* subEnc = NULL;
    avi* aviMuxer = NULL;
    char* fileSrt = NULL;
    short width = 0;
    short height = 0;
    
    printf("XsubInsertion version %d.%d.%d\n", XSUB_INSERTION_MAJOR, XSUB_INSERTION_MINOR, XSUB_INSERTION_MAJOR);
    
    if (argc != 4)
    {
        printf ("You entered %d parameter(s)\n", argc-1);
        printf ("Usage: subs configFile fileAviIn fileAviOut\n");
        return 1;
    }

    // Config file parameters
    char* configFile = strdup(argv[1]);
    char fontName[256] = "\0";
    char logFileName[256] = "\0";
    int fontSize = 0;
    int logLevel = 0;
    logger* log = new logger();

    // Read config file
    //printf("Reading config file...\n");
    err = readConfig(configFile, &fontSize, fontName, logFileName, &logLevel);
    if (err != 0)
    {
	printf("Error in config file\n");
	return 1;
    }
    // if config file ok open log file
    err = log->initialize(logFileName, logLevel);
    if (err != 0)
    {
	printf("Cannot create log file\n");
	return 1;
    }
    log->initFileLog(argv[2], XSUB_INSERTION_MAJOR, XSUB_INSERTION_MINOR, XSUB_INSERTION_PATCH);
    
    // Create avi object to demux and remux with subtitle
    aviMuxer = new avi(argv[2], argv[3], &width, &height, log);
    // Get srt filename
    fileSrt = strdup(argv[2]);
    fileSrt[strlen(fileSrt) - 3] = 's';
    fileSrt[strlen(fileSrt) - 2] = 'r';
    fileSrt[strlen(fileSrt) - 1] = 't';
    unsigned int maxNbChar = width / fontSize;
    txtSubs = new Subtitle(&err, fileSrt, true, log, maxNbChar);
    if (err == 0)
    {
	txtSubs->ReadSubrip();
	nbSubtitles = txtSubs->GetNumberOfSubtitles();
	maxNbLines = txtSubs->GetMaxNbLines();
	//txtSub->PrintTxtSubStruct();
    }
    else
    {
	return 1;
    }
    // Create a subtitle encoder
    subEnc = new SubtitleEncoder(&err);
    if (err == 0)
    {
	// Initialize it with the font we defined there
	err = subEnc->Initialize(fontName, fontSize, nbSubtitles, maxNbLines, width, height, log);
	if (err != 0)
	{
	    return err;
	}
    }
    else
    {
	return err;
    }
    // Once we have transcoded the subtitles insert them in the avi file
    // re-Mux avi input file to output adding xsubs
    aviMuxer->Remux(txtSubs, subEnc, nbSubtitles);

    log->closeFileLog();
    
    if (txtSubs != NULL)
    {
        delete(txtSubs);
        txtSubs = NULL;
    }
    if (subEnc != NULL)
    {
        delete(subEnc);
        subEnc = NULL;
    }
    if (aviMuxer != NULL)
    {
        delete(aviMuxer);
        aviMuxer = NULL;
    }
    if (log != NULL)
    {
        delete(log);
        log = NULL;
    }
    free (fileSrt);
    free (configFile);
    return 0;
}

int readConfig(char* configFileName, int* fontSize, char* fontName, char* logFileName, int* logLevel)
{
    ifstream config(configFileName);
    string line;
    string name;
    string value;
    int posEqual = 0;
    int err = 0;
    
    while(getline(config, line))
    {
	if(line.length() == 0)
	    continue;
	if (line[0] == '#')
	    continue;
	
	posEqual=line.find('=');
	name  = line.substr(0,posEqual);
	value = line.substr(posEqual+1);
	
	// Log file config
	if (!strcmp(name.c_str(), "logFile"))
	{
	    strcpy(logFileName, value.c_str());
	}
	// Font informations
	if (!strcmp(name.c_str(), "font"))
	{
	    strcpy(fontName, value.c_str());
	}
	if (!strcmp(name.c_str(), "fontSize"))
	{
	    *fontSize = atoi(value.c_str());
	}
	if (!strcmp(name.c_str(), "logLevel"))
	{
	    *logLevel = atoi(value.c_str());
	}

    }
    config.close();
    
    // check
    if (*fontSize == 0)
    {
	printf("No font size found");
	err = 1;
    }
    if (*logLevel > 5) *logLevel = 5;
    if (*logLevel < 0) *logLevel = 0;
    if (fontName[0] == '\0')
    {
	printf("No font file defined");
	err = 2;
    }
    if (logFileName[0] == '\0')
    {
	printf("No log file defined");
	err = 3;
    }  
    return err;
}
