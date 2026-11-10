# Pester 5 tests for the two NimbyTapes tools (Music, Plan 13, Phase 0 step 2):
#   mods\own\nimby-pack\NimbyTapes\tools\Test-TapesAssets.ps1   the audio-against-config check
#   mods\own\nimby-pack\NimbyTapes\tools\New-TimingTrack.ps1    the timing-track generator
# They cover the config reader (comments, nesting, inheritance, the one-sample rule), the
# verdict functions, and end-to-end runs of both scripts with their exit codes. The end-to-end
# runs start each script in a child process of the running shell, the way an operator does
# (-File), inside a throwaway repository under TestDrive. No ffmpeg or ffprobe is needed: each
# is replaced by a small .ps1 stand-in passed with -FfprobePath or -FfmpegPath. A .ps1 stand-in
# runs in process, so the tools' handling of a native tool's stderr is not exercised here.
#
# Regression cases from the November 10, 2026 self-review are marked "(regression)".

BeforeAll {
    . (Join-Path $PSScriptRoot 'TestHelpers.ps1')
    $repo = Get-TestRepoRoot
    $toolDir = Join-Path $repo 'mods\own\nimby-pack\NimbyTapes\tools'
    $assetScript = Join-Path $toolDir 'Test-TapesAssets.ps1'
    $trackScript = Join-Path $toolDir 'New-TimingTrack.ps1'
    . (Get-ScriptFunctionBlock -Path $assetScript -Name 'Get-ConfigTextWithoutComment', 'Find-ClosingBrace', 'Get-ConfigClass', 'Get-ConfigArray', 'Get-ConfigInheritedArray', 'Get-TapeTrackPlan', 'ConvertFrom-FfprobeJson', 'Get-FormatProblem', 'Get-LengthProblem')
    . (Get-ScriptFunctionBlock -Path $trackScript -Name 'Get-TimingTrackDefinition', 'Get-TimingTrackExpression', 'Get-TimingTrackArgument')

    # A config with one base tape, one tape with two tracks, and the sound classes behind them.
    # @TAPES@, @SHADERS@, and @SETS@ are where a test adds its own classes.
    $script:TapeConfigTemplate = @'
class CfgPatches { class NimbyTapes { units[] = { "NimbyTapes_Tape_Test" }; }; };
class CfgVehicles
{
    class Inventory_Base;
    class NimbyTapes_Tape_Base : Inventory_Base
    {
        scope = 0;
        tracks[] = {};
        trackSeconds[] = {};
    };
    class NimbyTapes_Tape_Test : NimbyTapes_Tape_Base
    {
        scope = 2;
        tracks[] = { "A_SoundSet", "B_SoundSet" };
        trackSeconds[] = { 240, 60 };
    };
@TAPES@
};
class CfgSoundShaders
{
    class Base_SoundShader { volume = 1; range = 25; };
    class A_SoundShader : Base_SoundShader { samples[] = { { "NimbyTapes\sound\timing_a", 1 } }; };
    class B_SoundShader : Base_SoundShader { samples[] = { { "NimbyTapes\sound\timing_b", 1 } }; };
@SHADERS@
};
class CfgSoundSets
{
    class Base_SoundSet { volumeFactor = 1; };
    class A_SoundSet : Base_SoundSet { soundShaders[] = { "A_SoundShader" }; };
    class B_SoundSet : Base_SoundSet { soundShaders[] = { "B_SoundShader" }; };
@SETS@
};
'@

    $script:PlanAddon = Join-Path ([System.IO.Path]::GetTempPath()) 'NimbyTapesPlanOnly'

    function New-TapeConfigText {
        # The template with a test's own tape, shader, and set classes added.
        param([string]$Tapes = '', [string]$Shaders = '', [string]$Sets = '')
        return $script:TapeConfigTemplate.Replace('@TAPES@', $Tapes).Replace('@SHADERS@', $Shaders).Replace('@SETS@', $Sets)
    }

    function Get-TestTrackPlan {
        # Get-TapeTrackPlan for config text, with a fixed addon folder and prefix. The plan only
        # builds paths; nothing is read from or written to the folder.
        param([Parameter(Mandatory)][AllowEmptyString()][string]$ConfigText)
        return Get-TapeTrackPlan -ConfigText $ConfigText -AddonPath $script:PlanAddon -AddonPrefix 'NimbyTapes'
    }

    function Join-TestPath {
        # Join-Path over any number of parts (Windows PowerShell 5.1 takes two).
        param([Parameter(Mandatory)][string[]]$Part)
        $path = $Part[0]
        for ($index = 1; $index -lt $Part.Count; $index++) { $path = Join-Path $path $Part[$index] }
        return $path
    }

    function New-FakeTapesRepo {
        # A throwaway repository under -Root: the real NimbyCommon.psm1, a NimbyTapes addon with
        # -ConfigText as its config.cpp and an empty sound folder, and the two stand-in tools.
        param(
            [Parameter(Mandatory)][string]$Root,
            [Parameter(Mandatory)][string]$SourceRepo,
            [Parameter(Mandatory)][AllowEmptyString()][string]$ConfigText
        )

        $modules = Join-TestPath -Part $Root, 'scripts', 'modules'
        $addon = Join-TestPath -Part $Root, 'mods', 'own', 'nimby-pack', 'NimbyTapes'
        $sound = Join-Path $addon 'sound'
        $stub = Join-Path $Root 'stub'
        $facts = Join-Path $stub 'facts'
        foreach ($dir in @($modules, $sound, $facts)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }
        Copy-Item -LiteralPath (Join-Path $SourceRepo 'scripts\modules\NimbyCommon.psm1') -Destination $modules
        Set-Content -LiteralPath (Join-Path $addon 'config.cpp') -Value $ConfigText -Encoding Ascii

        # Stand-in ffprobe: prints facts\<file name>.json for the file it is asked about, and
        # fails for a file with no facts, as ffprobe does for a file it cannot read.
        $ffprobe = Join-Path $stub 'ffprobe.ps1'
        Set-Content -LiteralPath $ffprobe -Encoding Ascii -Value @'
$file = $args[$args.Count - 1]
$fact = Join-Path (Join-Path $PSScriptRoot 'facts') ((Split-Path -Leaf $file) + '.json')
if (-not (Test-Path -LiteralPath $fact -PathType Leaf)) { 'stand-in ffprobe: cannot read the file'; exit 1 }
Get-Content -LiteralPath $fact -Raw
exit 0
'@

        # Stand-in ffmpeg: answers -version and -encoders, then writes its last argument as the
        # output file. With fail.flag beside it, it writes a partial file and fails instead.
        $ffmpeg = Join-Path $stub 'ffmpeg.ps1'
        Set-Content -LiteralPath $ffmpeg -Encoding Ascii -Value @'
$last = $args[$args.Count - 1]
if ($last -eq '-version') { 'ffmpeg version stand-in'; exit 0 }
if ($last -eq '-encoders') { ' A....D libvorbis            libvorbis (codec vorbis)'; exit 0 }
if (Test-Path -LiteralPath (Join-Path $PSScriptRoot 'fail.flag')) {
    Set-Content -LiteralPath $last -Value 'partial' -Encoding Ascii
    'stand-in ffmpeg: encode failed'
    exit 1
}
Set-Content -LiteralPath $last -Value 'encoded by the stand-in' -Encoding Ascii
exit 0
'@
        return [pscustomobject]@{ Root = $Root; Addon = $addon; Sound = $sound; Stub = $stub; Facts = $facts; Ffprobe = $ffprobe; Ffmpeg = $ffmpeg }
    }

    function Add-FakeOgg {
        # A placeholder audio file at -RelativePart under the addon, and what the stand-in
        # ffprobe reports for it. File names must be unique within one fake repository.
        param(
            [Parameter(Mandatory)][pscustomobject]$FakeRepo,
            [Parameter(Mandatory)][string[]]$RelativePart,
            [Parameter(Mandatory)][double]$Seconds,
            [string]$Codec = 'vorbis',
            [int]$Channels = 1,
            [int]$SampleRate = 44100
        )

        $file = Join-TestPath -Part (@($FakeRepo.Addon) + $RelativePart)
        New-Item -ItemType Directory -Path (Split-Path -Parent $file) -Force | Out-Null
        Set-Content -LiteralPath $file -Value 'placeholder audio' -Encoding Ascii
        $duration = $Seconds.ToString('0.000000', [System.Globalization.CultureInfo]::InvariantCulture)
        $json = '{{"streams":[{{"codec_name":"{0}","sample_rate":"{1}","channels":{2}}}],"format":{{"duration":"{3}"}}}}' -f $Codec, $SampleRate, $Channels, $duration
        Set-Content -LiteralPath (Join-Path $FakeRepo.Facts ((Split-Path -Leaf $file) + '.json')) -Value $json -Encoding Ascii
    }

    function Invoke-ToolScript {
        # Runs a tool script in a child process of the running shell and returns its exit code
        # and merged output. -WorkingDirectory is the directory the child starts in.
        param(
            [Parameter(Mandatory)][string]$Script,
            [string[]]$Argument = @(),
            [string]$WorkingDirectory
        )

        $shell = (Get-Process -Id $PID).Path
        $argv = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $Script) + $Argument
        # Windows PowerShell 5.1 turns a child's stderr lines into error records under 2>&1.
        $ErrorActionPreference = 'Continue'
        if ($WorkingDirectory) { Push-Location -LiteralPath $WorkingDirectory }
        try {
            $global:LASTEXITCODE = $null
            $output = & $shell @argv 2>&1 | Out-String
            $code = $LASTEXITCODE
        }
        finally {
            if ($WorkingDirectory) { Pop-Location }
        }
        return [pscustomobject]@{ ExitCode = $code; Output = $output }
    }
}

Describe 'Get-ConfigTextWithoutComment' {
    It 'removes line and block comments' {
        $text = Get-ConfigTextWithoutComment -Text "a = 1; // gone`nb = 2; /* gone`ntoo */ c = 3;"
        $text | Should -Match 'a = 1;'
        $text | Should -Match 'c = 3;'
        $text | Should -Not -Match 'gone'
    }
    It 'keeps comment markers that sit inside double quotes' {
        $text = Get-ConfigTextWithoutComment -Text 'path = "a//b/*c*/d"; // tail'
        $text.Contains('"a//b/*c*/d"') | Should -BeTrue
        $text | Should -Not -Match 'tail'
    }
    It 'drops the rest of the text after an unterminated block comment' {
        (Get-ConfigTextWithoutComment -Text 'a = 1; /* never closed').Trim() | Should -Be 'a = 1;'
    }
}

Describe 'Get-ConfigClass' {
    It 'reads names and parents and skips forward declarations' {
        $classes = @(Get-ConfigClass -Text 'class Outer { class Forward; class Inner : Forward { x = 1; }; y = 2; };')
        $classes.Count | Should -Be 2
        $classes[0].Name | Should -Be 'Outer'
        $classes[0].Parent | Should -Be ''
        $classes[1].Name | Should -Be 'Inner'
        $classes[1].Parent | Should -Be 'Forward'
    }
    It 'blanks nested class bodies out of the outer body' {
        $classes = @(Get-ConfigClass -Text 'class Outer { class Inner { x = 1; }; y = 2; };')
        $classes[0].Body | Should -Match 'y = 2;'
        $classes[0].Body | Should -Not -Match 'x = 1;'
        $classes[1].Body | Should -Match 'x = 1;'
    }
    It 'does not count a brace inside double quotes' {
        $classes = @(Get-ConfigClass -Text 'class A { name = "}"; z = 9; };')
        $classes.Count | Should -Be 1
        $classes[0].Body | Should -Match 'z = 9;'
    }
}

Describe 'Get-ConfigArray' {
    It 'returns strings without quotes and numbers as text, flattening nested braces' {
        $values = Get-ConfigArray -Body 'samples[] = { { "NimbyTapes\sound\a", 1 }, { "NimbyTapes\sound\b", 0.5 } };' -Name 'samples'
        ($values -join '|') | Should -Be 'NimbyTapes\sound\a|1|NimbyTapes\sound\b|0.5'
    }
    It 'returns an empty list, not null, for an empty array' {
        $values = Get-ConfigArray -Body 'tracks[] = {};' -Name 'tracks'
        ($null -eq $values) | Should -BeFalse
        $values.Count | Should -Be 0
    }
    It 'returns null when the body has no such array' {
        (Get-ConfigArray -Body 'trackSeconds[] = { 1 }; mytracks[] = { "x" };' -Name 'tracks') | Should -BeNullOrEmpty
        ($null -eq (Get-ConfigArray -Body 'trackSeconds[] = { 1 }; mytracks[] = { "x" };' -Name 'tracks')) | Should -BeTrue
    }
}

Describe 'Get-TapeTrackPlan' {
    It 'reads the two tracks of the test tape and nothing from the empty base tape' {
        $plan = Get-TestTrackPlan -ConfigText (New-TapeConfigText)
        @($plan.Problems).Count | Should -Be 0
        @($plan.Tracks).Count | Should -Be 2
        $plan.Tracks[0].Tape | Should -Be 'NimbyTapes_Tape_Test'
        $plan.Tracks[0].SoundSet | Should -Be 'A_SoundSet'
        $plan.Tracks[0].Shader | Should -Be 'A_SoundShader'
        $plan.Tracks[0].Seconds | Should -Be 240
        $plan.Tracks[1].Seconds | Should -Be 60
        $plan.Tracks[0].File | Should -Be (Join-TestPath -Part $script:PlanAddon, 'sound', 'timing_a.ogg')
    }
    It 'ignores a tape that is commented out' {
        $tapes = "// class Gone_A : NimbyTapes_Tape_Base { tracks[] = { `"A_SoundSet`" }; trackSeconds[] = { 1 }; };`n/* class Gone_B : NimbyTapes_Tape_Base { tracks[] = { `"A_SoundSet`" }; trackSeconds[] = { 1 }; }; */"
        $plan = Get-TestTrackPlan -ConfigText (New-TapeConfigText -Tapes $tapes)
        @($plan.Tracks).Count | Should -Be 2
        @($plan.Problems).Count | Should -Be 0
    }
    It 'reports a tape whose two lists differ in length and plans none of its tracks' {
        $plan = Get-TestTrackPlan -ConfigText (New-TapeConfigText -Tapes 'class Short : NimbyTapes_Tape_Base { tracks[] = { "A_SoundSet", "B_SoundSet" }; trackSeconds[] = { 240 }; };')
        @($plan.Problems).Count | Should -Be 1
        $plan.Problems[0] | Should -Match 'Short has 2 tracks\[\] and 1 trackSeconds\[\]'
        @($plan.Tracks | Where-Object { $_.Tape -eq 'Short' }).Count | Should -Be 0
    }
    It 'gives a derived tape that sets only trackSeconds[] its parent''s tracks[] (regression)' {
        $plan = Get-TestTrackPlan -ConfigText (New-TapeConfigText -Tapes 'class Child : NimbyTapes_Tape_Test { trackSeconds[] = { 10, 20 }; };')
        @($plan.Problems).Count | Should -Be 0
        $child = @($plan.Tracks | Where-Object { $_.Tape -eq 'Child' })
        $child.Count | Should -Be 2
        $child[0].SoundSet | Should -Be 'A_SoundSet'
        $child[0].Seconds | Should -Be 10
        $child[1].Seconds | Should -Be 20
    }
    It 'gives a derived tape that sets neither list both of its parent''s lists (regression)' {
        $plan = Get-TestTrackPlan -ConfigText (New-TapeConfigText -Tapes 'class Copy : NimbyTapes_Tape_Test { displayName = "Copy"; };')
        @($plan.Problems).Count | Should -Be 0
        @($plan.Tracks | Where-Object { $_.Tape -eq 'Copy' }).Count | Should -Be 2
    }
    It 'reports a sound set that is not defined' {
        $plan = Get-TestTrackPlan -ConfigText (New-TapeConfigText -Tapes 'class Lost : NimbyTapes_Tape_Base { tracks[] = { "Nowhere_SoundSet" }; trackSeconds[] = { 5 }; };')
        ($plan.Problems -join "`n") | Should -Match "Lost track 0 sound set 'Nowhere_SoundSet' is not defined"
    }
    It 'reports a sound set that names two sound shaders' {
        $plan = Get-TestTrackPlan -ConfigText (New-TapeConfigText -Tapes 'class Two : NimbyTapes_Tape_Base { tracks[] = { "Two_SoundSet" }; trackSeconds[] = { 5 }; };' -Sets 'class Two_SoundSet : Base_SoundSet { soundShaders[] = { "A_SoundShader", "B_SoundShader" }; };')
        ($plan.Problems -join "`n") | Should -Match 'Two track 0 sound set .* must name exactly one sound shader'
    }
    It 'reports a sound shader with two samples instead of reading the first (regression)' {
        $shaders = 'class Pair_SoundShader : Base_SoundShader { samples[] = { { "NimbyTapes\sound\timing_a", 1 }, { "NimbyTapes\sound\timing_b", 1 } }; };'
        $sets = 'class Pair_SoundSet : Base_SoundSet { soundShaders[] = { "Pair_SoundShader" }; };'
        $plan = Get-TestTrackPlan -ConfigText (New-TapeConfigText -Tapes 'class Pair : NimbyTapes_Tape_Base { tracks[] = { "Pair_SoundSet" }; trackSeconds[] = { 5 }; };' -Shaders $shaders -Sets $sets)
        ($plan.Problems -join "`n") | Should -Match 'Pair track 0 sound shader .* must have exactly one sample'
        @($plan.Tracks | Where-Object { $_.Tape -eq 'Pair' }).Count | Should -Be 0
    }
    It 'reads samples[] from a parent sound shader' {
        $shaders = 'class Heir_SoundShader : A_SoundShader { volume = 0.5; };'
        $sets = 'class Heir_SoundSet : Base_SoundSet { soundShaders[] = { "Heir_SoundShader" }; };'
        $plan = Get-TestTrackPlan -ConfigText (New-TapeConfigText -Tapes 'class Heir : NimbyTapes_Tape_Base { tracks[] = { "Heir_SoundSet" }; trackSeconds[] = { 240 }; };' -Shaders $shaders -Sets $sets)
        @($plan.Problems).Count | Should -Be 0
        $heir = @($plan.Tracks | Where-Object { $_.Tape -eq 'Heir' })
        $heir.Count | Should -Be 1
        (Split-Path -Leaf $heir[0].File) | Should -Be 'timing_a.ogg'
    }
    It 'reports a sample outside the addon prefix' {
        $shaders = 'class Away_SoundShader : Base_SoundShader { samples[] = { { "DZ\sounds\x", 1 } }; };'
        $sets = 'class Away_SoundSet : Base_SoundSet { soundShaders[] = { "Away_SoundShader" }; };'
        $plan = Get-TestTrackPlan -ConfigText (New-TapeConfigText -Tapes 'class Away : NimbyTapes_Tape_Base { tracks[] = { "Away_SoundSet" }; trackSeconds[] = { 5 }; };' -Shaders $shaders -Sets $sets)
        ($plan.Problems -join "`n") | Should -Match "is outside the addon prefix 'NimbyTapes'"
    }
    It 'resolves a sample outside sound\ and one written with forward slashes (regression)' {
        $shaders = 'class Data_SoundShader : Base_SoundShader { samples[] = { { "NimbyTapes\data\elsewhere", 1 } }; }; class Slash_SoundShader : Base_SoundShader { samples[] = { { "\NimbyTapes/sound/deep/slashed", 1 } }; };'
        $sets = 'class Data_SoundSet : Base_SoundSet { soundShaders[] = { "Data_SoundShader" }; }; class Slash_SoundSet : Base_SoundSet { soundShaders[] = { "Slash_SoundShader" }; };'
        $plan = Get-TestTrackPlan -ConfigText (New-TapeConfigText -Tapes 'class Odd : NimbyTapes_Tape_Base { tracks[] = { "Data_SoundSet", "Slash_SoundSet" }; trackSeconds[] = { 5, 6 }; };' -Shaders $shaders -Sets $sets)
        @($plan.Problems).Count | Should -Be 0
        $odd = @($plan.Tracks | Where-Object { $_.Tape -eq 'Odd' })
        $odd[0].File | Should -Be (Join-TestPath -Part $script:PlanAddon, 'data', 'elsewhere.ogg')
        $odd[1].File | Should -Be (Join-TestPath -Part $script:PlanAddon, 'sound', 'deep', 'slashed.ogg')
    }
    It 'reports a trackSeconds[] value that is not above zero' {
        $plan = Get-TestTrackPlan -ConfigText (New-TapeConfigText -Tapes 'class Zero : NimbyTapes_Tape_Base { tracks[] = { "A_SoundSet" }; trackSeconds[] = { 0 }; };')
        ($plan.Problems -join "`n") | Should -Match "Zero track 0 trackSeconds '0' is not a length above zero"
    }
    It 'reads the repository config: every timing track is on a tape with the generator''s length' {
        $addon = Join-Path $repo 'mods\own\nimby-pack\NimbyTapes'
        $plan = Get-TapeTrackPlan -ConfigText ([System.IO.File]::ReadAllText((Join-Path $addon 'config.cpp'))) -AddonPath $addon -AddonPrefix 'NimbyTapes'
        @($plan.Problems).Count | Should -Be 0
        foreach ($definition in (Get-TimingTrackDefinition)) {
            $onTape = @($plan.Tracks | Where-Object { (Split-Path -Leaf $_.File) -eq ($definition.Name + '.ogg') })
            ($onTape.Count -ge 1) | Should -BeTrue
            foreach ($track in $onTape) { $track.Seconds | Should -Be $definition.Seconds }
        }
    }
}

Describe 'ConvertFrom-FfprobeJson' {
    It 'reads codec, channels, sample rate, and length' {
        $fact = ConvertFrom-FfprobeJson -Json '{"streams":[{"codec_name":"vorbis","sample_rate":"44100","channels":1}],"format":{"duration":"240.013000"}}'
        $fact.Codec | Should -Be 'vorbis'
        $fact.Channels | Should -Be 1
        $fact.SampleRate | Should -Be 44100
        $fact.Seconds | Should -Be 240.013
    }
    It 'gives a length of 0 when ffprobe reports no format block' {
        (ConvertFrom-FfprobeJson -Json '{"streams":[{"codec_name":"vorbis","sample_rate":"44100","channels":1}]}').Seconds | Should -Be 0
    }
    It 'returns null for empty text, text that is not JSON, and JSON with no stream' {
        ($null -eq (ConvertFrom-FfprobeJson -Json '')) | Should -BeTrue
        ($null -eq (ConvertFrom-FfprobeJson -Json 'not json at all')) | Should -BeTrue
        ($null -eq (ConvertFrom-FfprobeJson -Json '{"streams":[]}')) | Should -BeTrue
        ($null -eq (ConvertFrom-FfprobeJson -Json '{"format":{"duration":"1"}}')) | Should -BeTrue
    }
}

Describe 'Get-FormatProblem' {
    It 'has nothing to say about Vorbis, mono, 44,100 Hz' {
        @(Get-FormatProblem -Fact ([pscustomobject]@{ Codec = 'vorbis'; Channels = 1; SampleRate = 44100; Seconds = 1.0 })).Count | Should -Be 0
    }
    It 'names each of codec, channel count, and sample rate when wrong' {
        $problems = @(Get-FormatProblem -Fact ([pscustomobject]@{ Codec = 'mp3'; Channels = 2; SampleRate = 48000; Seconds = 1.0 }))
        $problems.Count | Should -Be 3
        ($problems -join "`n") | Should -Match "codec is 'mp3'"
        ($problems -join "`n") | Should -Match 'channels is 2'
        ($problems -join "`n") | Should -Match 'sample rate is 48000'
    }
}

Describe 'Get-LengthProblem' {
    It 'accepts a length inside the tolerance and one exactly on it' {
        @(Get-LengthProblem -Measured 240.4 -Expected 240 -Tolerance 1).Count | Should -Be 0
        @(Get-LengthProblem -Measured 241 -Expected 240 -Tolerance 1).Count | Should -Be 0
        @(Get-LengthProblem -Measured 239 -Expected 240 -Tolerance 1).Count | Should -Be 0
    }
    It 'reports a length outside the tolerance in either direction' {
        $long = @(Get-LengthProblem -Measured 241.5 -Expected 240 -Tolerance 1)
        $long.Count | Should -Be 1
        $long[0] | Should -Match 'length is 241[.,]50 s, trackSeconds\[\] says 240 s'
        @(Get-LengthProblem -Measured 238.5 -Expected 240 -Tolerance 1).Count | Should -Be 1
    }
}

Describe 'New-TimingTrack.ps1 functions' {
    It 'defines timing_a at 240 seconds and timing_b at 60 seconds with different pitches' {
        $definitions = @(Get-TimingTrackDefinition)
        $definitions.Count | Should -Be 2
        $definitions[0].Name | Should -Be 'timing_a'
        $definitions[0].Seconds | Should -Be 240
        $definitions[1].Name | Should -Be 'timing_b'
        $definitions[1].Seconds | Should -Be 60
        ($definitions[0].BeepHz -ne $definitions[1].BeepHz) | Should -BeTrue
        ($definitions[0].PipHz -ne $definitions[1].PipHz) | Should -BeTrue
    }
    It 'builds an expression with no space and no double quote, so it travels as one argument' {
        $expression = Get-TimingTrackExpression -BeepHz 880 -PipHz 1760
        $expression.Contains(' ') | Should -BeFalse
        $expression.Contains('"') | Should -BeFalse
        $expression.Contains('2*PI*880*t') | Should -BeTrue
        $expression.Contains('2*PI*1760*t') | Should -BeTrue
    }
    It 'builds ffmpeg arguments for Vorbis, mono, 44,100 Hz that end in the output file' {
        $track = @(Get-TimingTrackDefinition)[0]
        $arguments = @(Get-TimingTrackArgument -Track $track -OutFile 'out.tmp.ogg')
        @($arguments | Where-Object { [string]::IsNullOrEmpty($_) }).Count | Should -Be 0
        $arguments[$arguments.Count - 1] | Should -Be 'out.tmp.ogg'
        $arguments[$arguments.Count - 3] | Should -Be '-f'
        $arguments[$arguments.Count - 2] | Should -Be 'ogg'
        $arguments[[array]::IndexOf($arguments, '-c:a') + 1] | Should -Be 'libvorbis'
        $arguments[[array]::IndexOf($arguments, '-ac') + 1] | Should -Be '1'
        $arguments[[array]::IndexOf($arguments, '-ar') + 1] | Should -Be '44100'
        $source = $arguments[[array]::IndexOf($arguments, '-i') + 1]
        $source.StartsWith("aevalsrc='") | Should -BeTrue
        $source.EndsWith("':s=44100:d=240") | Should -BeTrue
        ([array]::IndexOf($arguments, '-y') -ge 0) | Should -BeTrue
    }
}

Describe 'Test-TapesAssets.ps1 end to end' {
    BeforeAll {
        # good: two tracks that match the config.
        $good = New-FakeTapesRepo -Root (Join-Path $TestDrive 'good') -SourceRepo $repo -ConfigText (New-TapeConfigText)
        Add-FakeOgg -FakeRepo $good -RelativePart 'sound', 'timing_a.ogg' -Seconds 240.01
        Add-FakeOgg -FakeRepo $good -RelativePart 'sound', 'timing_b.ogg' -Seconds 60.02

        # bad: a stereo track file outside sound\, a track 5 seconds short, a nested file on no
        # tape, and a file the stand-in ffprobe cannot read.
        $badShaders = 'class Data_SoundShader : Base_SoundShader { samples[] = { { "NimbyTapes\data\elsewhere", 1 } }; };'
        $badSets = 'class Data_SoundSet : Base_SoundSet { soundShaders[] = { "Data_SoundShader" }; };'
        $badTapes = 'class NimbyTapes_Tape_Data : NimbyTapes_Tape_Base { tracks[] = { "Data_SoundSet" }; trackSeconds[] = { 30 }; };'
        $bad = New-FakeTapesRepo -Root (Join-Path $TestDrive 'bad') -SourceRepo $repo -ConfigText (New-TapeConfigText -Tapes $badTapes -Shaders $badShaders -Sets $badSets)
        Add-FakeOgg -FakeRepo $bad -RelativePart 'sound', 'timing_a.ogg' -Seconds 240
        Add-FakeOgg -FakeRepo $bad -RelativePart 'sound', 'timing_b.ogg' -Seconds 55
        Add-FakeOgg -FakeRepo $bad -RelativePart 'data', 'elsewhere.ogg' -Seconds 30 -Channels 2
        Add-FakeOgg -FakeRepo $bad -RelativePart 'sound', 'deep', 'spare.ogg' -Seconds 3
        Set-Content -LiteralPath (Join-Path $bad.Sound 'broken.ogg') -Value 'no facts for this one' -Encoding Ascii
        $badRun = Invoke-ToolScript -Script $assetScript -Argument '-AddonPath', $bad.Addon, '-FfprobePath', $bad.Ffprobe

        # bare: a config with no tape at all, and one audio file.
        $bare = New-FakeTapesRepo -Root (Join-Path $TestDrive 'bare') -SourceRepo $repo -ConfigText 'class CfgPatches { class NimbyTapes { units[] = {}; }; };'
        Add-FakeOgg -FakeRepo $bare -RelativePart 'sound', 'timing_a.ogg' -Seconds 240
    }

    It 'passes all four checks, given a relative -AddonPath from the repository root (regression)' {
        $relative = Join-TestPath -Part 'mods', 'own', 'nimby-pack', 'NimbyTapes'
        $run = Invoke-ToolScript -Script $assetScript -Argument '-AddonPath', $relative, '-FfprobePath', $good.Ffprobe -WorkingDirectory $good.Root
        $run.ExitCode | Should -Be 0
        foreach ($check in 'config', 'format', 'length', 'unused') { $run.Output | Should -Match ("(?m)^PASS {0}\s*$" -f $check) }
        $run.Output | Should -Match 'PASS: 0 failure'
        (@(Get-ChildItem -LiteralPath (Join-Path $good.Root 'logs') -Filter 'tapes-assets-*.log').Count -ge 1) | Should -BeTrue
    }
    It 'fails with exit 1 on the bad addon' {
        $badRun.ExitCode | Should -Be 1
        $badRun.Output | Should -Match 'FAIL: \d+ failure'
    }
    It 'probes a track file that sits outside sound\ and reports its format (regression)' {
        $badRun.Output | Should -Match 'FAIL format .*elsewhere\.ogg channels is 2'
    }
    It 'reports a file ffprobe cannot read' {
        $badRun.Output | Should -Match 'FAIL format .*broken\.ogg could not be read by ffprobe'
    }
    It 'reports a track whose length is outside the tolerance' {
        $badRun.Output | Should -Match 'FAIL length NimbyTapes_Tape_Test track 1 \(timing_b\.ogg\) length is 55'
    }
    It 'reports files under sound\ at any depth that are on no tape' {
        $badRun.Output | Should -Match 'FAIL unused spare\.ogg is on no tape'
        $badRun.Output | Should -Match 'FAIL unused broken\.ogg is on no tape'
    }
    It 'passes the config check on the bad addon, whose config is itself sound' {
        $badRun.Output | Should -Match '(?m)^PASS config\s*$'
    }
    It 'fails a config with no tape, and runs the format check alone with -AllowMissingConfig' {
        $strict = Invoke-ToolScript -Script $assetScript -Argument '-AddonPath', $bare.Addon, '-FfprobePath', $bare.Ffprobe
        $strict.ExitCode | Should -Be 1
        $strict.Output | Should -Match 'FAIL config config\.cpp defines no tape class'
        $lenient = Invoke-ToolScript -Script $assetScript -Argument '-AddonPath', $bare.Addon, '-FfprobePath', $bare.Ffprobe, '-AllowMissingConfig'
        $lenient.ExitCode | Should -Be 0
        $lenient.Output | Should -Match '(?m)^PASS format\s*$'
        $lenient.Output | Should -Not -Match 'PASS config'
        $lenient.Output | Should -Not -Match 'PASS length'
    }
    It 'fails with the install command when -FfprobePath names no file' {
        $run = Invoke-ToolScript -Script $assetScript -Argument '-AddonPath', $good.Addon, '-FfprobePath', (Join-Path $good.Stub 'absent.exe')
        $run.ExitCode | Should -Be 1
        $run.Output | Should -Match 'FAIL error ffprobe not found at'
        $run.Output | Should -Match 'winget install --id Gyan\.FFmpeg'
    }
    It 'fails when -AddonPath is not an addon folder' {
        $run = Invoke-ToolScript -Script $assetScript -Argument '-AddonPath', $good.Stub, '-RepoRoot', $good.Root, '-FfprobePath', $good.Ffprobe
        $run.ExitCode | Should -Be 1
        $run.Output | Should -Match 'has no config\.cpp'
    }
}

Describe 'New-TimingTrack.ps1 end to end' {
    BeforeAll {
        $fake = New-FakeTapesRepo -Root (Join-Path $TestDrive 'tracks') -SourceRepo $repo -ConfigText (New-TapeConfigText)
        $trackA = Join-Path $fake.Sound 'timing_a.ogg'
        $trackB = Join-Path $fake.Sound 'timing_b.ogg'
        $relative = Join-TestPath -Part 'mods', 'own', 'nimby-pack', 'NimbyTapes'
        # The four runs happen in order, each on what the one before left behind.
        $first = Invoke-ToolScript -Script $trackScript -Argument '-AddonPath', $relative, '-FfmpegPath', $fake.Ffmpeg -WorkingDirectory $fake.Root
        $afterFirst = @(Get-ChildItem -LiteralPath $fake.Sound -File | ForEach-Object { $_.Name } | Sort-Object) -join ','
        Set-Content -LiteralPath $trackA -Value 'KEEP' -Encoding Ascii
        Set-Content -LiteralPath $trackB -Value 'KEEP' -Encoding Ascii
        $second = Invoke-ToolScript -Script $trackScript -Argument '-AddonPath', $fake.Addon, '-FfmpegPath', $fake.Ffmpeg
        $afterSecond = (Get-Content -LiteralPath $trackA -Raw).Trim()
        Set-Content -LiteralPath (Join-Path $fake.Stub 'fail.flag') -Value 'fail' -Encoding Ascii
        $third = Invoke-ToolScript -Script $trackScript -Argument '-AddonPath', $fake.Addon, '-FfmpegPath', $fake.Ffmpeg, '-Force'
        $afterThird = (Get-Content -LiteralPath $trackA -Raw).Trim()
        $partialAfterThird = Test-Path -LiteralPath (Join-Path $fake.Sound 'timing_a.tmp.ogg') -PathType Leaf
        # Clear the flag so the last run encodes again.
        Remove-Item -LiteralPath (Join-Path $fake.Stub 'fail.flag')
        $fourth = Invoke-ToolScript -Script $trackScript -Argument '-AddonPath', $fake.Addon, '-FfmpegPath', $fake.Ffmpeg, '-Force'
        $afterFourth = (Get-Content -LiteralPath $trackA -Raw).Trim()
        $namesAfterFourth = @(Get-ChildItem -LiteralPath $fake.Sound -File | ForEach-Object { $_.Name } | Sort-Object) -join ','
    }

    It 'writes both tracks and leaves no temporary file, given a relative -AddonPath (regression)' {
        $first.ExitCode | Should -Be 0
        $afterFirst | Should -Be 'timing_a.ogg,timing_b.ogg'
        $first.Output | Should -Match 'OK .*timing_a\.ogg seconds=240 beepHz=880 pipHz=1760'
        $first.Output | Should -Match 'OK .*timing_b\.ogg seconds=60 beepHz=440 pipHz=660'
        (@(Get-ChildItem -LiteralPath (Join-Path $fake.Root 'logs') -Filter 'tapes-timingtrack-*.log').Count -ge 1) | Should -BeTrue
    }
    It 'leaves existing tracks alone without -Force' {
        $second.ExitCode | Should -Be 0
        $second.Output | Should -Match 'SKIP .*timing_a\.ogg exists'
        $second.Output | Should -Match 'SKIP .*timing_b\.ogg exists'
        $afterSecond | Should -Be 'KEEP'
    }
    It 'keeps the existing track when the encode fails under -Force (regression)' {
        $third.ExitCode | Should -Be 1
        $third.Output | Should -Match 'FAIL timing_a ffmpeg exit=1'
        $third.Output | Should -Match 'NOTE partial file left at .*timing_a\.tmp\.ogg'
        $afterThird | Should -Be 'KEEP'
        $partialAfterThird | Should -BeTrue
    }
    It 'replaces the tracks under -Force and takes the leftover temporary file with it (regression)' {
        $fourth.ExitCode | Should -Be 0
        $afterFourth | Should -Be 'encoded by the stand-in'
        $namesAfterFourth | Should -Be 'timing_a.ogg,timing_b.ogg'
    }
    It 'fails with the install command when -FfmpegPath names no file' {
        $run = Invoke-ToolScript -Script $trackScript -Argument '-AddonPath', $fake.Addon, '-FfmpegPath', (Join-Path $fake.Stub 'absent.exe')
        $run.ExitCode | Should -Be 1
        $run.Output | Should -Match 'FAIL error ffmpeg not found at'
        $run.Output | Should -Match 'winget install --id Gyan\.FFmpeg'
    }
}
