$root = 'd:\college work\term6\com_graphics\Fromville'

$header = Join-Path $root 'src/game/symbols/SymbolSystem.h'
$headerText = Get-Content -Raw $header
if ($headerText -notmatch '#include <algorithm>') {
    $headerText = $headerText -replace '#include <array>\r?\n', "#include <array>`r`n#include <algorithm>`r`n"
}
Set-Content -Path $header -Value $headerText

$puzzle = Join-Path $root 'src/game/puzzles/JadeSymbolVaultPuzzle.cpp'
$puzzleText = Get-Content -Raw $puzzle
$puzzleText = $puzzleText.Replace('        if (i > 0) selection += " → ";', '        if (i > 0) selection += " -> ";')
$puzzleText = $puzzleText.Replace('            case SymbolType::Knowledge: selection += "◯"; break;', '            case SymbolType::Knowledge: selection += "O"; break;')
$puzzleText = $puzzleText.Replace('            case SymbolType::Power: selection += "△"; break;', '            case SymbolType::Power: selection += "A"; break;')
$puzzleText = $puzzleText.Replace('            case SymbolType::Truth: selection += "◇"; break;', '            case SymbolType::Truth: selection += "D"; break;')
$puzzleText = $puzzleText.Replace('            case SymbolType::Awakening: selection += "◈"; break;', '            case SymbolType::Awakening: selection += "S"; break;')
$puzzleText = $puzzleText.Replace('    textRenderer.RenderText("Vault Resonance: [" + std::string(static_cast<int>(progress * 10), ''█'') + std::string(10 - static_cast<int>(progress * 10), ''░'') + "]",', '    textRenderer.RenderText("Vault Resonance: [" + std::string(static_cast<int>(progress * 10), ''#'') + std::string(10 - static_cast<int>(progress * 10), ''-'') + "]",')
$puzzleText = $puzzleText.Replace('        textRenderer.RenderText("✦ THE TOWN SPEAKS ✦", static_cast<float>(screenWidth) * 0.5f - 200.0f, baseY + 120.0f, 1.8f,', '        textRenderer.RenderText("THE TOWN SPEAKS", static_cast<float>(screenWidth) * 0.5f - 140.0f, baseY + 120.0f, 1.8f,')
Set-Content -Path $puzzle -Value $puzzleText
