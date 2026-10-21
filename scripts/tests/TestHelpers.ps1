# Shared by the *.Tests.ps1 files (Pester 5, Windows PowerShell 5.1 and PowerShell 7).
# Loads named functions out of a script without running the script's main body, so the pure
# functions can be tested with no DayZ, Steam, signing key, or secret.

Set-StrictMode -Version Latest

function Get-TestRepoRoot {
    # The repository root: two levels above scripts\tests.
    return (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
}

function Get-ScriptFunctionBlock {
    # Returns a script block that defines the named functions exactly as the script defines them.
    # Dot-source the result: . (Get-ScriptFunctionBlock -Path <script> -Name <function>, ...)
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string[]]$Name
    )

    $errors = $null
    $ast = [System.Management.Automation.Language.Parser]::ParseFile($Path, [ref]$null, [ref]$errors)
    if (@($errors).Count -gt 0) { throw ("Parse error in {0}: {1}" -f $Path, $errors[0].Message) }
    $found = @{}
    foreach ($function in $ast.FindAll({ param($node) $node -is [System.Management.Automation.Language.FunctionDefinitionAst] }, $false)) {
        if ($Name -contains $function.Name) { $found[$function.Name] = $function.Extent.Text }
    }
    $missing = @($Name | Where-Object { -not $found.ContainsKey($_) })
    if ($missing.Count -gt 0) { throw ("Not defined in {0}: {1}" -f $Path, ($missing -join ', ')) }
    return [scriptblock]::Create((@($Name | ForEach-Object { $found[$_] }) -join "`n`n"))
}

function Get-ScriptAssignmentValue {
    # Evaluates the right-hand side of a top-level "$script:<Name> = <literal>" assignment, so a
    # test can assert a script's constant lists by name without running the script.
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Name
    )

    $ast = [System.Management.Automation.Language.Parser]::ParseFile($Path, [ref]$null, [ref]$null)
    $assignment = $ast.FindAll({
            param($node)
            $node -is [System.Management.Automation.Language.AssignmentStatementAst] -and
            $node.Left.Extent.Text -eq ('$script:{0}' -f $Name)
        }, $false) | Select-Object -First 1
    if ($null -eq $assignment) { throw ("No `$script:{0} assignment in {1}" -f $Name, $Path) }
    return , @(& ([scriptblock]::Create($assignment.Right.Extent.Text)))
}
