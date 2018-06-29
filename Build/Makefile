#----------------------------------------------------------------------------
#
#----------------------------------------------------------------------------

NXP_BSTUDIO = C:/NXP/bstudio_nxp

#----------------------------------------------------------------------------
# Application target name

TARGET = TAGxx

#----------------------------------------------------------------------------
# Default target device is the JN5148

JENNIC_CHIP ?= JN5168

#----------------------------------------------------------------------------
# Development kit target hardware

JENNIC_PCB ?= DEVKIT4

#----------------------------------------------------------------------------
# Default SDK is the IEEE802.15.4 SDK

JENNIC_SDK ?= JN-SW-4163

#----------------------------------------------------------------------------
# Select the network stack (e.g. MAC, ZBPRO)

JENNIC_STACK ?= MAC

#----------------------------------------------------------------------------
# Debug options define DEBUG for HW debug
#DEBUG ?=HW
# Define which UART to use for debug
#DEBUG_PORT ?= UART1

#----------------------------------------------------------------------------
# Define TRACE to use with DBG module
#TRACE ?=1

#----------------------------------------------------------------------------
# Path definitions
# Select definitions for either single or multiple targets
# Use if application directory contains multiple targets

#SDK_BASE_DIR   	 	= $(abspath ../../../..)

SDK_BASE_DIR        = $(NXP_BSTUDIO)/sdk/$(JENNIC_SDK)/


APP_BASE       	 	= $(abspath ..)


COMPONENTS_BASE_DIR    	= $(SDK_BASE_DIR)/Components

APP_BLD_DIR		= $(APP_BASE)/Build
APP_SRC_DIR 	        = $(APP_BASE)/MAIN
APP_COMMON_5168_DIR 	= $(APP_BASE)/Common
#APP_COMMON_PRJX_DIR  	= $(APP_BASE)/../ISIB3/MAIN

# Use if application directory contains single target
#SDK_BASE_DIR         	= $(abspath ../../..)
#APP_BASE            		= $(abspath ..)
#APP_BLD_DIR          	= $(APP_BASE)/Build
#APP_SRC_DIR          	= $(APP_BASE)/Source

#----------------------------------------------------------------------------
# Application Source files

# Note: Path to source file is found using vpath below, so only .c filename is required
APPSRC += main.c
APPSRC += jnx_system.c
APPSRC += jnx_common.c
APPSRC += jnx_console.c
APPSRC += jnx_utilites.c
APPSRC += jnx_leds2.c
APPSRC += xtag_protocol.c
APPSRC += commands.c
APPSRC += jnx_eeprom.c
APPSRC += tof.c
#APPSRC += air_diag.c
#APPSRC += jnx_button.c
#APPSRC += vlf_tx.c
APPSRC += jnx_app_buff.c
APPSRC += crc32.c
APPSRC += cfg_tagx.c
#APPSRC += uart_data.c
#APPSRC += lcd_driver.c

##############################################################################
OFLAGS = -O2

#----------------------------------------------------------------------------
CFLAGS += -std=gnu99
CFLAGS += -Wundef
CFLAGS += -fms-extensions
CFLAGS += -Wall
CFLAGS += -Wno-multichar
CFLAGS += -Wcast-align
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wmissing-prototypes
CFLAGS += -Wredundant-decls
CFLAGS += -Wnested-externs
CFLAGS += -Winline
#CFLAGS +=  -Wconversion
CFLAGS += -W
CFLAGS += -Winline
CFALGS += -pedantic
//CFALGS += -S
//CFALGS += -save-temps
//CFALGS += -Wa
//CFALGS += -a
//CFALGS += -ad


#----------------------------------------------------------------------------
# Additional Application Source directories
# Define any additional application directories outside the application directory
# e.g. for AppQueueApi

#ADDITIONAL_SRC_DIR += $(COMPONENTS_BASE_DIR)/AppQueueApi/Source

#----------------------------------------------------------------------------
# Standard Application header search paths

INCFLAGS += -I$(APP_SRC_DIR)
INCFLAGS += -I$(APP_SRC_DIR)/..
INCFLAGS += -I$(APP_SRC_DIR)/..
INCFLAGS += -I$(APP_COMMON_5168_DIR)
INCFLAGS += -I$(APP_COMMON_5168_DIR)/Include
INCFLAGS += -I$(APP_COMMON_PRJX_DIR)

# Application specific include files
#INCFLAGS += -I$(COMPONENTS_BASE_DIR)/JennicLogo/Include
INCFLAGS += -I$(COMPONENTS_BASE_DIR)/Utilities/Include
INCFLAGS += -I$(COMPONENTS_BASE_DIR)/MicroSpecific/Include
INCFLAGS += -I$(COMPONENTS_BASE_DIR)/ProductionTestApi/Include


#CFLAGS   += -Wno-multichar

#----------------------------------------------------------------------------
# Application libraries
# Specify additional Component libraries

APPLIBS += TOF

# Specify additional Component libraries
LDLIBS += JPT_${JENNIC_CHIP}

# Define a default APP_ID from BootLoader app = 0
APP_ID ?= 0

# Define the _application_id for the correct value when linking
LDFLAGS += -Wl,--defsym,_application_id=$(APP_ID)

#Define the vLoadBootImage function call in the Bootloader
ifeq ($(JENNIC_CHIP),JN5168)
LDFLAGS += -Wl,--defsym,vLoadBootImage=0x000009e3
endif
ifeq ($(JENNIC_CHIP),JN5164)
LDFLAGS += -Wl,--defsym,vLoadBootImage=0x000009e3
endif
ifeq ($(JENNIC_CHIP),JN5161)
LDFLAGS += -Wl,--defsym,vLoadBootImage=0x000009e3
endif
ifeq ($(JENNIC_CHIP),JN5169)
LDFLAGS += -Wl,--defsym,vLoadBootImage=0x00001a76
endif

#----------------------------------------------------------------------------
# Configure for the selected chip or chip family

include $(SDK_BASE_DIR)/Chip/Common/Build/config.mk
include $(SDK_BASE_DIR)/Platform/Common/Build/Config.mk
include $(SDK_BASE_DIR)/Stack/Common/Build/Config.mk

#----------------------------------------------------------------------------

APPOBJS = $(APPSRC:.c=.o)

#----------------------------------------------------------------------------
# Application dynamic dependencies

APPDEPS = $(APPOBJS:.o=.d)

#----------------------------------------------------------------------------
# Linker
# Add application libraries before chip specific libraries to linker so
# symbols are resolved correctly (i.e. ordering is significant for GCC)

LDLIBS := $(addsuffix _$(JENNIC_CHIP_FAMILY),$(APPLIBS)) $(LDLIBS)

#----------------------------------------------------------------------------
# Dependency rules

.PHONY: all clean
# Path to directories containing application source
vpath % $(APP_SRC_DIR):$(APP_COMMON_5168_DIR):$(APP_COMMON_PRJX_DIR):$(APP_COMMON_ISIB_DIR)


all: $(TARGET)$(BIN_SUFFIX).bin

-include $(APPDEPS)
%.d:
	rm -f $*.o


%.o: %.S
	$(info Assembling $< ...)
	$(CC) -c -î $(subst Source,Build,$@) $(CFLAGS)  $(INCFLAGS) $< -MD -MF $*.d -MP $(OFLAGS)
	@echo

%.o: %.c
	$(info Compiling $< ...)
	$(CC) -c -o $(subst Source,Build,$@) $(CFLAGS) -Wno-multichar $(INCFLAGS) $< -MD -MF $*.d -MP $(OFLAGS)
	@echo

$(TARGET)$(BIN_SUFFIX).elf: $(APPOBJS) $(addsuffix _$(JENNIC_CHIP_FAMILY).a,$(addprefix $(COMPONENTS_BASE_DIR)/Library/lib,$(APPLIBS)))
	$(info Linking $@ ...)
	$(CC) -Wl,--gc-sections -Wl,-u_AppColdStart -Wl,-u_AppWarmStart $(LDFLAGS) -T$(LINKCMD) -o $@ $(APPOBJS) -Wl,--start-group  $(addprefix -l,$(LDLIBS)) -Wl,--end-group -Wl,-Map,$(TARGET)$(BIN_SUFFIX).map
	${SIZE} $@
	@echo

$(TARGET)$(BIN_SUFFIX).bin: $(TARGET)$(BIN_SUFFIX).elf
	$(info Generating binary ...)
	$(OBJCOPY) -S -O binary $< $@

#----------------------------------------------------------------------------
clean:
	rm -f $(APPOBJS) $(APPDEPS) $(TARGET)*.bin $(TARGET)*.elf $(TARGET)*.map


