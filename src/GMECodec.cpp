/*
 *      Copyright (C) 2014 Arne Morten Kvarving
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "libXBMC_addon.h"
#include "gme.h"

extern "C" {
#include <stdio.h>
#include <stdint.h>

#include "kodi_audiodec_dll.h"

ADDON::CHelper_libXBMC_addon *XBMC           = NULL;

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!XBMC)
    XBMC = new ADDON::CHelper_libXBMC_addon;

  if (!XBMC->RegisterMe(hdl))
  {
    delete XBMC, XBMC=NULL;
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  return ADDON_STATUS_OK;
}

//-- Stop ---------------------------------------------------------------------
// This dll must cease all runtime activities
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Stop()
{
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Destroy()
{
  XBMC=NULL;
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
bool ADDON_HasSettings()
{
  return false;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------

void ADDON_FreeSettings()
{
}

//-- SetSetting ---------------------------------------------------------------
// Set a specific Setting value (called from XBMC)
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void* value)
{
  return ADDON_STATUS_OK;
}

//-- Announce -----------------------------------------------------------------
// Receive announcements from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}

struct GMEContext {
  gme_t* gme;
  int len;
};

void* Init(const char* strFile, unsigned int filecache, int* channels,
           int* samplerate, int* bitspersample, int64_t* totaltime,
           int* bitrate, AEDataFormat* format, const AEChannel** channelinfo)
{
  int track=0;
  std::string toLoad(strFile);
  if (toLoad.rfind("stream") != std::string::npos)
  {
    size_t iStart=toLoad.rfind('-') + 1;
    track = atoi(toLoad.substr(iStart, toLoad.size()-iStart-10).c_str())-1;
    //  The directory we are in, is the file
    //  that contains the bitstream to play,
    //  so extract it
    size_t slash = toLoad.rfind('\\');
    if (slash == std::string::npos)
      slash = toLoad.rfind('/');
    toLoad = toLoad.substr(0, slash);
  }

  GMEContext* result = new GMEContext;

  gme_open_file(toLoad.c_str(), &result->gme, 48000);
  if (!result->gme)
  {
    delete result;
    return NULL;
  }
  *channels = 2;
  *samplerate = 48000;
  *bitspersample = 16;
  *bitrate = 0.0;
  *format = AE_FMT_S16NE;
  gme_info_t* out;
  gme_track_info(result->gme, &out, track);
  *totaltime = result->len = out->play_length;
  static enum AEChannel map[3] = {
    AE_CH_FL, AE_CH_FR, AE_CH_NULL
  };
  *channelinfo = map;
  gme_start_track(result->gme, track);
  return result;
}

int ReadPCM(void* context, uint8_t* pBuffer, int size, int *actualsize)
{
  GMEContext* gme = (GMEContext*)context;
  if (gme_tell(gme->gme) >= gme->len)
    return -1;
  *actualsize = size;
  gme_play(gme->gme, size/2, (short*)pBuffer);
  return 0;
}

int64_t Seek(void* context, int64_t time)
{
  GMEContext* gme = (GMEContext*)context;
  gme_seek(gme->gme, time);
  return gme_tell(gme->gme);
}

bool DeInit(void* context)
{
  if(!context)
    return true;

  GMEContext* gme = (GMEContext*)context;
  gme_delete(gme->gme);
  delete gme;

  return true;
}

bool ReadTag(const char* strFile, char* title, char* artist, int* length)
{
  gme_t* gme=NULL;
  gme_open_file(strFile, &gme, 48000);
  if (!gme)
    return 1;
  gme_info_t* out;
  gme_track_info(gme, &out, 0);
  *length = out->play_length/1000;
  strcpy(title, out->song);
  strcpy(artist, out->author);
  gme_delete(gme);
  return *length != 0;
}

int TrackCount(const char* strFile)
{
  gme_t* gme=NULL;
  gme_open_file(strFile, &gme, 48000);
  if (!gme)
    return 1;

  int result = gme_track_count(gme);
  gme_delete(gme);

  return result;
}
}
