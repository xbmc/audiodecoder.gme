/*
 *  Copyright (C) 2014-2022 Arne Morten Kvarving
 *  Copyright (C) 2016-2022 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "GMECodec.h"

#include <kodi/Filesystem.h>

CGMECodec::CGMECodec(const kodi::addon::IInstanceInfo& instance) : CInstanceAudioDecoder(instance)
{
}

CGMECodec::~CGMECodec()
{
  if (ctx.gme)
    gme_delete(ctx.gme);
}

bool CGMECodec::Init(const std::string& filename,
                     unsigned int filecache,
                     int& channels,
                     int& samplerate,
                     int& bitspersample,
                     int64_t& totaltime,
                     int& bitrate,
                     AudioEngineDataFormat& format,
                     std::vector<AudioEngineChannel>& channellist)
{
  int track = 0;
  const std::string toLoad = kodi::addon::CInstanceAudioDecoder::GetTrack("gme", filename, track);

  // Correct if packed sound file with several sounds
  if (track > 0)
    --track;

  gme_open_file(toLoad.c_str(), &ctx.gme, 48000);
  if (!ctx.gme)
    return false;

  channels = 2;
  samplerate = 48000;
  bitspersample = 16;
  bitrate = 0.0;
  format = AUDIOENGINE_FMT_S16NE;
  gme_info_t* out;
  gme_track_info(ctx.gme, &out, track);
  totaltime = ctx.len = out->play_length;
  channellist = {AUDIOENGINE_CH_FL, AUDIOENGINE_CH_FR};
  gme_start_track(ctx.gme, track);

  return true;
}

int CGMECodec::ReadPCM(uint8_t* buffer, size_t size, size_t& actualsize)
{
  if (gme_tell(ctx.gme) >= ctx.len)
    return AUDIODECODER_READ_EOF;
  actualsize = size;
  gme_play(ctx.gme, size / 2, (short*)buffer);
  return AUDIODECODER_READ_SUCCESS;
}

int64_t CGMECodec::Seek(int64_t time)
{
  gme_seek(ctx.gme, time);
  return gme_tell(ctx.gme);
}

bool CGMECodec::ReadTag(const std::string& filename, kodi::addon::AudioDecoderInfoTag& tag)
{
  int track = 0;
  const std::string toLoad = kodi::addon::CInstanceAudioDecoder::GetTrack("gme", filename, track);

  gme_t* gme = nullptr;
  gme_open_file(toLoad.c_str(), &gme, 48000);
  if (!gme)
    return false;

  gme_info_t* out;
  gme_track_info(gme, &out, track > 0 ? track - 1 : 0);
  tag.SetTrack(track);
  tag.SetSamplerate(48000);
  tag.SetChannels(2);
  tag.SetDuration(out->play_length / 1000);
  if (out->song)
    tag.SetTitle(out->song);
  if (out->game && tag.GetTitle().empty())
    tag.SetTitle(out->game);
  if (out->author)
    tag.SetArtist(out->author);
  if (out->game)
    tag.SetAlbum(out->game);
  if (out->comment)
    tag.SetComment(out->comment);
  gme_delete(gme);
  return true;
}

int CGMECodec::TrackCount(const std::string& fileName)
{
  gme_t* gme = nullptr;
  gme_open_file(fileName.c_str(), &gme, 48000);
  if (!gme)
    return 1;

  int result = gme_track_count(gme);
  gme_delete(gme);

  return result;
}

//------------------------------------------------------------------------------

class ATTR_DLL_LOCAL CMyAddon : public kodi::addon::CAddonBase
{
public:
  CMyAddon() = default;
  ADDON_STATUS CreateInstance(const kodi::addon::IInstanceInfo& instance,
                              KODI_ADDON_INSTANCE_HDL& hdl) override
  {
    hdl = new CGMECodec(instance);
    return ADDON_STATUS_OK;
  }
  virtual ~CMyAddon() = default;
};

ADDONCREATOR(CMyAddon)
