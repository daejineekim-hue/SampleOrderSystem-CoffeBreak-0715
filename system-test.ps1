param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)
$ErrorActionPreference = "Stop"

& "$PSScriptRoot\build.ps1" -Configuration $Configuration

$exe = "$PSScriptRoot\build\x64\$Configuration\SampleOrderSystem.exe"
$samplesFile = "$PSScriptRoot\data\samples.json"
$ordersFile = "$PSScriptRoot\data\orders.json"
$today = Get-Date -Format "yyyyMMdd"
$order1 = "ORD-$today-0001"

$script:failures = @()

function Invoke-Scenario {
    param([string[]]$InputLines)

    Remove-Item $samplesFile, $ordersFile -ErrorAction SilentlyContinue
    Push-Location $PSScriptRoot
    try {
        $inputText = ($InputLines -join "`n") + "`n"
        $output = $inputText | & $exe | Out-String
        return @{ Output = $output; ExitCode = $LASTEXITCODE }
    } finally {
        Pop-Location
    }
}

function Assert-Contains {
    param([string]$Output, [string]$Expected, [string]$Scenario)
    if ($Output -notlike "*$Expected*") {
        $script:failures += "[$Scenario] stdout에 다음 문자열이 없음: '$Expected'"
    }
}

function Assert-True {
    param([bool]$Condition, [string]$Message, [string]$Scenario)
    if (-not $Condition) {
        $script:failures += "[$Scenario] $Message"
    }
}

# ---- 시나리오 1: 정상 플로우 (재고충분 승인 -> 출고) ----
$result = Invoke-Scenario @(
    "1", "1", "SMP-001", "Wafer-A", "20", "0.9", "100", "0",
    "2", "1", "SMP-001", "customer-A", "10", "0",
    "3", "2", $order1, "1", "0",
    "4",
    "6", "2", $order1, "0",
    "0"
)
Assert-Contains $result.Output "등록되었습니다" "S1-happy-path"
Assert-Contains $result.Output "접수되었습니다. 주문번호: $order1" "S1-happy-path"
Assert-Contains $result.Output "처리되었습니다. 상태: CONFIRMED" "S1-happy-path"
Assert-Contains $result.Output "RESERVED=0  CONFIRMED=1  PRODUCING=0  RELEASE=0" "S1-happy-path"
Assert-Contains $result.Output "출고 처리되었습니다" "S1-happy-path"
Assert-True ($result.ExitCode -eq 0) "exe가 0이 아닌 종료 코드를 반환함: $($result.ExitCode)" "S1-happy-path"
if (Test-Path $ordersFile) {
    $orders = Get-Content $ordersFile -Raw | ConvertFrom-Json
    Assert-True ($orders[0].status -eq "RELEASE") "주문 상태가 RELEASE가 아님: $($orders[0].status)" "S1-happy-path"
} else {
    $script:failures += "[S1-happy-path] orders.json이 생성되지 않음"
}
if (Test-Path $samplesFile) {
    $samples = Get-Content $samplesFile -Raw | ConvertFrom-Json
    Assert-True ($samples[0].stock -eq 90) "출고 후 재고가 90이 아님(재차감 의심): $($samples[0].stock)" "S1-happy-path"
} else {
    $script:failures += "[S1-happy-path] samples.json이 생성되지 않음"
}

# ---- 시나리오 2: 재고부족 -> 생산라인 라우팅 (worked example: shortage=50, yield=0.92) ----
$result = Invoke-Scenario @(
    "1", "1", "SMP-001", "Wafer-A", "20", "0.92", "30", "0",
    "2", "1", "SMP-001", "customer-A", "80", "0",
    "3", "2", $order1, "1", "0",
    "5",
    "0"
)
Assert-Contains $result.Output "처리되었습니다. 상태: PRODUCING" "S2-insufficient-stock"
Assert-Contains $result.Output "생산예정량=61" "S2-insufficient-stock"
Assert-Contains $result.Output "남은시간=1220" "S2-insufficient-stock"

# ---- 시나리오 3: 주문 거절 -> 모니터링 집계에서 제외 ----
$result = Invoke-Scenario @(
    "1", "1", "SMP-001", "Wafer-A", "20", "0.9", "50", "0",
    "2", "1", "SMP-001", "customer-A", "10", "0",
    "3", "2", $order1, "2", "0",
    "4",
    "0"
)
Assert-Contains $result.Output "거절되었습니다" "S3-reject"
Assert-Contains $result.Output "RESERVED=0  CONFIRMED=0  PRODUCING=0  RELEASE=0" "S3-reject"

# ---- 시나리오 4: 잘못된 메인 메뉴 입력 후 정상 복구 ----
$result = Invoke-Scenario @("99", "abc", "1", "0", "0")
$errorCount = ([regex]::Matches($result.Output, [regex]::Escape("[오류]"))).Count
Assert-True ($errorCount -ge 2) "오류 메시지가 2회 미만 표시됨 (실제: $errorCount)" "S4-invalid-input-recovery"
Assert-Contains $result.Output "시스템을 종료합니다" "S4-invalid-input-recovery"
Assert-True ($result.ExitCode -eq 0) "잘못된 입력 후 비정상 종료: $($result.ExitCode)" "S4-invalid-input-recovery"

# ---- 시나리오 5: 숨김 더미 데이터 메뉴(9번) ----
$result = Invoke-Scenario @("9", "5", "0")
Assert-Contains $result.Output "총 5건 생성 완료" "S5-dummy-data"
if (Test-Path $samplesFile) {
    $samples = Get-Content $samplesFile -Raw | ConvertFrom-Json
    Assert-True ($samples.Count -eq 5) "생성된 시료 개수가 5가 아님: $($samples.Count)" "S5-dummy-data"
} else {
    $script:failures += "[S5-dummy-data] samples.json이 생성되지 않음"
}

# ---- 시나리오 6: 첫 화면에서 즉시 종료(0) ----
$result = Invoke-Scenario @("0")
Assert-Contains $result.Output "시스템을 종료합니다" "S6-immediate-exit"
Assert-True ($result.ExitCode -eq 0) "즉시 종료 시 비정상 종료 코드: $($result.ExitCode)" "S6-immediate-exit"

# ---- 시나리오 7: 1회성 화면(생산라인 조회/모니터링) 뒤 다음 입력이 씹히지 않아야 함 ----
# 회귀: MainMenuController는 getline()으로 메뉴 번호를 읽어 개행을 이미 소비하는데,
# ProductionLineController/MonitoringController가 화면 끝에서 또 skipToNextLine()을
# 호출하면 남아있는 개행이 없어 사용자의 "다음" 실제 입력 한 줄을 통째로 삼켜버린다.
$result = Invoke-Scenario @(
    "1", "1", "SMP-001", "Wafer-A", "20", "0.9", "100", "0",
    "5",
    "4",
    "0"
)
Assert-Contains $result.Output "생산라인 조회" "S7-no-input-swallowed-after-oneshot-screen"
Assert-Contains $result.Output "주문량 확인" "S7-no-input-swallowed-after-oneshot-screen"
Assert-Contains $result.Output "시스템을 종료합니다" "S7-no-input-swallowed-after-oneshot-screen"
Assert-True ($result.ExitCode -eq 0) "예상치 못한 종료 코드: $($result.ExitCode)" "S7-no-input-swallowed-after-oneshot-screen"

Remove-Item $samplesFile, $ordersFile -ErrorAction SilentlyContinue

if ($script:failures.Count -gt 0) {
    Write-Output "`n=== 시스템 테스트 실패 ($($script:failures.Count)건) ==="
    $script:failures | ForEach-Object { Write-Output "  - $_" }
    exit 1
}

Write-Output "`n모든 시스템 테스트 통과 (7개 시나리오)"
