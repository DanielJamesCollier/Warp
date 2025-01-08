call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

:: Compiler Flags Explanation:
:: /permissive-          - Enforces standard C++ conformance, disabling Microsoft-specific extensions.
:: /GS                   - Enables security checks to detect buffer overruns during runtime.
:: /W3                   - Sets warning level 3 (moderate warnings), suitable for general development.
:: /Zc:wchar_t           - Treats `wchar_t` as a distinct native type instead of a typedef.
:: /Zi                   - Generates debugging information in the program database (PDB) format.
:: /Gm-                  - Disables minimal rebuild, which can improve build consistency.
:: /Od                   - Disables optimization for easier debugging.
:: /sdl                  - Enables additional security checks during code generation.
:: /Zc:inline            - Enforces standard inline behavior, affecting how inline functions are handled.
:: /fp:precise           - Ensures strict floating-point model for consistent results across platforms.
:: /errorReport:prompt   - Prompts to send error reports when internal compiler errors occur.
:: /WX-                  - Treats warnings as warnings, not errors (warnings won’t stop the build).
:: /Zc:forScope          - Enforces standard scoping rules for `for` loop variables.
:: /RTC1                 - Enables runtime checks for uninitialized variables and other errors.
:: /Gd                   - Sets __cdecl as the default calling convention for functions.
:: /MTd                  - Links the debug version of the multithreaded static C runtime library.
:: /FC                   - Displays full paths in diagnostic output.
:: /EHsc                 - Enables standard exception handling model (catch and throw).
:: /nologo               - Suppresses the display of the startup banner and copyright message.
:: /diagnostics:column   - Shows the column number of errors/warnings for easier identification.
:: /std:c17              - Specifies the C17 standard for compilation.
:: /fsanitize=address    - Enables AddressSanitizer to detect memory corruption issues.
:: /guard:cf             - Enables control flow guard to mitigate certain types of attacks.
:: /analyze              - Runs code analysis to detect potential issues during compilation.
:: /Qspectre             - Mitigates certain Spectre variant 1 security vulnerabilities.
set DEBUG_FLAGS=/permissive- /GS /W3 /Zc:wchar_t /Zi /Gm- /Od /sdl /Zc:inline /fp:precise /errorReport:prompt /WX- /Zc:forScope /RTC1 /Gd /MTd /FC /EHsc /nologo /diagnostics:column /std:c17 /guard:cf /analyze /Qspectre

set LINKER_FLAGS= /DEBUG:FULL /INCREMENTAL:NO
