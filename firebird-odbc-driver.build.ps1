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

# Synopsis: Run the test suite.
task test build, {
	exec { ctest --test-dir $BuildDir -C $Configuration --output-on-failure }
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
