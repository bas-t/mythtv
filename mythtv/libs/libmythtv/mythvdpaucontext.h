#ifndef MYTHVDPAUCONTEXT_H
#define MYTHVDPAUCONTEXT_H

// MythTV
#include "mythhwcontext.h"
#include "mythcodeccontext.h"

class MythVDPAUContext
{
  public:
    static MythCodecID GetSupportedCodec (AVCodecContext *CodecContext,
                                          AVCodec       **Codec,
                                          const QString  &Decoder,
                                          uint            StreamType,
                                          AVPixelFormat  &PixFmt);
    static enum AVPixelFormat GetFormat  (AVCodecContext *Context,
                                          const enum AVPixelFormat *PixFmt);

  private:
    static int  InitialiseContext        (AVCodecContext *Context);
};

#endif // MYTHVDPAUCONTEXT_H