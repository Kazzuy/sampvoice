// Calls every native of documents/sampvoice.inc and uses every SV_FILTER value.
// Compile: pawncc tests/pawn/svcoverage.pwn -i<folder with sampvoice.inc>
#include <a_samp>
#include <sampvoice>

main() {}

public OnGameModeInit()
{
    new SV_UINT:stream = SvCreateStream(25.0);
    new SV_UINT:effect = SvCreateEffect();

    // Debug
    SvEnableDebug();
    SvCheckDebug();
    SvDisableDebug();

    // Stream
    SvEnableTransiter(stream);
    SvCheckTransiter(stream);
    SvDisableTransiter(stream);
    SvSetVolume(stream, 1.0);
    SvSetPanning(stream, 0.0);
    SvSetDistance(stream, 25.0);
    SvSetPosition(stream, 0.0, 0.0, 0.0);
    SvSetTarget(stream, 0);
    SvSetEffect(stream, effect);
    SvSetIcon(stream, "mic");

    // Effect: every SV_FILTER member
    SvAppendFilter(effect, SV_CHORUS, 0);
    SvAppendFilter(effect, SV_COMPRESSOR, 1);
    SvAppendFilter(effect, SV_DISTORTION, 2);
    SvAppendFilter(effect, SV_ECHO, 3);
    SvAppendFilter(effect, SV_FLANGER, 4);
    SvAppendFilter(effect, SV_GARGLE, 5);
    SvAppendFilter(effect, SV_I3DL2REVERB, 6);
    SvAppendFilter(effect, SV_PARAMEQ, 7);
    SvAppendFilter(effect, SV_REVERB, 8);
    SvRemoveFilter(effect, SV_CHORUS, 0);
    SvRemoveFilter(effect, SV_COMPRESSOR, 1);
    SvRemoveFilter(effect, SV_DISTORTION, 2);
    SvRemoveFilter(effect, SV_ECHO, 3);
    SvRemoveFilter(effect, SV_FLANGER, 4);
    SvRemoveFilter(effect, SV_GARGLE, 5);
    SvRemoveFilter(effect, SV_I3DL2REVERB, 6);
    SvRemoveFilter(effect, SV_PARAMEQ, 7);
    SvRemoveFilter(effect, SV_REVERB, 8);

    // Player natives (player 0 may be absent: the natives return false)
    SvGetVersion(0);
    SvHasMicro(0);
    SvEnableListener(0);
    SvCheckListener(0);
    SvDisableListener(0);
    SvEnableSpeaker(0, 0);
    SvCheckSpeaker(0, 0);
    SvDisableSpeaker(0, 0);
    SvAttachStream(0, stream, 0);
    SvHasStream(0, stream, 0);
    SvDetachStream(0, stream, 0);
    SvSetKey(0, 0x42, 0);
    SvGetKey(0, 0);
    SvPlay(0, 0);
    SvStop(0, 0);
    SvAttachListener(stream, 0);
    SvHasListener(stream, 0);
    SvDetachListener(stream, 0);

    SvDeleteEffect(effect);
    SvDeleteStream(stream);
    print("[svcoverage] all natives called");
    return 1;
}
