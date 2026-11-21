// -----------------------------------------------------------------------------
// NimbyTapes - wave accessor (Plan 13, Architecture, Wave accessor).
// Purpose : EffectSound keeps its wave protected (m_SoundWaveObject,
//           3_Game/DayZ/Effects/EffectSound.c:23). The deck needs it on the client to set the
//           start offset of a track that is already under way and to read its position.
// Scope   : One getter. Nothing saved, nothing changed in how an EffectSound behaves.
// Credit  : The same one-line accessor exists in Zens Music by Zenarchist
//           (ZenMusic:ZenMusicBase/scripts/3_Game/ZenMusicBase/Effects/EffectSound.c:1-13,
//           commit 3de1452). See NOTICE.
// -----------------------------------------------------------------------------
modded class EffectSound
{
    AbstractWave NimbyTapesGetWave()
    {
        return m_SoundWaveObject;
    }
}
