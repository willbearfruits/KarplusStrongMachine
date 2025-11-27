# Diagnostics and Development Files

This directory contains diagnostic tools, test files, and development notes used during the creation of the Karplus-Strong Machine projects.

## Diagnostic Programs

- **AudioCodecDiagnostic.cpp** - Tests audio codec initialization timing
- **DiagnosticTest.cpp** - General hardware diagnostic
- **SerialDiagnostic.cpp** - Serial output debugging
- **TestTone.cpp** - Simple tone generator for audio testing

## Development Versions

- **KarplusStrongMachine_Fixed.cpp** - Version with audio codec timing fixes
- **KarplusStrongMachine_Monitored.cpp** - Version with serial monitoring
- **KarplusStrongMachine_WithOLED.cpp** - OLED integration experiments

## Build Files

- **Makefile_*** - Various Makefile configurations for different test builds
- **test-diagnostic.sh** - Script to test diagnostic builds
- **test-fixed.sh** - Script to test fixed version
- **monitor.sh** - Serial monitoring script

## Documentation

- **AUDIO_TROUBLESHOOTING_REPORT.md** - Audio codec debugging notes
- **INIT_TIMING_DIAGRAM.txt** - Initialization timing analysis
- **OLED_INTEGRATION_SUMMARY.txt** - OLED display integration notes
- **KEY_CODE_SECTIONS.txt** - Important code section references

## Note

These files are kept for reference and debugging purposes. For production code, use the main project files in the parent directory.
