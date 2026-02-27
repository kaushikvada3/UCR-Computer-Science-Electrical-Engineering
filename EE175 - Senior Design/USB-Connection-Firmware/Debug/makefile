################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include USB_DEVICE/Target/subdir.mk
-include USB_DEVICE/App/subdir.mk
-include STM32CubeIDE/Debug/Middlewares/USB_Device_Library/subdir.mk
-include STM32CubeIDE/Debug/Drivers/STM32F3xx_HAL_Driver/subdir.mk
-include STM32CubeIDE/Debug/Drivers/CMSIS/subdir.mk
-include STM32CubeIDE/Debug/Application/User/USB_DEVICE/Target/subdir.mk
-include STM32CubeIDE/Debug/Application/User/USB_DEVICE/App/subdir.mk
-include STM32CubeIDE/Debug/Application/User/Startup/subdir.mk
-include STM32CubeIDE/Debug/Application/User/Core/subdir.mk
-include STM32CubeIDE/Debug/subdir.mk
-include STM32CubeIDE/Application/User/Startup/subdir.mk
-include STM32CubeIDE/Application/User/Core/subdir.mk
-include Middlewares/ST/STM32_USB_Device_Library/Core/Src/subdir.mk
-include Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/subdir.mk
-include Drivers/STM32F3xx_HAL_Driver/Src/subdir.mk
-include Core/Src/subdir.mk
-include .metadata/.plugins/org.eclipse.cdt.make.core/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := USB-Connection-Firmware
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
USB-Connection-Firmware.elf \

MAP_FILES += \
USB-Connection-Firmware.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
USB-Connection-Firmware.list \


# All Target
all: main-build

# Main-build Target
main-build: USB-Connection-Firmware.elf secondary-outputs

# Tool invocations
USB-Connection-Firmware.elf USB-Connection-Firmware.map: $(OBJS) $(USER_OBJS) /Users/kaushikvada/STM32CubeIDE/workspace_2.0.0/USB-Connection-Firmware/STM32F303RCTX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "USB-Connection-Firmware.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m4 -T"/Users/kaushikvada/STM32CubeIDE/workspace_2.0.0/USB-Connection-Firmware/STM32F303RCTX_FLASH.ld" --specs=nosys.specs -Wl,-Map="USB-Connection-Firmware.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -u _printf_float -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

USB-Connection-Firmware.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "USB-Connection-Firmware.list"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) USB-Connection-Firmware.elf USB-Connection-Firmware.list USB-Connection-Firmware.map default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
