# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [v3.0](https://github.com/sigfox-tech-radio/sigfox-ep-lib/releases/tag/v3.0) - 17 Apr 2026

### Added

* Add **compilation flag** to disable the SSID usage in filtering functions. In case the SSID field is not scanned by the WiFi chip, disabling the flag will significantly reduces the memory footprint, at the expense of the geolocation success rate (mobile phones and empty SSIDs will not be removed anymore).

### Changed

* Use **byte array format for MAC addresses** instead of strings to simplify the interface and reduce the memory footprint.

## [v2.0](https://github.com/sigfox-tech-radio/sigfox-ep-addon-aw/releases/tag/v2.0) - 18 Mar 2025

### General

* First version of the new Sigfox EP Atlas WiFi addon.

### Added

* Sigfox Atlas WiFi **payload builder**.
* **Mandatory MAC address filtering**.
* **Optional MAC address filtering** and **sorting**.
