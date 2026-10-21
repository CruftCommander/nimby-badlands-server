# Log lines for the boot-verdict tests in Build-NimbyPack.Tests.ps1.
# "real" lines are copied from local evidence logs (logs\ is git-ignored, so the text is kept
# here); "constructed" lines follow the same engine format for patterns no local log has hit.
@{
    FailureLines = @(
        @{ Pattern = 'SCRIPT\s*\(E\)'; Kind = 'real'; Source = 'logs\build-20261014-164948\build.log:35'
            Line = 'SCRIPT    (E): @"NimbyAutorun/Scripts/4_world/nimbyautorun_playerbase.c,93": Syntax error' }
        @{ Pattern = "Can't compile"; Kind = 'real'; Source = 'logs\build-20261017-215154\server-profile\script_2026-10-17_21-51-56.log:41'
            Line = '21:52:00.544 SCRIPT    (E): Can''t compile "World" script module!' }
        @{ Pattern = 'Cannot open file'; Kind = 'constructed'; Source = 'engine format'
            Line = '21:52:00.544 SCRIPT    (E): Cannot open file "NimbyHands/scripts/4_world/nimbyhands_missing.c"' }
        @{ Pattern = 'Unknown type'; Kind = 'constructed'; Source = 'engine format'
            Line = '21:52:00.544 SCRIPT    (E): @"NimbyTack/Scripts/4_World/nimbytack_world.c,12": Unknown type ''NimbyTackMissing''' }
        @{ Pattern = 'Undefined function'; Kind = 'constructed'; Source = 'engine format'
            Line = '21:52:00.544 SCRIPT    (E): @"NimbyNotes/Scripts/4_World/nimbynotes_paper.c,40": Undefined function ''NimbyNotesMissing''' }
        @{ Pattern = 'Scripted variables corrupted'; Kind = 'real'; Source = 'logs\server-20261015-121241\DayZServer_x64_2026-10-15_08-32-47.RPT:953'
            Line = '2026/10/15,  8:33:41.723 !!! Scripted variables corrupted upon "ChestRigVest_Khaki"' }
        @{ Pattern = '\[NIMBY\]\[[^\]]+\]\[SelfTest\] FAIL'; Kind = 'constructed'; Source = 'PASS line format, RPT 2026-10-19 21-52-19:1584'
            Line = '21:52:34.668  SCRIPT       : [NIMBY][Hands][SelfTest] FAIL transfer lifecycle' }
        @{ Pattern = '\[NIMBY\]\[[^\]]+\]\[SelfTest\] FAIL'; Kind = 'constructed'; Source = 'hyphenated log prefix (TGA-016 failure scenario)'
            Line = '21:52:34.668  SCRIPT       : [NIMBY][Dev-Harness][SelfTest] FAIL harness case' }
    )
    QuietLines   = @(
        @{ Name = 'SCRIPT (W)'; Source = 'RPT 2026-10-19 21-52-19:176'
            Line = '21:52:22.271 SCRIPT    (W): @"scripts/3_Game/human.c,17": Missing script declaration for method ''HumanInputController.DisableCrouchCameraHorizontalRotation''' }
        @{ Name = 'ANIMATION (E)'; Source = 'RPT 2026-10-19 21-52-19:182'
            Line = '21:52:22.404 ANIMATION (E): Can''t load D:\GitHub\nimby-badlands-server\build\@NIMBY-20261019-215218/Anims/cfg/skeletons.anim.xml' }
        @{ Name = 'self-test PASS'; Source = 'RPT 2026-10-19 21-52-19:1584'
            Line = '21:52:34.668  SCRIPT       : [NIMBY][Autorun][SelfTest] PASS walk index valid' }
    )
    ReadyLine    = '21:52:53.914 [CE][Hive] :: Init sequence finished.'
}
