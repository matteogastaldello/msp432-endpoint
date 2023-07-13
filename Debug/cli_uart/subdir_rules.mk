################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
cli_uart/%.obj: ../cli_uart/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/Applications/ti/ccs1200/ccs/tools/compiler/ti-cgt-arm_20.2.6.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="/Applications/ti/ccs1200/ccs/ccs_base/arm/include" --include_path="/Users/matteo/ti/simplelink_msp432p4_sdk_3_40_01_02/source" --include_path="/Users/matteo/ti/simplelink_msp432p4_sdk_3_40_01_02/source/ti/posix/ccs" --include_path="/Applications/ti/ccs1200/ccs/ccs_base/arm/include/CMSIS" --include_path="/Users/matteo/Documents/uni/iot/git/embedded-project/msp432-code" --include_path="/Users/matteo/Documents/uni/iot/git/embedded-project/msp432-code" --include_path="/Users/matteo/Documents/uni/iot/git/embedded-project/msp432-code/simplelink/include" --include_path="/Users/matteo/Documents/uni/iot/git/embedded-project/msp432-code/simplelink/source" --include_path="/Users/matteo/Documents/uni/iot/git/embedded-project/msp432-code/NetworkUtils" --include_path="/Users/matteo/Documents/uni/iot/git/embedded-project/msp432-code/board" --include_path="/Users/matteo/Documents/uni/iot/git/embedded-project/msp432-code/cli_uart" --include_path="/Users/matteo/Documents/uni/iot/git/embedded-project/msp432-code/driverlib/MSP432P4xx" --include_path="/Users/matteo/Documents/uni/iot/git/embedded-project/msp432-code/spi_cc3100" --include_path="/Users/matteo/Documents/uni/iot/git/embedded-project/msp432-code/uart_cc3100" --include_path="/Applications/ti/ccs1200/ccs/tools/compiler/ti-cgt-arm_20.2.6.LTS/include" --advice:power=all --define=__MSP432P401R__ --define=ccs --define=_USE_CLI_ -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="cli_uart/$(basename $(<F)).d_raw" --obj_directory="cli_uart" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


