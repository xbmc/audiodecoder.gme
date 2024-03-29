/*
 *  Copyright (C) 2014-2022 Arne Morten Kvarving
 *  Copyright (C) 2016-2022 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "gme.h"

#include <kodi/addon-instance/AudioDecoder.h>

struct GMEContext
{
  gme_t* gme = nullptr;
  int len;
};

class ATTR_DLL_LOCAL CGMECodec : public kodi::addon::CInstanceAudioDecoder
{
public:
  CGMECodec(const kodi::addon::IInstanceInfo& instance);
  virtual ~CGMECodec();

  bool Init(const std::string& filename,
            unsigned int filecache,
            int& channels,
            int& samplerate,
            int& bitspersample,
            int64_t& totaltime,
            int& bitrate,
            AudioEngineDataFormat& format,
            std::vector<AudioEngineChannel>& channellist) override;
  int ReadPCM(uint8_t* buffer, size_t size, size_t& actualsize) override;
  int64_t Seek(int64_t time) override;
  bool ReadTag(const std::string& filename, kodi::addon::AudioDecoderInfoTag& tag) override;
  int TrackCount(const std::string& fileName) override;

private:
  GMEContext ctx;
};
