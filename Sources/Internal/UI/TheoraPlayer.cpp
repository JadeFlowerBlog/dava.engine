/*==================================================================================
 Copyright (c) 2008, DAVA Consulting, LLC
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 * Neither the name of the DAVA Consulting, LLC nor the
 names of its contributors may be used to endorse or promote products
 derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE DAVA CONSULTING, LLC AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL DAVA CONSULTING, LLC BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 Revision History:
 * Created by Igor Solovey
 =====================================================================================*/

#include "UI/TheoraPlayer.h"

namespace DAVA
{
    
REGISTER_CLASS(TheoraPlayer);

TheoraPlayer::TheoraPlayer(const String & filePath) :
thSetup(0),
thCtx(0),
isPlaying(false),
theora_p(0),
isVideoBufReady(false),
videoBufGranulePos(-1),
videoTime(0),
isRepeat(false),
currFrameTime(0),
frameTime(0)
{
    SetClipContents(true);
    OpenFile(filePath);
}

TheoraPlayer::~TheoraPlayer()
{
    CloseFile();
}

int32 TheoraPlayer::BufferData()
{
    char * buffer = ogg_sync_buffer(&syncState, 4096);
    int32 bytes = file->Read(buffer, 4096);
    ogg_sync_wrote(&syncState, bytes);
    return bytes;
}

void TheoraPlayer::CloseFile()
{
    if(thSetup)
        th_setup_free(thSetup);
    thSetup = 0;
    if(thCtx)
        th_decode_free(thCtx);
    thCtx = 0;
    theora_p = 0;
    isVideoBufReady = false;
    videoBufGranulePos = -1;
    videoTime = 0;
    ogg_sync_clear(&syncState);
    SafeRelease(file);
    SafeDelete(frameBuffer);
}

void TheoraPlayer::OpenFile(const String &path)
{
    if(path == "")
        return;
    
    filePath = path;
    
    file = File::Create(path, File::OPEN | File::READ);
    ogg_sync_init(&syncState);
    th_info_init(&thInfo);
    th_comment_init(&thComment);
    
    int32 stateflag = 0;
    while(!stateflag)
    {
        if(!BufferData())
            break;
        
        while(ogg_sync_pageout(&syncState, &page) > 0)
        {
            ogg_stream_state test;
            
            /* is this a mandated initial header? If not, stop parsing */
            if(!ogg_page_bos(&page))
            {
                /* don't leak the page; get it into the appropriate stream */
                ogg_stream_pagein(&state, &page);
                stateflag = 1;
                break;
            }
            
            ogg_stream_init(&test, ogg_page_serialno(&page));
            ogg_stream_pagein(&test, &page);
            ogg_stream_packetout(&test, &packet);
            
            /* identify the codec: try theora */
            if(!theora_p && th_decode_headerin(&thInfo, &thComment, &thSetup, &packet) >= 0)
            {
                /* it is theora */
                memcpy(&state, &test, sizeof(test));
                theora_p = 1;
            }
            else
            {
                /* whatever it is, we don't care about it */
                ogg_stream_clear(&test);
            }
        }
        /* fall through to non-bos page parsing */
    }
    
    while(theora_p && theora_p < 3)
    {
        int ret;
        
        /* look for further theora headers */
        while(theora_p && (theora_p < 3) && (ret = ogg_stream_packetout(&state, &packet)))
        {
            if(ret < 0)
            {
                Logger::Error("TheoraPlayer: Error parsing Theora stream headers; corrupt stream?\n");
                return;
            }
            if(!th_decode_headerin(&thInfo, &thComment, &thSetup, &packet))
            {
                Logger::Error("TheoraPlayer: Error parsing Theora stream headers; corrupt stream?\n");
                return;
            }
            theora_p++;
        }
        
        /* The header pages/packets will arrive before anything else we
         care about, or the stream is not obeying spec */
        
        if(ogg_sync_pageout(&syncState, &page) > 0)
        {
            ogg_stream_pagein(&state, &page); /* demux into the appropriate stream */
        }
        else
        {
            int ret = BufferData(); /* someone needs more data */
            if(ret == 0)
            {
                Logger::Error("TheoraPlayer: End of file while searching for codec headers.\n");
                return;
            }
        }
    }
    if(theora_p)
    {
        thCtx = th_decode_alloc(&thInfo, thSetup);
        
        th_decode_ctl(thCtx, TH_DECCTL_GET_PPLEVEL_MAX, &pp_level_max, sizeof(pp_level_max));
        pp_level=pp_level_max;
        th_decode_ctl(thCtx, TH_DECCTL_SET_PPLEVEL, &pp_level, sizeof(pp_level));
        pp_inc=0;
    }
    else
    {
        /* tear down the partial theora setup */
        th_info_clear(&thInfo);
        th_comment_clear(&thComment);
    }
    
    if(thSetup)
        th_setup_free(thSetup);
    thSetup = 0;

    frameBufferW = binCeil(thInfo.pic_width);
    frameBufferH = binCeil(thInfo.pic_height);
    
    frameBuffer = new unsigned char[frameBufferW * frameBufferH * 4];
    
    repeatFilePos = file->GetPos();
    
    frameTime = (float32)(thInfo.fps_denominator)/(float32)(thInfo.fps_numerator);
}

void TheoraPlayer::SetPlaying(bool _isPlaying)
{
    isPlaying = _isPlaying;
}

void TheoraPlayer::SetRepeat(bool _isRepeat)
{
    isRepeat = _isRepeat;
}

bool TheoraPlayer::IsRepeat()
{
    return isRepeat;
}

bool TheoraPlayer::IsPlaying()
{
    return isPlaying;
}

void TheoraPlayer::Update(float32 timeElapsed)
{
    if(!isPlaying)
        return;
        
    videoTime += timeElapsed;
    
    currFrameTime += timeElapsed;
    if(currFrameTime < frameTime)
    {
        return;
    }
    else
    {
        currFrameTime -= frameTime;
    }
    
    int ret;
    
    while(theora_p && !isVideoBufReady)
    {
        ret = ogg_stream_packetout(&state, &packet);
        if(ret > 0)
        {
            if(pp_inc)
            {
                pp_level += pp_inc;
                th_decode_ctl(thCtx, TH_DECCTL_SET_PPLEVEL, &pp_level, sizeof(pp_level));
                pp_inc = 0;
            }
            if(packet.granulepos >= 0)
                th_decode_ctl(thCtx, TH_DECCTL_SET_GRANPOS, &packet.granulepos, sizeof(packet.granulepos));

            if(th_decode_packetin(thCtx, &packet, &videoBufGranulePos) == 0)
            {
                if((videoBufTime = th_granule_time(thCtx, videoBufGranulePos)) >= videoTime)
                    isVideoBufReady = true;
                else
                    pp_inc = (pp_level > 0)? -1 : 0;
            }
        }
        else
        {
            isVideoBufReady = false;
            break;
        }
    }
    
    if(!isVideoBufReady)
    {
        BufferData();
        while(ogg_sync_pageout(&syncState, &page) > 0)
            ogg_stream_pagein(&state, &page);
    }
    
    if(isVideoBufReady)
    {
        isVideoBufReady = false;
        ret = th_decode_ycbcr_out(thCtx, yuvBuffer);
    
        for(int i = 0; i < frameBufferH; i++) //Y
        {
            int yShift = yuvBuffer[0].stride * i;
            int uShift = yuvBuffer[1].stride * (i / 2);
            int vShift = yuvBuffer[2].stride * (i / 2);
            for(int j = 0; j < frameBufferW; j++) //X
            {
                int index = (i * frameBufferW + j) * 4;
                
                if(i <= yuvBuffer[0].height && j <= yuvBuffer[0].width)
                {
                    unsigned char Y = *(yuvBuffer[0].data + yShift + j);
                    unsigned char U = *(yuvBuffer[1].data + uShift + j / 2);
                    unsigned char V = *(yuvBuffer[2].data + vShift + j / 2);
                
                    frameBuffer[index]   = ClampFloatToByte(Y + 1.371f * (V - 128));
                    frameBuffer[index+1] = ClampFloatToByte(Y - 0.698f * (V - 128) - 0.336f * (U - 128));
                    frameBuffer[index+2] = ClampFloatToByte(Y + 1.732f * (U - 128));
                    frameBuffer[index+3] = 255;
                }
                else
                {
                    memset(&frameBuffer[index], 0, 4 * sizeof(unsigned char));
                }
            }
        }
    
        if(!ret)
        {
            Texture * tex = Texture::CreateFromData(FORMAT_RGBA8888, frameBuffer, frameBufferW, frameBufferH);
            Sprite * spr = Sprite::CreateFromTexture(tex, 0, 0, tex->width, tex->height);
            spr->ConvertToVirtualSize();

            SafeRelease(tex);
            SetSprite(spr, 0);
            SafeRelease(spr);
        }
    }
    
    if(theora_p)
    {
        double tdiff = videoBufTime - videoTime;
        /*If we have lots of extra time, increase the post-processing level.*/
        if(tdiff > thInfo.fps_denominator * 0.25f / thInfo.fps_numerator)
        {
            pp_inc = (pp_level < pp_level_max) ? 1 : 0;
        }
        else if(tdiff < thInfo.fps_denominator * 0.05 / thInfo.fps_numerator)
        {
            pp_inc = (pp_level > 0)? -1 : 0;
        }
    }
    if(isRepeat && file->GetPos() == file->GetSize())
    {
        CloseFile();
        OpenFile(filePath);
    }
}

unsigned char TheoraPlayer::ClampFloatToByte(const float &value)
{
    char result = (char)value;
    
    (value < 0) ? result = 0 : NULL;
    (value > 255) ? result = 255 : NULL;
    
    return (unsigned char)result;
}

uint32 TheoraPlayer::binCeil(uint32 value)
{
    uint32 bin = 1;
    while (bin < value)
        bin *= 2;
    return bin;
}

void TheoraPlayer::LoadFromYamlNode(YamlNode * node, UIYamlLoader * loader)
{
    UIControl::LoadFromYamlNode(node, loader);
    YamlNode * fileNode = node->AsMap()["file"];
    
	if(fileNode)
        OpenFile(fileNode->AsString());
    
    YamlNode * rectNode = node->Get("rect");
    
    if(rectNode)
    {
        Rect rect = rectNode->AsRect();
        if(rect.dx == -1)
            rect.dx = thInfo.pic_width;
        if(rect.dy == -1)
            rect.dy = thInfo.pic_height;
        
        SetRect(rect);
    }
}

void TheoraPlayer::Draw(const UIGeometricData &geometricData)
{
    if(GetSprite())
    {
        GetSprite()->SetPosition(geometricData.position);
        GetSprite()->Draw();
    }
}
    
}