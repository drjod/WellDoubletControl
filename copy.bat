@echo off

set simulator_folder=..\ogs_kb1
set wdc_folder=%simulator_folder%\Libs\WellDoubletControl

md %wdc_folder%

copy build\src\Release\wellDoubletControl.lib %wdc_folder%
copy src\*.h %wdc_folder%
copy build\wdc_config.h %wdc_folder%

