/*
 *  Class TestIPTVRecorder
 *
 *  Copyright (C) Karl Dietz 2014
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <QtTest/QtTest>

#include "iptvtuningdata.h"
#include "channelscan/iptvchannelfetcher.h"
#include "recorders/rtp/rtpdatapacket.h"
#include "recorders/rtp/rtptsdatapacket.h"

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#define MSKIP(MSG) QSKIP(MSG, SkipSingle)
#else
#define MSKIP(MSG) QSKIP(MSG)
#endif

class TestIPTVRecorder: public QObject
{
    Q_OBJECT

  private slots:
    /**
     * Test if supported Urls really are considered valid.
     */
    void TuningData(void)
    {
        IPTVTuningData tuning;

        /* test url from #11949 without port, free.fr */
        tuning.SetDataURL(QUrl(QString("rtsp://mafreebox.freebox.fr/fbxtv_pub/stream?namespace=1&service=203&flavour=sd")));
        QVERIFY (tuning.IsValid());
        QVERIFY (tuning.IsRTSP());

        /* test url from #11949 with port, free.fr */
        tuning.SetDataURL(QUrl(QString("rtsp://mafreebox.freebox.fr:554/fbxtv_pub/stream?namespace=1&service=203&flavour=sd")));
        QVERIFY (tuning.IsValid());
        QVERIFY (tuning.IsRTSP());

        /* test url from #11852 with port, telekom.de */
        tuning.SetDataURL(QUrl(QString("rtp://@239.35.10.1:10000")));
        QVERIFY (tuning.IsValid());
        QVERIFY (tuning.IsRTP());

        /* test url from das-erste.de with port, telekom.de */
        tuning.SetDataURL(QUrl(QString("rtp://239.35.10.4:10000")));
        QVERIFY (tuning.IsValid());
        QVERIFY (tuning.IsRTP());

        /* test url from #11847 with port, Dreambox */
        tuning.SetDataURL(QUrl(QString("http://yourdreambox:8001/1:0:1:488:3FE:22F1:EEEE0000:0:0:0:")));
        QVERIFY (tuning.IsValid());
        QVERIFY (tuning.IsHTTPTS());
    }


    /**
     * Test if the expectation "if the Url works with VLC it should work with MythTV" is being met.
     */
    void TuningDataVLCStyle(void)
    {
        MSKIP ("Do we want to support non-conformant urls that happen to work with VLC?");
        IPTVTuningData tuning;

        /* test url from http://www.tldp.org/HOWTO/VideoLAN-HOWTO/x549.html */
        tuning.SetDataURL(QUrl(QString("udp:@239.255.12.42")));
        QVERIFY (tuning.IsValid());
        QVERIFY (tuning.IsUDP());

        /* test url from http://www.tldp.org/HOWTO/VideoLAN-HOWTO/x1245.html */
        tuning.SetDataURL(QUrl(QString("udp:@[ff08::1]")));
        QVERIFY (tuning.IsValid());
        QVERIFY (tuning.IsUDP());

        /* test url from http://www.tldp.org/HOWTO/VideoLAN-HOWTO/x1245.html */
        tuning.SetDataURL(QUrl(QString("udp:[ff08::1%eth0]")));
        QVERIFY (tuning.IsValid());
        QVERIFY (tuning.IsUDP());
    }

    /**
     * Test parsing a playlist
     */
    void ParseChanInfo(void)
    {
        /* #12077 - DVB with Neutrino - Sweden */
        QString rawdataHTTP ("#EXTM3U\n"
                             "#EXTINF:0,1 - SVT1 HD Mitt\n"
                             "#EXTMYTHTV:xmltvid=svt1hd.svt.se\n"
                             "#EXTVLCOPT:program=1330\n"
                             "http://192.168.0.234:8001/1:0:19:532:6:22F1:EEEE0000:0:0:0:\n");

        /* #11949 - FreeboxTV - France - Free */
        QString rawdataRTSP ("#EXTM3U\n"
                             "#EXTINF:0,2 - France 2 (bas débit)\n"
                             "rtsp://mafreebox.freebox.fr/fbxtv_pub/stream?namespace=1&service=201&flavour=ld\n"
                             "#EXTINF:0,2 - France 2 (HD)\n"
                             "rtsp://mafreebox.freebox.fr/fbxtv_pub/stream?namespace=1&service=201&flavour=hd\n"
                             "#EXTINF:0,2 - France 2 (standard)\n"
                             "rtsp://mafreebox.freebox.fr/fbxtv_pub/stream?namespace=1&service=201&flavour=sd\n"
                             "#EXTINF:0,2 - France 2 (auto)\n"
                             "rtsp://mafreebox.freebox.fr/fbxtv_pub/stream?namespace=1&service=201");

        /* #11963 - Movistar TV - Spain - Telefonica */
        QString rawdataUDP ("#EXTM3U\n"
                            "#EXTINF:0,001 - La 1\n"
                            "udp://239.0.0.76:8208\n");

        /*
         * SAT>IP style channel format from page 63 of
         * http://www.satip.info/sites/satip/files/resource/satip_specification_version_1_2.pdf
         */
        QString rawdataSATIP ("#EXTM3U\n"
                              "#EXTINF:0,10. ZDFinfokanal\n"
                              "rtp://239.0.0.76:8200\n");

        /*
         * Austrian A1 TV playlist with empty lines and channel number in tvg-num
         */
        QString rawdataA1TV ("#EXTM3U\n"
                             "# A1 TV Basispaket 2015-09-11 09:17\n"
                             "\n"
                             "#EXTINF:-1 tvg-num=\"1\" tvg-logo=\"609\",ORFeins\n"
                             "rtp://@239.2.16.1:8208\n"
                             "#EXTINF:-1 tvg-num=\"2\" tvg-logo=\"625\",ORF 2 W\n"
                             "rtp://@239.2.16.2:8208\n");

        /*
         * #12188 - Spanish Movistar TV playlist with channel number in square braces
         * first variant from https://code.mythtv.org/trac/ticket/12188#comment:6
         * second variant from http://www.adslzone.net/postt350532.html
         */
        QString rawdataMovistarTV ("#EXTM3U\n"
                                   "#EXTINF:-1,Canal Sur Andalucía [2275]\n"
                                   "rtp://@239.0.0.1:8208\n"
                                   "#EXTINF:-1,[001] La 1\n"
                                   "rtp://@239.0.0.76:8208\n");

        /*
         * #12610 - iptv.ink / russion iptv plugin forum playlist with channel number in duration
         */
        QString rawdataIPTVInk ("#EXTM3U\n"
                                "#EXTINF:0002 tvg-id=\"daserste.de\" group-title=\"DE Hauptsender\" tvg-logo=\"609281.png\", [COLOR gold]Das Erste[/COLOR]\n"
                                "http://api.iptv.ink/pl.m3u8?ch=71565725\n");

        fbox_chan_map_t chanmap;

        /* test plain old MPEG-2 TS over multicast playlist */
        chanmap = IPTVChannelFetcher::ParsePlaylist (rawdataUDP, NULL);
        QCOMPARE (chanmap["001"].m_name, QString ("La 1"));
        QVERIFY (chanmap["001"].IsValid ());
        QVERIFY (chanmap["001"].m_tuning.IsValid ());
        QCOMPARE (chanmap["001"].m_tuning.GetDataURL().toString(), QString ("udp://239.0.0.76:8208"));

        /* test playlist for Neutrino STBs */
        chanmap = IPTVChannelFetcher::ParsePlaylist (rawdataHTTP, NULL);
        QVERIFY (chanmap["1"].IsValid ());
        QVERIFY (chanmap["1"].m_tuning.IsValid ());
        QCOMPARE (chanmap["1"].m_name, QString ("SVT1 HD Mitt"));
        QCOMPARE (chanmap["1"].m_xmltvid, QString ("svt1hd.svt.se"));
        QCOMPARE (chanmap["1"].m_programNumber, (uint) 1330);
        QCOMPARE (chanmap["1"].m_tuning.GetDataURL().toString(), QString ("http://192.168.0.234:8001/1:0:19:532:6:22F1:EEEE0000:0:0:0:"));

        /* test playlist for FreeboxTV, last channel in playlist "wins" */
        chanmap = IPTVChannelFetcher::ParsePlaylist (rawdataRTSP, NULL);
        QVERIFY (chanmap["2"].IsValid ());
        QVERIFY (chanmap["2"].m_tuning.IsValid ());
        QCOMPARE (chanmap["2"].m_name, QString ("France 2 (auto)"));
        QCOMPARE (chanmap["2"].m_tuning.GetDataURL().toString(), QString ("rtsp://mafreebox.freebox.fr/fbxtv_pub/stream?namespace=1&service=201"));

        /* test playlist for SAT>IP with "#. name" instead of "# - name" */
        chanmap = IPTVChannelFetcher::ParsePlaylist (rawdataSATIP, NULL);
        QVERIFY (chanmap["10"].IsValid ());
        QVERIFY (chanmap["10"].m_tuning.IsValid ());
        QCOMPARE (chanmap["10"].m_name, QString ("ZDFinfokanal"));

        /* test playlist from A1 TV with empty lines and tvg-num */
        chanmap = IPTVChannelFetcher::ParsePlaylist (rawdataA1TV, NULL);
        QVERIFY (chanmap["1"].IsValid ());
        QVERIFY (chanmap["1"].m_tuning.IsValid ());
        QCOMPARE (chanmap["1"].m_name, QString ("ORFeins"));

        /* test playlist from Movistar TV with channel number in braces */
        chanmap = IPTVChannelFetcher::ParsePlaylist (rawdataMovistarTV, NULL);
        QVERIFY (chanmap["001"].IsValid ());
        QVERIFY (chanmap["001"].m_tuning.IsValid ());
        QCOMPARE (chanmap["001"].m_name, QString ("La 1"));
        QVERIFY (chanmap["2275"].IsValid ());
        QVERIFY (chanmap["2275"].m_tuning.IsValid ());
        QCOMPARE (chanmap["2275"].m_name, QString ("Canal Sur Andalucía"));

        /* test playlist from iptv.ink with channel number in duration */
        chanmap = IPTVChannelFetcher::ParsePlaylist (rawdataIPTVInk, NULL);
        QVERIFY (chanmap["0002"].IsValid ());
        QVERIFY (chanmap["0002"].m_tuning.IsValid ());
        QCOMPARE (chanmap["0002"].m_name, QString ("[COLOR gold]Das Erste[/COLOR]"));
    }

    /**
     * Test parsing of RTP packets
     */
    void ParseRTP(void)
    {
        /* #11852 - RTP packet from VLC - minimal RTP header and 7 TS packets */
        unsigned char packet_data0[1328] = {
            0x80, 0xA1, 0xB0, 0x16, 0x66, 0x2D, 0x90, 0x6E,  0x32, 0x4C, 0x6F, 0x10, 0x47, 0x00, 0x45, 0x17,
            0x69, 0x4D, 0x0E, 0xCC, 0xD9, 0x49, 0x8B, 0x3F,  0xAB, 0x6A, 0x0C, 0xA8, 0xBA, 0x69, 0x0E, 0x49,
            0x49, 0xEA, 0x90, 0x5E, 0xD3, 0xC4, 0xD0, 0x98,  0x53, 0x81, 0x0A, 0xD1, 0xCC, 0x67, 0xB0, 0x3A,
            0xDA, 0x08, 0x6A, 0x53, 0xF8, 0xD4, 0xF0, 0x8C,  0xAE, 0xC3, 0x35, 0x32, 0x18, 0x05, 0x4E, 0xB3,
            0x0E, 0xAD, 0x4C, 0x19, 0xC8, 0xDA, 0xEB, 0x02,  0x9C, 0xBF, 0xD3, 0xC6, 0xBF, 0xC1, 0xD0, 0x67,
            0xB2, 0xDB, 0xC4, 0x03, 0xBB, 0x2A, 0xFE, 0xE4,  0xA0, 0xED, 0x28, 0x88, 0x28, 0x08, 0x43, 0x76,
            0x11, 0xAE, 0xBB, 0x24, 0x53, 0x2D, 0x81, 0x81,  0x4F, 0x24, 0xFC, 0x09, 0x90, 0xC1, 0xCA, 0x0C,
            0xE9, 0x52, 0x40, 0x76, 0x1C, 0xD5, 0x27, 0x9A,  0x06, 0x69, 0xAE, 0x7E, 0xDA, 0x7D, 0xC7, 0x18,
            0x4A, 0xCA, 0x74, 0x47, 0x82, 0x3B, 0x40, 0xC9,  0x8B, 0xE3, 0xDC, 0xEC, 0x93, 0x4C, 0x24, 0x0A,
            0x94, 0x57, 0xD5, 0x13, 0x19, 0xB2, 0xC3, 0xEE,  0xFF, 0xE1, 0xFB, 0x27, 0x0A, 0x9C, 0x84, 0x8F,
            0x9B, 0x1B, 0x10, 0x3B, 0xDA, 0x33, 0xAB, 0x2F,  0xE4, 0x0B, 0x1C, 0x2A, 0x63, 0x18, 0xD2, 0xD7,
            0x78, 0x03, 0x9F, 0xDC, 0x08, 0xCE, 0x7C, 0xD3,  0x31, 0x4A, 0xA1, 0xE9, 0x01, 0x35, 0x75, 0xAF,
            0xB2, 0x4F, 0x17, 0x90, 0x2F, 0x77, 0xE5, 0xF4,  0x47, 0x00, 0x45, 0x18, 0x17, 0x65, 0x97, 0x7C,
            0xBE, 0xBA, 0x6D, 0xB2, 0xF8, 0x2F, 0x25, 0xFF,  0x54, 0x52, 0x7A, 0xDB, 0x91, 0x20, 0x09, 0xB6,
            0xEA, 0x61, 0xB0, 0x93, 0x2B, 0xF6, 0x85, 0xA9,  0xF8, 0x16, 0xA6, 0x9A, 0x49, 0x20, 0x94, 0x04,
            0xB6, 0x37, 0x82, 0x46, 0x25, 0x25, 0x47, 0xD7,  0xC2, 0xA1, 0xAA, 0x4F, 0xA2, 0x97, 0xE7, 0x46,

            0xDD, 0x61, 0x45, 0x95, 0x4F, 0x7F, 0x64, 0x24,  0x82, 0x9C, 0xBC, 0x43, 0x92, 0xFD, 0x0C, 0xDF,
            0xF4, 0x27, 0xBB, 0x6E, 0x8B, 0xC3, 0x4C, 0x62,  0xA2, 0xEC, 0x93, 0x5E, 0xD1, 0x40, 0x3D, 0xDF,
            0xC9, 0xB6, 0x47, 0x03, 0xC7, 0x01, 0x04, 0xF7,  0xA3, 0x56, 0xA5, 0x55, 0x0E, 0x63, 0xD6, 0x07,
            0xAA, 0x63, 0x51, 0xDD, 0x75, 0xEE, 0x34, 0x55,  0xEA, 0x8A, 0x80, 0xDD, 0xC0, 0x6D, 0xD0, 0x57,
            0x79, 0xE0, 0x57, 0x55, 0xA9, 0xB9, 0xCB, 0x1B,  0x34, 0xB9, 0x1D, 0xB7, 0x68, 0x8B, 0x53, 0xF3,
            0x95, 0xFC, 0x24, 0xC0, 0x8A, 0x40, 0xAC, 0xA9,  0x46, 0x3C, 0x6A, 0xB1, 0x5A, 0xED, 0x1B, 0xE0,
            0xB5, 0xDD, 0x3C, 0xDD, 0x3B, 0xDA, 0xB4, 0x10,  0xAD, 0xE2, 0xED, 0xBD, 0xEC, 0xA7, 0xC3, 0x9A,
            0x48, 0x4B, 0xAF, 0x2B, 0x61, 0xBB, 0x39, 0x78,  0xB9, 0xDD, 0x1E, 0xE9, 0xC8, 0x0D, 0xB1, 0x43,
            0xA9, 0x5D, 0x4B, 0xB6, 0x47, 0x00, 0x45, 0x19,  0x29, 0x5E, 0xFE, 0xAB, 0x76, 0xF6, 0xF3, 0x6A,
            0xC2, 0x94, 0xB4, 0xAD, 0xFA, 0xE5, 0xE7, 0x01,  0x7A, 0xE3, 0x67, 0x48, 0x44, 0x7F, 0x22, 0x91,
            0x66, 0xFF, 0x55, 0x33, 0xB0, 0x86, 0xD6, 0xAA,  0xFF, 0x4D, 0xAF, 0x06, 0x08, 0x00, 0x00, 0xDD,
            0xF4, 0xB2, 0x3E, 0x7B, 0x32, 0x57, 0x14, 0xA1,  0xF9, 0xEB, 0xCF, 0x8B, 0x13, 0x66, 0x1A, 0xDB,
            0x86, 0x37, 0xA5, 0x5C, 0x5E, 0x28, 0x92, 0xA3,  0xE6, 0xD7, 0x90, 0x0F, 0x1F, 0x03, 0x1E, 0x1F,
            0xC1, 0xBC, 0xAE, 0x41, 0x03, 0x11, 0x64, 0xAA,  0x5D, 0x46, 0x5C, 0xA5, 0x06, 0xF8, 0xD4, 0xE0,
            0x26, 0x76, 0xC7, 0xD7, 0xD5, 0x92, 0x0F, 0x77,  0x5B, 0x2C, 0xCC, 0xCD, 0xCD, 0xD0, 0xE0, 0x58,
            0xCE, 0x98, 0x33, 0x79, 0x46, 0xD6, 0x72, 0x4C,  0xB7, 0x1C, 0xBE, 0x8C, 0x84, 0x27, 0x43, 0x79,

            0xA6, 0x10, 0x03, 0x09, 0xC9, 0xD6, 0x90, 0x44,  0x5B, 0x5E, 0xEB, 0x67, 0xA2, 0xD0, 0xB7, 0x61,
            0x8B, 0x9E, 0x2A, 0x51, 0xE9, 0xA2, 0x7C, 0x58,  0x26, 0x35, 0x27, 0x52, 0x05, 0xAF, 0x73, 0xD4,
            0xE1, 0xA0, 0x17, 0xE5, 0x70, 0x28, 0x2F, 0x59,  0x41, 0xFB, 0xA2, 0x51, 0xE0, 0x79, 0x5A, 0xEC,
            0x4B, 0xDF, 0xEE, 0x97, 0x57, 0x08, 0xA8, 0x2B,  0xA9, 0xF2, 0x00, 0xA3, 0x28, 0xDC, 0x07, 0xE3,
            0x47, 0x00, 0x45, 0x3A, 0x22, 0x00, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xB1,  0x20, 0xD6, 0x84, 0x21, 0x82, 0x03, 0xDE, 0x6D,
            0x86, 0xF3, 0xAC, 0x4C, 0x10, 0x5C, 0x6F, 0x85,  0x49, 0x41, 0x9F, 0xCD, 0x1A, 0x4C, 0x9F, 0x7F,
            0xD7, 0xE6, 0x33, 0x89, 0xED, 0xFC, 0x90, 0x77,  0x5F, 0xDA, 0xBA, 0xE8, 0x05, 0x96, 0x6C, 0x03,
            0xD9, 0xBF, 0xD5, 0x3C, 0xCA, 0x9A, 0x6F, 0x05,  0xFA, 0x0D, 0x27, 0x6B, 0xA6, 0x7E, 0xCD, 0x8D,
            0xB7, 0x37, 0x0A, 0x6D, 0x0C, 0x88, 0x2E, 0x41,  0xD3, 0x9A, 0xAF, 0xD8, 0x76, 0x67, 0x8D, 0x6F,
            0xB4, 0x25, 0xB6, 0xA8, 0xAC, 0x43, 0x52, 0x09,  0xBE, 0xBE, 0xA5, 0x1F, 0x55, 0x8B, 0xF9, 0x32,
            0xA0, 0xC7, 0x43, 0x55, 0x0D, 0x84, 0x6F, 0xF4,  0xD9, 0x6E, 0xC4, 0xE2, 0xFE, 0xA3, 0x49, 0x13,
            0x70, 0xC8, 0xD4, 0x5B, 0xAE, 0x8A, 0xA8, 0xD1,  0xBA, 0x9B, 0x3D, 0xDA, 0x05, 0x1B, 0xA0, 0xFC,
            0x25, 0x1D, 0xEB, 0xD8, 0x14, 0x74, 0xD6, 0x2A,  0xA9, 0x99, 0x47, 0x59, 0x7D, 0xFC, 0x26, 0xF6,
            0x96, 0xD0, 0x71, 0x7D, 0x2D, 0x0B, 0x55, 0xDE,  0x51, 0x1C, 0xA7, 0x80, 0x47, 0x40, 0x00, 0x3A,

            0xA6, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,  0x00, 0xB0, 0x0D, 0xEA, 0xD4, 0xF9, 0x00, 0x00,
            0x00, 0x01, 0xE0, 0x42, 0xFC, 0x60, 0x82, 0x3A,  0x47, 0x40, 0x42, 0x3A, 0x90, 0x00, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x02, 0xB0,
            0x23, 0x00, 0x01, 0xF3, 0x00, 0x00, 0xE0, 0x45,  0xF0, 0x00, 0x03, 0xE0, 0x44, 0xF0, 0x06, 0x0A,
            0x04, 0x65, 0x6E, 0x67, 0x00, 0x1B, 0xE0, 0x45,  0xF0, 0x06, 0x0A, 0x04, 0x65, 0x6E, 0x67, 0x00,
            0xDA, 0xA4, 0x80, 0x10, 0x47, 0x40, 0x45, 0x1B,  0x00, 0x00, 0x01, 0xE0, 0x08, 0x38, 0x80, 0xC0,
            0x0A, 0x33, 0x98, 0xB7, 0xEF, 0x71, 0x13, 0x98,  0xB7, 0xB7, 0x31, 0x00, 0x00, 0x00, 0x01, 0x09,
            0xE0, 0x00, 0x00, 0x00, 0x01, 0x41, 0x9F, 0x06,  0x42, 0x12, 0xFF, 0xF7, 0xA1, 0x2F, 0x45, 0xC1,
            0xBC, 0x6F, 0xCE, 0x62, 0x3F, 0xBD, 0xF6, 0x2E,  0x98, 0x69, 0x52, 0x4A, 0xA6, 0xDF, 0xEA, 0xC5,
            0x70, 0x3F, 0xF8, 0x94, 0x01, 0xF7, 0x07, 0x0F,  0x6C, 0xC5, 0x4B, 0x16, 0x7A, 0xA8, 0xEE, 0xF4,
            0x4B, 0x16, 0x8B, 0x66, 0x1B, 0x90, 0xBB, 0xFA,  0xCF, 0x5D, 0xBD, 0x63, 0x30, 0x58, 0x12, 0x62,
            0x2E, 0xA4, 0xA6, 0xBB, 0xC2, 0xB0, 0xFF, 0xFD,  0xE3, 0xB2, 0xA9, 0x8E, 0xC1, 0xE3, 0x49, 0x40,
            0x66, 0x06, 0xDB, 0xCE, 0x77, 0xD0, 0x32, 0xAE,  0xC3, 0x61, 0x13, 0xF1, 0x6E, 0x8B, 0xC8, 0x4A,
            0xE0, 0x64, 0x82, 0x7A, 0x91, 0xEC, 0xC4, 0x0C,  0xAD, 0xFD, 0x3F, 0xB6, 0x86, 0x8F, 0xC7, 0x5A,

            0xF5, 0xEE, 0x3D, 0x0D, 0x89, 0x24, 0x83, 0x8B,  0x65, 0xC6, 0x5B, 0x09, 0xAA, 0xE4, 0x2E, 0x52,
            0xE7, 0x91, 0x31, 0x1F, 0x53, 0x2D, 0x59, 0x33,  0x9A, 0x6D, 0xD4, 0x2C, 0xE1, 0xE8, 0x62, 0x6B,
            0x08, 0x37, 0x91, 0xA4, 0xAA, 0x17, 0xF0, 0x99,  0x44, 0x3F, 0x71, 0x25, 0x3C, 0x6E, 0xDF, 0x67
        };

        /* #11852 - RTP packet from A1 TV - small with RTP header extensions */
        unsigned char packet_data1[216] = {
            0x90, 0x21, 0x70, 0x40, 0x5B, 0xBA, 0x12, 0x0E,  0x00, 0x00, 0x00, 0x01, 0xBE, 0xDE, 0x00, 0x03,
            0x13, 0x45, 0xF0, 0x00, 0x46, 0x12, 0x74, 0x00,  0x20, 0x01, 0x59, 0xEF, 0x47, 0x00, 0x20, 0x32,
            0x61, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xDF, 0x13, 0x35, 0x92, 0xB5, 0x1F,  0x07, 0x16, 0x87, 0x07, 0x60, 0x52, 0xF8, 0x55,
            0x82, 0x8D, 0xBF, 0xDA, 0x73, 0x75, 0xC0, 0xC0,  0x34, 0xA7, 0x56, 0x60, 0xA4, 0x9D, 0x05, 0xC5,
            0x5B, 0xC0, 0xEA, 0xC1, 0xD3, 0x0B, 0x20, 0xDC,  0x2E, 0x99, 0x62, 0x75, 0xFA, 0x56, 0x49, 0x65,
            0x63, 0x5A, 0x80, 0xFE, 0x20, 0x95, 0xBA, 0xC8,  0x11, 0x4C, 0x4A, 0x4E, 0x9F, 0x36, 0xD6, 0xC0,
            0x0A, 0x18, 0xA5, 0xB0, 0x38, 0xF9, 0xA2, 0x02,  0x64, 0x57, 0x49, 0x30, 0xD9, 0x0B, 0xA1, 0x30,
            0xFF, 0xC0, 0x12, 0xE8, 0x46, 0x98, 0x16, 0x4B
        };

        /* #11852 - RTP packet from A1 TV - big with RTP header extensions */
        unsigned char packet_data2[1348] = {
            0x90, 0x21, 0x70, 0xDE, 0x5B, 0xBA, 0xC9, 0xC6,  0x00, 0x00, 0x00, 0x01, 0xBE, 0xDE, 0x00, 0x04,
            0x13, 0x87, 0xF0, 0x00, 0x72, 0x01, 0x6C, 0x16,  0x46, 0x13, 0x12, 0x02, 0x02, 0x00, 0x20, 0x01,
            0x47, 0x40, 0x00, 0x1A, 0x00, 0x00, 0xB0, 0x0D,  0x00, 0x01, 0xC1, 0x00, 0x00, 0x00, 0x01, 0xE0,
            0x2A, 0x8D, 0x49, 0xFF, 0x97, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0x47, 0x40, 0x2A, 0x18,
            0x00, 0x02, 0xB0, 0x3E, 0x00, 0x01, 0xC7, 0x00,  0x00, 0xE0, 0x20, 0xF0, 0x00, 0x1B, 0xE0, 0x20,
            0xF0, 0x04, 0x2A, 0x02, 0x7E, 0x1F, 0x06, 0xE0,  0x21, 0xF0, 0x09, 0x6A, 0x01, 0x00, 0x0A, 0x04,

            0x64, 0x65, 0x75, 0x00, 0x03, 0xE0, 0x22, 0xF0,  0x06, 0x0A, 0x04, 0x6F, 0x6C, 0x61, 0x00, 0x06,
            0xE0, 0x29, 0xF0, 0x0A, 0x52, 0x01, 0x04, 0x56,  0x05, 0x67, 0x65, 0x72, 0x09, 0x00, 0x3F, 0xE9,
            0xC3, 0xC9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  0x47, 0x00, 0x20, 0x22, 0xB7, 0x10, 0xAD, 0xDD,
            0x64, 0xE2, 0xFE, 0x49, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x47, 0x40, 0x20, 0x33,  0x10, 0x72, 0xAD, 0xDD, 0x64, 0xE3, 0x7E, 0xAD,
            0x08, 0x02, 0x06, 0x58, 0xC8, 0xAD, 0xDE, 0x1A,  0xEE, 0x00, 0x00, 0x01, 0xE0, 0xDE, 0x08, 0x84,
            0xD0, 0x0F, 0x3B, 0x6E, 0xF1, 0x6B, 0xB9, 0x1B,  0x6E, 0xF1, 0x33, 0x79, 0x80, 0x25, 0x9F, 0xFF,
            0xFF, 0x00, 0x00, 0x00, 0x01, 0x09, 0x10, 0x00,  0x00, 0x00, 0x01, 0x67, 0x4D, 0x40, 0x1E, 0x96,
            0x52, 0x81, 0x08, 0x49, 0xB0, 0xC4, 0xE0, 0xA0,  0xA0, 0xA8, 0x00, 0x00, 0x03, 0x00, 0x08, 0x00,
            0x00, 0x03, 0x01, 0x94, 0xA0, 0x00, 0x00, 0x00,  0x01, 0x68, 0xEF, 0x38, 0x80, 0x00, 0x00, 0x01,
            0x06, 0x01, 0x01, 0x32, 0x04, 0x09, 0xB5, 0x00,  0x31, 0x44, 0x54, 0x47, 0x31, 0x41, 0xF8, 0x06,
            0x01, 0x84, 0x80, 0x00, 0x00, 0x00, 0x01, 0x65,  0x88, 0x80, 0x18, 0x00, 0x37, 0xE5, 0xC4, 0xB9,
            0x32, 0xDF, 0xA6, 0x9F, 0x81, 0x47, 0x6B, 0xD1,  0xB5, 0xCE, 0x9D, 0x8E, 0x9E, 0x65, 0xF3, 0x26,
            0x13, 0x2A, 0x2D, 0xC8, 0x20, 0xFC, 0x91, 0x54,  0x64, 0xA0, 0x15, 0x09, 0xAD, 0x1C, 0x17, 0x0E,
            0xB7, 0xD2, 0xF3, 0xC0, 0xC6, 0xF4, 0x50, 0x92,  0xD6, 0xC8, 0xFD, 0xE9, 0x18, 0x05, 0xA0, 0x68,

            0xAD, 0x0C, 0x9D, 0xDA, 0x27, 0xF7, 0x69, 0x21,  0xCA, 0x3B, 0xD0, 0x35, 0x91, 0xA9, 0xCC, 0xDD,
            0x47, 0x00, 0x20, 0x14, 0x54, 0x1F, 0x41, 0x1E,  0x54, 0x5F, 0x68, 0x68, 0x6B, 0xEC, 0x34, 0x1C,
            0x3C, 0x29, 0x1C, 0xA6, 0xF6, 0xA8, 0xF4, 0xC9,  0x6F, 0x9B, 0x69, 0x89, 0x76, 0x6F, 0xC9, 0x7E,
            0x8F, 0x4F, 0xBB, 0xEA, 0x61, 0x90, 0xAB, 0x43,  0x3D, 0xC1, 0xA6, 0xDE, 0x0F, 0x8B, 0xF0, 0xB7,
            0x44, 0x98, 0x39, 0x37, 0x44, 0xA1, 0x61, 0x98,  0x95, 0xF8, 0x7A, 0x76, 0xE5, 0x02, 0x43, 0xD6,
            0xA1, 0x93, 0x71, 0x93, 0xE4, 0x7F, 0x1A, 0xFA,  0xB5, 0x44, 0xED, 0xC3, 0xEB, 0x88, 0x8D, 0x89,
            0xBF, 0x46, 0x2F, 0xD8, 0xAD, 0x23, 0x0B, 0xF3,  0x0B, 0x11, 0x17, 0x5E, 0xE3, 0x64, 0xD5, 0xDF,
            0x9E, 0xB5, 0xA4, 0x8F, 0x0E, 0x9B, 0x5C, 0xFB,  0xAD, 0x44, 0x1D, 0xF9, 0x81, 0xA0, 0x1F, 0x2C,
            0xA7, 0xEC, 0x3D, 0xC6, 0x1D, 0x2F, 0xB1, 0x5D,  0x08, 0xC9, 0xCA, 0x14, 0xE0, 0xEC, 0xC0, 0xC4,
            0xE1, 0xAF, 0x2A, 0xB9, 0x51, 0x92, 0x5F, 0x24,  0xCD, 0x32, 0xE7, 0x88, 0xFB, 0xC1, 0xCE, 0xE3,
            0xC4, 0xE2, 0x1E, 0x32, 0x45, 0xEE, 0x5E, 0xB3,  0x40, 0x10, 0x4B, 0xD7, 0x5E, 0x1A, 0xE2, 0x70,
            0x0C, 0xCE, 0x0B, 0x1E, 0x31, 0x0E, 0x5A, 0xDA,  0x0B, 0xF1, 0x00, 0x29, 0x7C, 0x91, 0x97, 0x58,
            0x58, 0x8B, 0xF8, 0xE6, 0x4B, 0x54, 0x38, 0xCD,  0xCA, 0x4E, 0x6D, 0x97, 0x47, 0x00, 0x20, 0x15,
            0x90, 0xCE, 0x1C, 0x54, 0x54, 0x59, 0xD3, 0x94,  0x92, 0x08, 0xE6, 0x73, 0x50, 0x5E, 0x4A, 0xA4,
            0x8D, 0x3F, 0x34, 0xDF, 0xFE, 0x2A, 0x72, 0x4A,  0x72, 0x1A, 0x5B, 0xD8, 0xA3, 0x8C, 0x37, 0xFD,
            0xEF, 0x9D, 0x9B, 0x05, 0xC0, 0x7F, 0x5E, 0xDF,  0x6F, 0xCE, 0x03, 0xD8, 0x3E, 0x03, 0xE6, 0x11,

            0xAC, 0xDD, 0xCE, 0xF0, 0x8D, 0x77, 0xE9, 0xE4,  0x14, 0xF3, 0x1B, 0x68, 0x8E, 0x87, 0xBE, 0x14,
            0x8A, 0x19, 0x3B, 0x7D, 0x50, 0x68, 0xDC, 0xB4,  0xB8, 0xFF, 0x27, 0xD4, 0x21, 0x4F, 0x14, 0xF0,
            0xF0, 0x18, 0x14, 0x4F, 0x6B, 0x63, 0xF7, 0x71,  0x06, 0x33, 0x55, 0x36, 0x12, 0x7E, 0x12, 0xA5,
            0x29, 0xB9, 0x4D, 0x3F, 0x94, 0x4A, 0xE5, 0x92,  0x44, 0x0D, 0xC1, 0xDF, 0x88, 0xC0, 0x14, 0x17,
            0xAF, 0xDC, 0x9F, 0x68, 0x95, 0xC5, 0xE3, 0x09,  0xA3, 0x0F, 0xB4, 0xBC, 0x3C, 0x44, 0x8D, 0xE5,
            0xF7, 0xCE, 0x88, 0xED, 0xCE, 0xA8, 0xBA, 0xEC,  0x68, 0x4D, 0x19, 0x51, 0xA5, 0xD4, 0xAD, 0xDE,
            0x1C, 0x95, 0xE7, 0xED, 0x7C, 0x35, 0xE9, 0xEE,  0xA0, 0x94, 0xE9, 0xDF, 0x8F, 0xEA, 0x52, 0xEA,
            0x93, 0xE9, 0x7A, 0x02, 0xE5, 0x82, 0xA5, 0x88,  0x98, 0x83, 0xD6, 0x71, 0xEA, 0xB1, 0x46, 0x39,
            0x50, 0x8F, 0x7C, 0x08, 0x6E, 0xFE, 0x98, 0xC2,  0x47, 0x00, 0x20, 0x16, 0xAE, 0x04, 0x44, 0x5E,
            0x3E, 0x87, 0x28, 0x97, 0x73, 0x69, 0xAF, 0xD9,  0x77, 0x6B, 0xD2, 0x95, 0xE9, 0xC7, 0x33, 0x9F,
            0xCC, 0x80, 0xAA, 0xB7, 0x9A, 0xA7, 0x0A, 0xB0,  0x5F, 0xD6, 0xE1, 0xE1, 0x50, 0xAA, 0xBC, 0x18,
            0xB8, 0x7B, 0xA8, 0x93, 0x37, 0xD8, 0xE4, 0xC7,  0x5E, 0xA7, 0x74, 0x9E, 0xFD, 0x5E, 0x09, 0x62,
            0x86, 0xF4, 0x63, 0x2C, 0xC5, 0xED, 0x6E, 0xBB,  0xA4, 0x95, 0x94, 0x9A, 0x32, 0xED, 0x8E, 0x80,
            0x99, 0x39, 0xA8, 0xEF, 0xCC, 0x40, 0x96, 0xAB,  0x8D, 0x92, 0x27, 0xF0, 0x4D, 0x08, 0x40, 0x7C,
            0xB8, 0xD9, 0xDB, 0xCE, 0xCA, 0x5A, 0xE7, 0x54,  0x19, 0x9A, 0x48, 0x06, 0xCF, 0xAB, 0x4B, 0x48,
            0x91, 0xAF, 0x7E, 0xA5, 0xF7, 0xF0, 0x2D, 0x38,  0xD5, 0x22, 0x71, 0x6D, 0xBE, 0xFE, 0x9A, 0x3A,

            0xC6, 0x72, 0x1F, 0x73, 0xD6, 0x37, 0x90, 0x7E,  0x15, 0x0B, 0x20, 0x7D, 0x2E, 0x63, 0x95, 0x0E,
            0xC2, 0xC2, 0x00, 0x7C, 0xB3, 0xAE, 0x66, 0xB7,  0x41, 0xF0, 0xA0, 0xB0, 0xBF, 0x66, 0xED, 0xC8,
            0x78, 0x33, 0xD6, 0x3E, 0x89, 0x95, 0x99, 0x12,  0xD8, 0xD8, 0x8B, 0xC7, 0xD2, 0x79, 0xA8, 0x4F,
            0x56, 0x6E, 0x54, 0x74, 0xDB, 0x34, 0x5A, 0xA1,  0xB3, 0x13, 0x8B, 0x62, 0xF1, 0xAB, 0x4C, 0x12,
            0x90, 0x20, 0x1D, 0xF9
        };


        /* regression test of working packet */
        RTPDataPacket packet0;
        packet0.GetDataReference().append((char*)packet_data0, sizeof(packet_data0));
        QVERIFY (packet0.IsValid());
        RTPTSDataPacket ts_packet0(packet0);
        QCOMPARE (ts_packet0.GetTSData()[0], (uint8_t)0x47);
        QCOMPARE (ts_packet0.GetTSDataSize(), (unsigned int)7 * 188);


        /* test of short packet with header extension */
        RTPDataPacket packet1;
        packet1.GetDataReference().append((char*)packet_data1, sizeof(packet_data1));
        QVERIFY (packet1.IsValid());
        QCOMPARE (packet1.GetDataReference().size(), 216);
        QCOMPARE (packet1.GetPaddingSize(), (unsigned int)0);
        QCOMPARE (packet1.GetPayloadOffset(), (unsigned int)12+4+3*4);
        RTPTSDataPacket ts_packet1(packet1);
        QCOMPARE (ts_packet1.GetTSData()[0], (uint8_t)0x47);
        QCOMPARE (ts_packet1.GetTSDataSize(), (unsigned int)1 * 188);


        /* test with header extension */
        RTPDataPacket packet2;
        packet2.GetDataReference().append((char*)packet_data2, sizeof(packet_data2));
        QVERIFY (packet2.IsValid());
        RTPTSDataPacket ts_packet2(packet2);
        QCOMPARE (ts_packet2.GetTSData()[0], (uint8_t)0x47);
        QCOMPARE (ts_packet2.GetTSDataSize(), (unsigned int)7 * 188);
    }
};