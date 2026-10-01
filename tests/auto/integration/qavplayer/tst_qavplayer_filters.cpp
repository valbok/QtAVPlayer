/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "tst_qavplayer.h"
#include "qavplayer.h"

#include <QtTest/QtTest>
#include <QDebug>

void tst_QAVPlayer::configureFilter()
{
    QAVPlayer p;

    QFileInfo file(testData("small.mp4"));
    p.setInputVideoCodec("software");
    p.setSource(file.absoluteFilePath());
    QSignalSpy spy(&p, &QAVPlayer::filtersChanged);
    QSignalSpy spyErrorOccurred(&p, &QAVPlayer::errorOccurred);
    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; });

    p.pause();
    QTRY_VERIFY(frame);
    QCOMPARE(frame.size(), QSize(560, 320));

    frame = QAVVideoFrame();

    QString desc = "scale=iw/2:-1";
    p.setFilter(desc);
    QCOMPARE(p.filters(), {desc});
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(spyErrorOccurred.count(), 0);

    p.play();
    QTRY_VERIFY(frame);
    QTRY_COMPARE(frame.size(), QSize(560 / 2, 320 / 2));

    spy.clear();
    spyErrorOccurred.clear();

    p.stop();
    p.setFilter(desc);
    QCOMPARE(p.filters(), {desc});
    QCOMPARE(spy.count(), 0);
    QCOMPARE(spyErrorOccurred.count(), 0);

    p.setFilter("");
    QCOMPARE(p.filters(), {});
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(spyErrorOccurred.count(), 0);

    frame = QAVVideoFrame();

    p.pause();
    QTRY_VERIFY(frame);

    frame = QAVVideoFrame();

    p.play();
    QTRY_VERIFY(frame);
    QTRY_COMPARE(frame.size(), QSize(560, 320));

    p.stop();
    p.setFilter("wrong");
    QCOMPARE(p.filters(), {"wrong"});
    QTRY_COMPARE(spyErrorOccurred.count(), 0);
    QTRY_VERIFY(frame);

    frame = QAVVideoFrame();
    spyErrorOccurred.clear();

    p.pause();
    QTRY_COMPARE(spyErrorOccurred.count(), 1);

    frame = QAVVideoFrame();
    spyErrorOccurred.clear();

    p.pause();
    QCOMPARE(p.filters(), {"wrong"});
    QTRY_COMPARE(spyErrorOccurred.count(), 1);

    spy.clear();
    spyErrorOccurred.clear();

    p.setFilter(desc);
    QCOMPARE(p.filters(), {desc});
    QTRY_COMPARE(spy.count(), 1);

    p.pause();
    QTRY_VERIFY(frame);
    QCOMPARE(spyErrorOccurred.count(), 0);

    spy.clear();
    spyErrorOccurred.clear();

    p.setFilter("wrong");
    QCOMPARE(p.filters(), {"wrong"});
    QTRY_COMPARE(spy.count(), 1);
    QTRY_COMPARE(spyErrorOccurred.count(), 0);
    QCOMPARE(p.state(), QAVPlayer::PausedState);

    spyErrorOccurred.clear();

    p.pause();
    QTRY_COMPARE(spyErrorOccurred.count(), 0);

    spyErrorOccurred.clear();

    QSignalSpy spyStepped(&p, &QAVPlayer::stepped);
    p.stepForward();
    QTRY_VERIFY2(spyErrorOccurred.count() == 1 || spyStepped.count() == 1,
                 "Expected either filter error or stepped signal after stepForward()");

    spy.clear();
    spyErrorOccurred.clear();

    p.setFilter("wrong2");
    QCOMPARE(p.filters(), {"wrong2"});
    QTRY_COMPARE(spy.count(), 1);
    QTRY_COMPARE(spyErrorOccurred.count(), 0);

    spyErrorOccurred.clear();

    p.stop();
    QCOMPARE(spyErrorOccurred.count(), 0);

    spyErrorOccurred.clear();

    p.play();
    QTRY_COMPARE(spyErrorOccurred.count(), 1);

    spy.clear();
    spyErrorOccurred.clear();
    frame = QAVVideoFrame();

    p.setFilter("");
    QCOMPARE(p.filters(), {});
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(spyErrorOccurred.count(), 0);

    p.play();
    QTRY_VERIFY(frame);
    QTRY_COMPARE(frame.size(), QSize(560, 320));
    QCOMPARE(spyErrorOccurred.count(), 0);

    p.setFilter(desc);
    QTRY_COMPARE(frame.size(), QSize(560 / 2, 320 / 2));

    p.seek(p.duration());
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);
}

void tst_QAVPlayer::changeSourceFilter()
{
    QAVPlayer p;

    QFileInfo file1(testData("small.mp4"));
    p.setInputVideoCodec("software");
    QSignalSpy spy(&p, &QAVPlayer::filtersChanged);
    QSignalSpy spyErrorOccurred(&p, &QAVPlayer::errorOccurred);
    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; });

    const QString desc = "scale=iw/2:-1";
    p.setFilter(desc);
    p.setSource(file1.absoluteFilePath());
    p.play();
    QTRY_VERIFY(frame);
    QCOMPARE(frame.size(), QSize(560 / 2, 320 / 2));

    QFileInfo file2(testData("colors.mp4"));
    p.setSource(file2.absoluteFilePath());

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.filters(), {desc});

    frame = QAVVideoFrame();

    p.play();
    QTRY_COMPARE(frame.size(), QSize(160 / 2, 120 / 2));

    p.setSource(file1.absoluteFilePath());
    p.play();
    QTRY_COMPARE(frame.size(), QSize(560 / 2, 320 / 2));
}

void tst_QAVPlayer::filter_data()
{
    QTest::addColumn<QString>("filter");

    QTest::newRow("fps=fps=10") << QString("fps=fps=10");
    QTest::newRow("curves=vintage") << QString("curves=vintage");
    QTest::newRow("scale=1920:1080,setsar=1:1") << QString("scale=1920:1080,setsar=1:1");
    QTest::newRow("mirror") << QString("crop=iw/2:ih:0:0,split[left][tmp];[tmp]hflip[right];[left][right] hstack");
    QTest::newRow("split") << QString("split=4[a][b][c][d];[b]lutrgb=g=0:b=0[x];[c]lutrgb=r=0:b=0[y];[d]lutrgb=r=0:g=0[z];[a][x][y][z]hstack=4");
    QTest::newRow("yuv") << QString("split=4[a][b][c][d];[b]lutyuv=u=128:v=128[x];[c]lutyuv=y=0:v=128[y];[d]lutyuv=y=0:u=128[z];[a][x][y][z]hstack=4");
    QTest::newRow("histogram") << QString("format=gbrp,split=4[a][b][c][d],[d]histogram=display_mode=0:level_height=244[dd],[a]waveform=m=1:d=0:r=0:c=7[aa],[b]waveform=m=0:d=0:r=0:c=7[bb],[c][aa]vstack[V],[bb][dd]vstack[V2],[V][V2]hstack");
    QTest::newRow("vectorscope") << QString("format=yuv422p,split=4[a][b][c][d],[a]waveform[aa],[b][aa]vstack[V],[c]waveform=m=0[cc],[d]vectorscope=color4[dd],[cc][dd]vstack[V2],[V][V2]hstack");
    //QTest::newRow("waveform") << QString("split[a][b];[a]format=gray,waveform,split[c][d];[b]pad=iw:ih+256[padded];[c]geq=g=1:b=1[red];[d]geq=r=1:b=1,crop=in_w:220:0:16[mid];[red][mid]overlay=0:16[wave];[padded][wave]overlay=0:H-h");
    QTest::newRow("envelope") << QString("split[a][b];[a]waveform=e=3,split=3[c][d][e];[e]crop=in_w:20:0:235,lutyuv=v=180[low];[c]crop=in_w:16:0:0,lutyuv=y=val:v=180[high];[d]crop=in_w:220:0:16,lutyuv=v=110[mid] ; [b][high][mid][low]vstack=4");
    QTest::newRow("yuv420p10le") << QString("format=yuv420p10le|yuv422p10le|yuv444p10le|yuv440p10le,            lutyuv=                y=if(eq(1\\,-1)\\,512\\,if(eq(1\\,0)\\,val\\,bitand(val\\,pow(2\\,10-1))*pow(2\\,1))):                u=if(eq(-1\\,-1)\\,512\\,if(eq(-1\\,0)\\,val\\,bitand(val\\,pow(2\\,10--1))*pow(2\\,-1))):                v=if(eq(-1\\,-1)\\,512\\,if(eq(-1\\,0)\\,val\\,bitand(val\\,pow(2\\,10--1))*pow(2\\,-1))),format=yuv444p");
    QTest::newRow("bitplanenoise") << QString("bitplanenoise=bitplane=1:filter=1,extractplanes=y,format=yuv444p");
    QTest::newRow("lutyuv") << QString("lutyuv=y=if(gt(val\\,maxval)\\,val-maxval\\,0):u=(maxval+minval)/2:v=(maxval+minval)/2,histeq=strength=1");
    QTest::newRow("signalstats") << QString("signalstats=out=brng:c=0x40e0d0,format=yuv444p|rgb24");
    QTest::newRow("ciescope") << QString("ciescope=system=1:gamuts=pow(2\\,1):contrast=0.7:intensity=0.01");
    QTest::newRow("extractplanes") << QString("format=yuv444p,split[y][u];[y]extractplanes=y,pad=w=iw+256:h=ih:x=128,format=yuv444p[y1];[u]extractplanes=u,histeq,pad=w=iw+256:h=ih:x=0+128:y=0,format=yuv444p[u1];[y1][u1]vstack,il=l=i:c=i");
    QTest::newRow("crop") << QString("split[a][b];[a]crop=360:576:0:0[a1];[b]colormatrix=bt601:bt709[b1];[b1][a1]overlay");
    QTest::newRow("crop_histeq") << QString("split=4[a][b][c][d];[a]crop=w=24:h=24:x=0:y=0,histeq=strength=0,scale=24*16:24*16:flags=neighbor,drawgrid=w=iw/24:h=ih/24:t=1:c=green@0.5[a1];[b]crop=w=24:h=24:x=iw-24:y=0,histeq=strength=0,scale=24*16:24*16:flags=neighbor,drawgrid=w=iw/24:h=ih/24:t=1:c=green@0.5[b1];[c]crop=w=24:h=24:x=0:y=ih-24,histeq=strength=0,scale=24*16:24*16:flags=neighbor,drawgrid=w=iw/24:h=ih/24:t=1:c=green@0.5[c1];[d]crop=w=24:h=24:x=iw-24:y=ih-24,histeq=strength=0,scale=24*16:24*16:flags=neighbor,drawgrid=w=iw/24:h=ih/24:t=1:c=green@0.5[d1];[a1][b1]hstack[ab];[c1][d1]hstack[cd];[ab][cd]vstack,setsar=1/1,drawgrid=w=iw/2:h=ih/2:t=2:c=blue@0.5");
    QTest::newRow("datascope") << QString("datascope=x=0:y=0:mode=1:axis=1");
    QTest::newRow("extractplanes_formats") << QString("format=yuv444p|yuv422p|yuv420p|yuv410p,extractplanes=v,histeq=strength=0.2:intensity=0.2");
    QTest::newRow("bottom_blend") << QString("split[a][b];[a]field=bottom[a1];[b]field=top,negate[b2];[a1][b2]blend=all_mode=average,histeq=strength=0:intensity=0");
    QTest::newRow("force_original_aspect_ratio") << QString("scale=iw/8:ih/4:force_original_aspect_ratio=decrease,tile=8x4:overlap=8*4-1:init_padding=8*4-1");
    QTest::newRow("histogram_linear") << QString("histogram=level_height=576:levels_mode=linear");
    //QTest::newRow("thistogram") << QString("thistogram=levels_mode=linear"); does not exist in old ffmpeg version
    QTest::newRow("shuffleplanes") << QString("limiter=min=0:max=255:planes=1,shuffleplanes=1-1,histeq=strength=0.2,format=gray,format=yuv444p");
    QTest::newRow("crop_overlap") << QString("format=rgb24|yuv444p,crop=iw:1:0:288:0:1,tile=1x480:overlap=480-1:init_padding=480-1");
    QTest::newRow("crop_mirror") << QString("crop=iw:1:0:288:0:1,waveform=intensity=1:mode=column:mirror=1:components=7:display=overlay:graticule=green:flags=numbers+dots:scale=0");
    QTest::newRow("oscilloscope") << QString("oscilloscope=x=500000/1000000:y=500000/1000000:s=500000/1000000:t=500000/1000000");
    QTest::newRow("geq") << QString("geq=lum=lum(X\\,Y)-lum(X-1\\,Y-0)+128:cb=cb(X\\,Y)-cb(X-0\\,Y-0)+128:cr=cr(X\\,Y)-cr(X-0\\,Y-0)+128,histeq=strength=0");
    QTest::newRow("pixscope") << QString("pixscope=x=20/100:y=20/100:w=8:h=8,format=rgb24");
    QTest::newRow("crop_in_range") << QString("split[a][b];[a]crop=100:100:0:0[a1];[b]scale=iw+1:ih:in_range=tv:out_range=full,scale=iw-1:ih[b1];[b1][a1]overlay");
    QTest::newRow("between_hypot") << QString("format=yuv444p,geq=lum=lum(X\\,Y):cb=if(between(hypot(cb(X\\,Y)-128\\,cr(X\\,Y)-128)\\,89\\,182)\\,32\\,128):cr=if(between(hypot(cb(X\\,Y)-128\\,cr(X\\,Y)-128)\\,89\\,182)\\,220\\,128)");
    QTest::newRow("tblend") << QString("tblend=all_mode=difference128,histeq=strength=0.2:intensity=0.2");
    QTest::newRow("signalstats") << QString("format=yuv444p,signalstats=out=tout:c=0x40e0d0");
    QTest::newRow("extractplanes_between") << QString("extractplanes=y,format=rgb24,lutrgb=r=if(between(val\\,235\\,255)\\,64\\,val):g=if(between(val\\,235\\,255)\\,224\\,val):b=if(between(val\\,235\\,255)\\,208\\,val)");
    QTest::newRow("vectorscope_envelope") << QString("vectorscope=i=0.1:mode=3:envelope=0:colorspace=1:graticule=green:flags=name,pad=ih*1.33333:ih:(ow-iw)/2:(oh-ih)/2");
    QTest::newRow("vectorscope_graticule") << QString("split[h][l];[l]vectorscope=i=0.1:mode=3:envelope=0:colorspace=1:graticule=green:flags=name:l=0:h=0.5[l1];[h]vectorscope=i=0.1:mode=3:envelope=0:colorspace=1:graticule=green:flags=name:l=0.5:h=1[h1];[l1][h1]hstack");
    QTest::newRow("lutyuv_drawbox") << QString("split[a][b];            [a]lutyuv=y=val/4,scale=720:576,setsar=1/1,format=yuv444p|yuv444p10le,drawbox=w=120:h=120:x=20:y=20:color=invert:thickness=1[a1];            [b]crop=120:120:20:20,            format=yuv422p|yuv422p10le|yuv420p|yuv411p|yuv444p|yuv444p10le,vectorscope=i=0.1:mode=3:envelope=0:colorspace=601:graticule=green:flags=name,pad=ih*1.33333:ih:(ow-iw)/2:(oh-ih)/2,scale=720:576,setsar=1/1[b1];            [a1][b1]blend=addition");
    QTest::newRow("signalstats_vrep") << QString("format=yuv444p,signalstats=out=vrep:c=0x40e0d0");
    QTest::newRow("waveform_green") << QString("waveform=intensity=0.1:mode=column:mirror=1:c=1:f=0:graticule=green:flags=numbers+dots:scale=0");
    QTest::newRow("lutyuv_drawbox_green") << QString("split[a][b];            [a]lutyuv=y=val/4,scale=720:576,setsar=1/1,format=yuv444p|yuv444p10le,drawbox=w=121:h=121:x=20:y=20:color=invert:thickness=1[a1];            [b]crop=121:121:20:20,            waveform=intensity=0.8:mode=column:mirror=1:c=1:f=0:graticule=green:flags=numbers+dots:scale=0,scale=720:576,setsar=1/1[b1];            [a1][b1]blend=addition");
    QTest::newRow("crop_neighbor") << QString("crop=x=200:y=200:w=120:h=120,scale=720:576:flags=neighbor,histeq=strength=0,setsar=1/1");
    QTest::newRow("pad_negate_edgedetect_overlay") << QString("[0:v]pad=iw*2:ih*2[a];  [1:v]negate[b];  [2:v]hflip[c];  [3:v]edgedetect[d];  [a][b]overlay=w[x];  [x][c]overlay=0:h[y];  [y][d]overlay=w:h[out]");
    //QTest::newRow("waveform_audio_video") << QString("[0:v]waveform[v];[1:a]showwaves=s=640x256[a];[v][a]xstack");
    QTest::newRow("negate_hflip_edgedetect_hstack") << QString("[1:v]negate[a];  [2:v]hflip[b];  [3:v]edgedetect[c];  [0:v][a]hstack=inputs=2[top];  [b][c]hstack=inputs=2[bottom];  [top][bottom]vstack=inputs=2[out]");
    QTest::newRow("crop_black") << QString("crop=in_w-2*150:in_h,pad=980:980:x=0:y=0:color=black");
}

void tst_QAVPlayer::filter()
{
    QFETCH(QString, filter);
    QAVPlayer p;

    QFileInfo file(testData("dv25_pal__411_4-3_2ch_32k_bars_sine.dv"));

    QSignalSpy spyVideoFilterChanged(&p, &QAVPlayer::filtersChanged);
    QSignalSpy spyErrorOccurred(&p, &QAVPlayer::errorOccurred);
    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; });

    p.setFilter(filter);
    p.setSource(file.absoluteFilePath());
    p.pause();
    QTRY_VERIFY_WITH_TIMEOUT(frame, 10000);
    QTRY_COMPARE(spyVideoFilterChanged.count(), 1);

    frame = QAVVideoFrame();

    p.play();
    QTRY_VERIFY(frame);
    QCOMPARE(p.filters(), {filter});
    QCOMPARE(spyErrorOccurred.count(), 0);

    QTest::qWait(100);

    QCOMPARE(spyErrorOccurred.count(), 0);
    p.seek(p.duration());
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);
}

void tst_QAVPlayer::filterName()
{
    QAVPlayer p;
    QFileInfo file(testData("small.mp4"));
    p.setInputVideoCodec("software");
    p.setSource(file.absoluteFilePath());
    p.setFilter("scale=iw/2:-1");
    QSet<QString> set;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &frame) {
        qDebug() << frame.pts() << frame.filterName();
        if (!frame.filterName().isEmpty())
            set.insert(frame.filterName());
    });

    p.play();
    QTRY_COMPARE_WITH_TIMEOUT(set.size(), 1, 30000);
    QVERIFY(set.contains("0:0"));
    set.clear();
    p.setFilter("[0:v]split=2[in1][in2];[in1]boxblur[out1];[in2]negate[out2]");
    QTRY_VERIFY_WITH_TIMEOUT(set.size() >= 2, 30000);
    QTRY_VERIFY(set.contains("out1"));
    QTRY_VERIFY(set.contains("out2"));
    set.clear();
    p.setFilter("[0:v]split=3[in1][in2][in3];[in1]boxblur[out1];[in2]negate[out2];[in3]scale=iw/2:-1[out3]");
    QTRY_VERIFY_WITH_TIMEOUT(set.size() >= 3, 30000);
    QTRY_VERIFY(set.contains("out1"));
    QTRY_VERIFY(set.contains("out2"));
    QTRY_VERIFY(set.contains("out3"));
    set.clear();
    p.setFilters({
            "scale=iw/2:-1[scale]",
            "negate[negate]",
            "[0:v]split=3[in1][in2][in3];[in1]boxblur[out1];[in2]negate[out2];[in3]scale=iw/2:-1[out3]"
        });
    QTRY_VERIFY_WITH_TIMEOUT(set.size() >= 5, 30000);
    QTRY_VERIFY(set.contains("scale"));
    QTRY_VERIFY(set.contains("negate"));
    QTRY_VERIFY(set.contains("out1"));
    QTRY_VERIFY(set.contains("out2"));
    QTRY_VERIFY(set.contains("out3"));
}

void tst_QAVPlayer::filterNameStep()
{
    QAVPlayer p;
    QFileInfo file(testData("small.mp4"));
    p.setInputVideoCodec("software");
    p.setSource(file.absoluteFilePath());
    p.setFilter("[0:v]split=3[in1][in2][in3];[in1]boxblur[out1];[in2]negate[out2];[in3]scale=iw/2:-1[out3]");
    QSet<QString> set;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &frame) {
        qDebug() << frame.pts() << frame.filterName();
        if (!frame.filterName().isEmpty())
            set.insert(frame.filterName());
        ++framesCount;
    });

    p.pause();
    QTRY_VERIFY_WITH_TIMEOUT(set.size() == 3, 5000);
    QTRY_VERIFY(set.contains("out1"));
    QTRY_VERIFY(set.contains("out2"));
    QTRY_VERIFY(set.contains("out3"));
    QCOMPARE(framesCount, 3);

    set.clear();
    framesCount = 0;
    p.setFilters({
            "scale=iw/2:-1[scale]",
            "negate[negate]",
            "[0:v]split=3[in1][in2][in3];[in1]boxblur[out1];[in2]negate[out2];[in3]scale=iw/2:-1[out3]"
        });

    p.stepForward();
    QTRY_VERIFY_WITH_TIMEOUT(set.size() >= 5, 30000);
    QTRY_VERIFY(set.contains("scale"));
    QTRY_VERIFY(set.contains("negate"));
    QTRY_VERIFY(set.contains("out1"));
    QTRY_VERIFY(set.contains("out2"));
    QTRY_VERIFY(set.contains("out3"));
    QVERIFY(framesCount >= 5);

    set.clear();
    framesCount = 0;

    p.stepBackward();
    QTRY_VERIFY_WITH_TIMEOUT(set.size() >= 5, 30000);
    QTRY_VERIFY(set.contains("scale"));
    QTRY_VERIFY(set.contains("negate"));
    QTRY_VERIFY(set.contains("out1"));
    QTRY_VERIFY(set.contains("out2"));
    QTRY_VERIFY(set.contains("out3"));
    QCOMPARE(framesCount, 5);

    set.clear();
    framesCount = 0;

    p.setFilter("");
    p.stepForward();
    QTRY_COMPARE(framesCount, 1);
    QVERIFY(set.isEmpty());
    set.clear();
    framesCount = 0;

    p.stepForward();
    QTRY_COMPARE(framesCount, 1);
    QVERIFY(set.isEmpty());
}

void tst_QAVPlayer::audioVideoFilter()
{
    QAVPlayer p;
    QFileInfo file(testData("test.mkv"));
    p.setInputVideoCodec("software");
    p.setSource(file.absoluteFilePath());
    p.setFilter("ahistogram=dmode=separate:rheight=0:s=360x1:r=32,transpose=2,tile=layout=512x1,format=rgb24 [panel_4]");

    int framesCount = 0;
    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) {
        frame = f;
        ++framesCount;
    }, Qt::DirectConnection);

    p.setSynced(false);
    p.play();
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 15000);
    QCOMPARE(framesCount, 1);
    QCOMPARE(frame.filterName(), "panel_4");
}

void tst_QAVPlayer::audioFilterVideoFrames()
{
    QAVPlayer p;
    QFileInfo file(testData("test.mkv"));
    p.setInputVideoCodec("software");
    p.setSource(file.absoluteFilePath());
    p.setFilter("aformat=sample_fmts=flt|fltp,astats=metadata=1:reset=1:length=0.4,aphasemeter=video=0,ebur128=metadata=1,aformat=sample_fmts=flt|fltp");

    int videoFramesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &) {
        ++videoFramesCount;
    }, Qt::DirectConnection);

    int audioFramesCount = 0;
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&](const QAVAudioFrame &) {
        ++audioFramesCount;
    }, Qt::DirectConnection);

    p.setSynced(false);
    p.play();
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 15000);
    QTRY_VERIFY(videoFramesCount > 0);
    QTRY_VERIFY(audioFramesCount > 0);
}

void tst_QAVPlayer::multipleFilters()
{
    QAVPlayer p;
    QFileInfo file(testData("test.mkv"));
    p.setInputVideoCodec("software");
    p.setSource(file.absoluteFilePath());
    QList<QString> filters = {
        "signalstats=stat=tout+vrep+brng [stats]",
        "aformat=sample_fmts=flt|fltp,astats=metadata=1:reset=1:length=0.4,aphasemeter=video=0,ebur128=metadata=1,aformat=sample_fmts=flt|fltp",
        "scale=72:72,format=rgb24 [thumbnails]",
        //"scale=iw/4:ih/4,format=gray,convolution=0m='0 1 0 1 -4 1 0 1 0':0bias=128,split[a][b];[a]scale=iw:1[a1];[a1][b]scale2ref[a2][b];[b][a2]lut2=c0=((x-y)*(x-y))/2,scale=iw:1,transpose=2,tile=layout=512x1,setsar=1/1,format=rgb24 [panel_0]",
        "scale,format=rgb24,crop=1:ih:iw/2:0,tile=layout=512x1,setsar=1/1 [panel_1]",
        "scale,format=rgb24,transpose=2,crop=1:ih:iw/2:0,tile=layout=512x1,setsar=1/1 [panel_2]",
        "aformat=channel_layouts=stereo:sample_fmts=flt|fltp,ahistogram=dmode=separate:rheight=0:s=360x1:r=32,transpose=2,tile=layout=512x1,format=rgb24 [panel_3]",
        "aformat=channel_layouts=stereo:sample_fmts=flt|fltp,showwaves=mode=p2p:split_channels=1:size=512x360:scale=lin:draw=full:rate=32/512,format=rgb24 [panel_4]",
    };

    QMap<QString, int> framesCount;
    QMutex mutex;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) {
        QMutexLocker locker(&mutex);
        if (!framesCount.contains(f.filterName()))
            qDebug() << "video [" << f.filterName() << "]";
        ++framesCount[f.filterName()];
    }, Qt::DirectConnection);

    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&](const QAVAudioFrame &f) {
        QMutexLocker locker(&mutex);
        if (!framesCount.contains(f.filterName()))
            qDebug() << "audio [" << f.filterName() << "]";
        ++framesCount[f.filterName()];
    }, Qt::DirectConnection);

    p.setSynced(false);
    p.setFilters(filters);
    p.play();
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 15000);
    QTRY_VERIFY(framesCount.contains("stats"));
    QTRY_COMPARE(framesCount["stats"], 250);
    QTRY_VERIFY(framesCount.contains("1:0"));
    QTRY_COMPARE(framesCount["1:0"], 101);
    QTRY_VERIFY(framesCount.contains("thumbnails"));
    QTRY_COMPARE(framesCount["thumbnails"], 250);
    //QTRY_VERIFY(framesCount.contains("panel_0"));
    //QCOMPARE(framesCount["panel_0"], 1);
    QTRY_VERIFY(framesCount.contains("panel_1"));
    QCOMPARE(framesCount["panel_1"], 1);
    QTRY_VERIFY(framesCount.contains("panel_2"));
    QCOMPARE(framesCount["panel_2"], 1);
    QTRY_VERIFY(framesCount.contains("panel_3"));
    QCOMPARE(framesCount["panel_3"], 1);
    QTRY_VERIFY(framesCount.contains("panel_4"));
    QCOMPARE(framesCount["panel_4"], 1);
}

void tst_QAVPlayer::multipleAudioVideoFilters()
{
    QAVPlayer p;
    QFileInfo file(testData("test_5beeps.mkv"));
    p.setInputVideoCodec("software");
    QList<QString> filters = {
        "signalstats=stat=tout+vrep+brng [stats]",
        "aformat=sample_fmts=flt|fltp,astats=metadata=1:reset=1:length=0.4,aphasemeter=video=0,ebur128=metadata=1,aformat=sample_fmts=flt|fltp [audio]",
    };
    p.setFilters(filters);
    p.setSource(file.absoluteFilePath());

    QMap<QString, int> framesCount;
    QAVVideoFrame videoFrame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) {
        videoFrame = f;
        ++framesCount[f.filterName()];
    }, Qt::DirectConnection);

    QAVAudioFrame audioFrame;
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&](const QAVAudioFrame &f) {
        audioFrame = f;
        ++framesCount[f.filterName()];
    }, Qt::DirectConnection);

    p.setSynced(false);
    p.play();
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 15000);
    QVERIFY(framesCount.contains("stats"));
    QTRY_COMPARE(framesCount["stats"], 125);
    QCOMPARE(videoFrame.pts(), 4.96);
    QVERIFY(framesCount.contains("audio"));
    QTRY_COMPARE(framesCount["audio"], 51);
    QVERIFY(audioFrame.pts() < 5.5);
}

void tst_QAVPlayer::flushFilters()
{
    QAVPlayer p;
    QFileInfo file(testData("BAVC1010958_DV000107.dv"));
    p.setInputVideoCodec("software");
    p.setFilter("scale,format=rgb32,crop=1:ih:iw/2:0,tile=layout=512x1,setsar=1/1 [panel_0]");
    p.setSource(file.absoluteFilePath());
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &) {
        ++framesCount;
    });
    p.play();
    p.setSynced(false);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 15000);
    QTRY_COMPARE(framesCount, 1);
}

void tst_QAVPlayer::multiFilterInputs_data()
{
    QTest::addColumn<QString>("filter");

    QTest::newRow("vstack") << QString("sws_flags=neighbor;format=yuv444p,scale[b];showvolume=w=320:h=40:f=0.95:dm=1[a];[a][b]vstack");
    QTest::newRow("xstack") << QString("[0:a:0]abitscope,scale=320x240[z2];[0:v:0]scale=320x240[b];[z2][b]xstack");
}

void tst_QAVPlayer::multiFilterInputs()
{
    QFETCH(QString, filter);
    QAVPlayer p;
    QFileInfo file(testData("av_sample.mkv"));
    int framesCount = 0;
    QAVFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; }, Qt::DirectConnection);

    p.setFilter(filter);
    p.setSource(file.absoluteFilePath());
    p.setSynced(false);
    p.play();

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QVERIFY(frame);
    QVERIFY(frame.stream());
    QCOMPARE(framesCount, 250);
    auto s = p.currentVideoStreams().first();
    QCOMPARE(framesCount, s.framesCount());
    QCOMPARE(framesCount, p.progress(s).framesCount());
    QCOMPARE(framesCount, p.progress(s).expectedFramesCount());
    QVERIFY(p.progress(s).pts() > 0);
    QVERIFY(p.progress(s).fps() > 0.0);
    QVERIFY(p.progress(s).frameRate() > 0.0);
    QVERIFY(p.progress(s).expectedFrameRate() > 0.0);
}
