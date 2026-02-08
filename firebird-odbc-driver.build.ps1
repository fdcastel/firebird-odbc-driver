<#
.Synopsis
	Build script for Firebird ODBC Driver (Invoke-Build)

.Description
	Tasks: clean, build, test, install, uninstall.

.Parameter Configuration
	Build configuration: Debug (default) or Release.
#>

param(
	[ValidateSet('Debug', 'Release')]
	[string]$Configuration = 'Debug'
)

# Detect OS
$IsWindowsOS = $IsWindows -or ($PSVersionTable.PSVersion.Major -le 5)
$IsLinuxOS = $IsLinux

# Computed properties
$BuildDir = Join-Path $BuildRoot 'build'

$DriverName = if ($Configuration -eq 'Release') {
	'Firebird ODBC Driver'
} else {
	'Firebird ODBC Driver (Debug)'
}

if ($IsWindowsOS) {
	$DriverFileName = 'FirebirdODBC.dll'
	$DriverPath = Join-Path $BuildDir $Configuration $DriverFileName
} else {
	$DriverFileName = 'libOdbcFb.so'
	$DriverPath = Join-Path $BuildDir $DriverFileName
}

# Synopsis: Remove the build directory.
task clean {
	remove $BuildDir
}

# Synopsis: Build the driver and tests (default task).
task build {
	exec { cmake -B $BuildDir -S $BuildRoot -DCMAKE_BUILD_TYPE=$Configuration -DBUILD_TESTING=ON }

	if ($IsWindowsOS) {
		exec { cmake --build $BuildDir --config $Configuration }
	} else {
		$jobs = 4
		try { $jobs = [int](nproc) } catch {}
		exec { cmake --build $BuildDir --config $Configuration -- "-j$jobs" }
	}

	assert (Test-Path $DriverPath) "Driver not found at: $DriverPath"
}

# Synopsis: Create Firebird test databases.
task build-test-databases {
	$fbVersion = '5.0.2'
	$envPath = '/fbodbc-tests/fb502'
	$dbPathUtf8 = '/fbodbc-tests/TEST.FB50.FDB'
	$dbPathIso = '/fbodbc-tests/TEST-ISO.FB50.FDB'

	if ($IsLinuxOS) {
		exec { sudo mkdir -p '/fbodbc-tests' }
		exec { sudo chmod 777 '/fbodbc-tests' }
	} else {
		New-Item -ItemType Directory -Path '/fbodbc-tests' -Force | Out-Null
	}

	Import-Module PSFirebird

	# Create or reuse Firebird environment
	if (Test-Path (Join-Path $envPath 'firebird.msg')) {
		$fb = Get-FirebirdEnvironment -Path $envPath
		print Green "Reusing existing Firebird environment: $envPath"
	} else {
		$fb = New-FirebirdEnvironment -Version $fbVersion -Path $envPath -Force
		print Green "Firebird environment created: $envPath"
	}

	# Ensure fbintl.conf exists (PSFirebird doesn't include it, but Firebird
	# needs it to resolve non-built-in charsets like ISO8859_1)
	$fbintlConf = Join-Path $fb.Path 'intl' 'fbintl.conf'
	if (-not (Test-Path $fbintlConf)) {
		@'
intl_module = builtin {
    icu_versions = default
}

intl_module = fbintl {
    filename = $(this)/fbintl
    icu_versions = default
}

charset = ISO8859_1 {
    intl_module = fbintl
    collation = ISO8859_1
}

charset = ISO8859_2 {
    intl_module = fbintl
    collation = ISO8859_2
}

charset = ISO8859_3 {
    intl_module = fbintl
    collation = ISO8859_3
}

charset = ISO8859_4 {
    intl_module = fbintl
    collation = ISO8859_4
}

charset = ISO8859_5 {
    intl_module = fbintl
    collation = ISO8859_5
}

charset = ISO8859_6 {
    intl_module = fbintl
    collation = ISO8859_6
}

charset = ISO8859_7 {
    intl_module = fbintl
    collation = ISO8859_7
}

charset = ISO8859_8 {
    intl_module = fbintl
    collation = ISO8859_8
}

charset = ISO8859_9 {
    intl_module = fbintl
    collation = ISO8859_9
}

charset = ISO8859_13 {
    intl_module = fbintl
    collation = ISO8859_13
}

charset = WIN1250 {
    intl_module = fbintl
    collation = WIN1250
}

charset = WIN1251 {
    intl_module = fbintl
    collation = WIN1251
}

charset = WIN1252 {
    intl_module = fbintl
    collation = WIN1252
}

charset = WIN1253 {
    intl_module = fbintl
    collation = WIN1253
}

charset = WIN1254 {
    intl_module = fbintl
    collation = WIN1254
}

charset = WIN1255 {
    intl_module = fbintl
    collation = WIN1255
}

charset = WIN1256 {
    intl_module = fbintl
    collation = WIN1256
}

charset = WIN1257 {
    intl_module = fbintl
    collation = WIN1257
}

charset = WIN1258 {
    intl_module = fbintl
    collation = WIN1258
}

charset = DOS437 {
    intl_module = fbintl
    collation = DOS437
}

charset = DOS850 {
    intl_module = fbintl
    collation = DOS850
}

charset = DOS852 {
    intl_module = fbintl
    collation = DOS852
}

charset = DOS857 {
    intl_module = fbintl
    collation = DOS857
}

charset = DOS858 {
    intl_module = fbintl
    collation = DOS858
}

charset = DOS860 {
    intl_module = fbintl
    collation = DOS860
}

charset = DOS861 {
    intl_module = fbintl
    collation = DOS861
}

charset = DOS862 {
    intl_module = fbintl
    collation = DOS862
}

charset = DOS863 {
    intl_module = fbintl
    collation = DOS863
}

charset = DOS864 {
    intl_module = fbintl
    collation = DOS864
}

charset = DOS865 {
    intl_module = fbintl
    collation = DOS865
}

charset = DOS866 {
    intl_module = fbintl
    collation = DOS866
}

charset = DOS869 {
    intl_module = fbintl
    collation = DOS869
}

charset = CYRL {
    intl_module = fbintl
    collation = CYRL
}

charset = KOI8R {
    intl_module = fbintl
    collation = KOI8R
}

charset = KOI8U {
    intl_module = fbintl
    collation = KOI8U
}

charset = BIG_5 {
    intl_module = fbintl
    collation = BIG_5
}

charset = GB_2312 {
    intl_module = fbintl
    collation = GB_2312
}

charset = GBK {
    intl_module = fbintl
    collation = GBK
}

charset = GB18030 {
    intl_module = fbintl
    collation = GB18030
}

charset = KSC_5601 {
    intl_module = fbintl
    collation = KSC_5601
}

charset = TIS620 {
    intl_module = fbintl
    collation = TIS620
}

charset = SJIS_0208 {
    intl_module = fbintl
    collation = SJIS_0208
}

charset = EUCJ_0208 {
    intl_module = fbintl
    collation = EUCJ_0208
}

charset = CP943C {
    intl_module = fbintl
    collation = CP943C
}
'@ | Set-Content -Path $fbintlConf -Encoding ASCII
		print Green "Created $fbintlConf"
	}

	# Stop any running Firebird processes so database files can be replaced
	if ($IsWindowsOS) {
		Get-Service -Name 'FirebirdServer*' -ErrorAction SilentlyContinue |
			Where-Object Status -eq 'Running' |
			Stop-Service -Force -ErrorAction SilentlyContinue
	}
	$fbProcs = Get-Process -Name 'firebird' -ErrorAction SilentlyContinue
	if ($fbProcs) {
		$fbProcs | Stop-Process -Force -ErrorAction SilentlyContinue
		$fbProcs | Wait-Process -Timeout 10 -ErrorAction SilentlyContinue
	}
	if ($IsLinuxOS) {
		sudo pkill -f firebird 2>$null
		Start-Sleep -Seconds 2
	}

	New-FirebirdDatabase -Database $dbPathUtf8 -Environment $fb -Force
	print Green "UTF8 database created: $dbPathUtf8"

	New-FirebirdDatabase -Database $dbPathIso -Environment $fb -Charset ISO8859_1 -Force
	print Green "ISO8859_1 database created: $dbPathIso"

	# Determine client library path
	if ($IsWindowsOS) {
		$script:ClientPath = (Resolve-Path (Join-Path $fb.Path 'fbclient.dll')).Path
	} else {
		$clientLib = Join-Path $fb.Path 'lib' 'libfbclient.so'
		if (-not (Test-Path $clientLib)) {
			$clientLib = Get-ChildItem -Path $fb.Path -Recurse -Filter 'libfbclient.so*' |
				Select-Object -First 1 -ExpandProperty FullName
		}
		$script:ClientPath = $clientLib
	}

	print Green "Client library: $script:ClientPath"
}

# Synopsis: Run the test suite.
task test build, build-test-databases, install, {
	if (-not $env:FIREBIRD_ODBC_CONNECTION) {
		print Yellow 'WARNING: FIREBIRD_ODBC_CONNECTION environment variable is not set. Using built-in connection strings.'
	}

	$testConfigs = @(
		@{ Database = '/fbodbc-tests/TEST.FB50.FDB';     Charset = 'UTF8';      Label = 'UTF8 database, UTF8 charset' }
		@{ Database = '/fbodbc-tests/TEST-ISO.FB50.FDB'; Charset = 'ISO8859_1'; Label = 'ISO8859_1 database, ISO8859_1 charset' }
		@{ Database = '/fbodbc-tests/TEST-ISO.FB50.FDB'; Charset = 'UTF8';      Label = 'ISO8859_1 database, UTF8 charset' }
	)

	$passed = 0
	foreach ($cfg in $testConfigs) {
		$env:FIREBIRD_ODBC_CONNECTION = "Driver={$DriverName};Database=$($cfg.Database);UID=SYSDBA;PWD=masterkey;CHARSET=$($cfg.Charset);CLIENT=$script:ClientPath"
		print Cyan "--- Test run: $($cfg.Label) ---"
		print Cyan "    Connection: $env:FIREBIRD_ODBC_CONNECTION"
		exec { ctest --test-dir $BuildDir -C $Configuration --output-on-failure }
		$passed++
	}

	print Green "All $passed test runs passed."
}

# Synopsis: Register the ODBC driver on the system.
task install build, {
	if ($IsWindowsOS) {
		Install-WindowsDriver
	} elseif ($IsLinuxOS) {
		Install-LinuxDriver
	} else {
		throw 'Unsupported OS. Only Windows and Linux are supported.'
	}
}

# Synopsis: Unregister the ODBC driver from the system.
task uninstall {
	if ($IsWindowsOS) {
		Uninstall-WindowsDriver
	} elseif ($IsLinuxOS) {
		Uninstall-LinuxDriver
	} else {
		throw 'Unsupported OS. Only Windows and Linux are supported.'
	}
}

# Synopsis: Build the driver and tests.
task . build

#region Windows

function Install-WindowsDriver {
	$regBase = 'HKLM:\SOFTWARE\ODBC\ODBCINST.INI'
	$regPath = Join-Path $regBase $DriverName
	$driversPath = Join-Path $regBase 'ODBC Drivers'

	if (!(Test-Path $regPath)) {
		New-Item -Path $regBase -Name $DriverName -Force | Out-Null
	}

	Set-ItemProperty -Path $regPath -Name 'Driver' -Value $DriverPath -Type String -Force
	Set-ItemProperty -Path $regPath -Name 'Setup' -Value $DriverPath -Type String -Force
	Set-ItemProperty -Path $regPath -Name 'APILevel' -Value '1' -Type String -Force
	Set-ItemProperty -Path $regPath -Name 'ConnectFunctions' -Value 'YYY' -Type String -Force
	Set-ItemProperty -Path $regPath -Name 'DriverODBCVer' -Value '03.51' -Type String -Force
	Set-ItemProperty -Path $regPath -Name 'FileUsage' -Value '0' -Type String -Force
	Set-ItemProperty -Path $regPath -Name 'SQLLevel' -Value '1' -Type String -Force

	if (!(Test-Path $driversPath)) {
		New-Item -Path $regBase -Name 'ODBC Drivers' -Force | Out-Null
	}
	Set-ItemProperty -Path $driversPath -Name $DriverName -Value 'Installed' -Type String -Force

	print Green "Driver '$DriverName' registered: $DriverPath"
}

function Uninstall-WindowsDriver {
	$regBase = 'HKLM:\SOFTWARE\ODBC\ODBCINST.INI'
	$regPath = Join-Path $regBase $DriverName
	$driversPath = Join-Path $regBase 'ODBC Drivers'

	if (Test-Path $regPath) {
		Remove-Item -Path $regPath -Recurse -Force
	}
	if (Test-Path $driversPath) {
		Remove-ItemProperty -Path $driversPath -Name $DriverName -ErrorAction SilentlyContinue
	}

	print Green "Driver '$DriverName' unregistered."
}

#endregion

#region Linux

function Install-LinuxDriver {
	if (-not (Get-Command odbcinst -ErrorAction Ignore)) {
		throw 'odbcinst not found. Install unixODBC: sudo apt-get install unixodbc'
	}

	$driverAbsPath = (Resolve-Path $DriverPath).Path

	$tempIni = [System.IO.Path]::GetTempFileName()
	try {
		@"
[$DriverName]
Description = $DriverName
Driver = $driverAbsPath
Setup = $driverAbsPath
Threading = 0
FileUsage = 0
"@ | Set-Content -Path $tempIni

		exec { sudo odbcinst -i -d -f $tempIni -r }
	} finally {
		Remove-Item -Path $tempIni -Force -ErrorAction SilentlyContinue
	}

	print Green "Driver '$DriverName' registered: $driverAbsPath"
}

function Uninstall-LinuxDriver {
	if (-not (Get-Command odbcinst -ErrorAction Ignore)) {
		throw 'odbcinst not found. Install unixODBC: sudo apt-get install unixodbc'
	}

	exec { odbcinst -u -d -n $DriverName }

	print Green "Driver '$DriverName' unregistered."
}

#endregion
