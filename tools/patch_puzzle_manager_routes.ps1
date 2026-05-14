$path = 'd:\college work\term6\com_graphics\Fromville\src\game\puzzles\PuzzleManager.cpp'
$lines = Get-Content $path
for ($i = 0; $i -lt $lines.Count; $i++) {
    if ($lines[$i] -eq '    if (questCharacter == CharacterType::Jade) {') {
        $lines[$i + 1] = '        return (objectiveIndex == 0) ? PuzzleType::JadeSymbolPuzzle : PuzzleType::JadeSymbolVaultPuzzle;'
        $lines[$i + 2] = '    }'
        $lines[$i + 3] = ''
        $lines[$i + 4] = '    if (questCharacter == CharacterType::Tabitha) {'
        $lines[$i + 5] = '        return (objectiveIndex == 0) ? PuzzleType::TabithaTunnelPuzzle : PuzzleType::TabithaTunnelMapPuzzle;'
        $lines[$i + 6] = '    }'
        $lines[$i + 7] = ''
        $lines[$i + 8] = '    if (questCharacter == CharacterType::Victor) {'
        $lines[$i + 9] = '        return (objectiveIndex == 0) ? PuzzleType::VictorMemoryPuzzle : PuzzleType::VictorMemoryTimelinePuzzle;'
        $lines[$i + 10] = '    }'
        $lines[$i + 11] = ''
        $lines[$i + 12] = '    if (questCharacter == CharacterType::Sara) {'
        $lines[$i + 13] = '        return (objectiveIndex == 0) ? PuzzleType::SaraRedemptionPuzzle : PuzzleType::SaraFinalJudgmentPuzzle;'
        $lines[$i + 14] = '    }'
        break
    }
}
Set-Content -Path $path -Value $lines
