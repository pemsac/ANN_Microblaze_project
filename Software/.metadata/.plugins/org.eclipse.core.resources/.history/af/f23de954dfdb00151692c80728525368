################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
ELFSIZE += \
ANN_Heartbeat_Sorter.elf.size \

ELFCHECK += \
ANN_Heartbeat_Sorter.elf.elfcheck \


# All Target
all: ANN_Heartbeat_Sorter.elf secondary-outputs

# Tool invocations
ANN_Heartbeat_Sorter.elf: $(OBJS) ../src/lscript.ld $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MicroBlaze gcc linker'
	mb-gcc -Wl,-T -Wl,../src/lscript.ld -L../../standalone_bsp/microblaze_0/lib -mxl-barrel-shift -mxl-pattern-compare -mcpu=v8.50.c -mno-xl-soft-mul -mhard-float -Wl,--no-relax -Wl,--gc-sections -o "ANN_Heartbeat_Sorter.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

ANN_Heartbeat_Sorter.elf.size: ANN_Heartbeat_Sorter.elf
	@echo 'Invoking: MicroBlaze Print Size'
	mb-size ANN_Heartbeat_Sorter.elf  |tee "ANN_Heartbeat_Sorter.elf.size"
	@echo 'Finished building: $@'
	@echo ' '

ANN_Heartbeat_Sorter.elf.elfcheck: ANN_Heartbeat_Sorter.elf
	@echo 'Invoking: Xilinx ELF Check'
	elfcheck ANN_Heartbeat_Sorter.elf -hw ../../Hardware_hw_platform/system.xml -pe microblaze_0  |tee "ANN_Heartbeat_Sorter.elf.elfcheck"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES)$(ELFSIZE)$(ELFCHECK)$(S_UPPER_DEPS) ANN_Heartbeat_Sorter.elf
	-@echo ' '

secondary-outputs: $(ELFSIZE) $(ELFCHECK)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
