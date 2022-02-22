# Makefile for ESP8266 projects
#
# Thanks to:
# - zarya
# - Jeroen Domburg (Sprite_tm)
# - Christian Klippel (mamalala)
# - Tommie Gannert (tommie)
#
# Changelog:
# - 2014-10-06: Changed the variables to include the header file directory
# - 2014-10-06: Added global var for the Xtensa tool root
# - 2014-11-23: Updated for SDK 0.9.3
# - 2014-12-25: Replaced esptool by esptool.py
# - 2015-09-17: adopted to RHTS project (hmueller)
# - 2017-05-27: adopted to Windsensor project (hmueller)
# - 2017-06-07: rewritten to support OTA and various flash sizes
# - 2017-12-09: adopted to rcswitch project (hmueller)

VERBOSE		= 0
FLAVOR		= release # debug or release

# name for the target project
TARGET		= rcplugs

# Output directors to store intermediate compiled files
# relative to the project directory
BUILD_BASE	= build
FW_BASE		= firmware

# Root of Espressif toolset (compiler, SDK)
ESP_ROOT	= /opt/Espressif

# base directory for the compiler
COMPILE		= gcc
XTENSA_TOOLS_ROOT ?= $(ESP_ROOT)/xtensa-lx106-elf/bin

# base directory of the ESP8266 SDK package, absolute
SDK_BASE	?= $(ESP_ROOT)/ESP8266_NONOS_SDK-3.0.4

# esptool.py path and port
#ESPTOOL		?= $(XTENSA_TOOLS_ROOT)/esptool.py
ESPTOOL		?= esptool.py
#ESPPORT		?= /dev/tty.SLAB_USBtoUART
#ESPPORT		?= /dev/tty.usbserial-1430
ESPPORT		?= /dev/tty.usbserial

# gen_appbin.py Espressif's generate bin script
GEN_APPBIN	?= python2 $(SDK_BASE)/tools/gen_appbin.py

# Server and location on update server for new firmware
UPDATE_SERVER	:= $(shell grep "^\#define OTA_HOST" include/user_config.h | sed 's/.*"\(.*\)".*/\1/')
UPDATE_PATH		:= /var/www/html/esp8266fw/$(TARGET)

# MQTT settings to publish new firmware to target
MQTT_PUB		:= mosquitto_pub
MQTT_HOST		:= $(shell grep '^\#define MQTT_HOST' include/user_config.h | sed 's/.*"\(.*\)".*/\1/')
MQTT_USER		:= $(shell grep '^\#define MQTT_USER' include/user_config.h | sed 's/.*"\(.*\)".*/\1/')
MQTT_PASS		:= $(shell grep '^\#define MQTT_PASS' include/user_config.h | sed 's/.*"\(.*\)".*/\1/')

# app version: 0 = non-OTA, 1 = OTA user1 (auto builds user2), 2 = OTA user2
APP			?= 1
# OTA boot version: old=boot_v1.1 or new=boot_v1.2+
BOOT		?= new
# SPI_MODE: 0=QIO, 1=QOUT, 2=DIO, 3=DOUT
SPI_MODE	?= 2
# SPI_SPEED frequency divider: 0=40MHz, 1=26.7MHz, 2=20MHz, 15=80MHz
SPI_SPEED	?= 0

# flash size and map:
# 0= 512KB( 256KB+ 256KB)
# 2=1024KB( 512KB+ 512KB)
# 3=2048KB( 512KB+ 512KB)
# 4=4096KB( 512KB+ 512KB)
# 5=2048KB(1024KB+1024KB)
# 6=4096KB(1024KB+1024KB)
# 7=4096KB(2048KB+2048KB) not supported, just for compatible with nodeMCU board
# 8=8192KB(1024KB+1024KB)
# 9=16384KB(1024KB+1024KB)
SIZE_MAP	?= 4
FLASH_SIZE	?= 4096

# which modules (subdirectories) of the project to include in compiling
MODULES		= common modules user rc-switch
EXTRA_INCDIR	= include common modules rc-switch $(ESP_ROOT)/local/include $(ESP_ROOT)/local/include/wiringESP

# libraries used in this project, mainly provided by the SDK
LIBS		= c gcc hal pp phy net80211 lwip wpa wps crypto upgrade main mbedtls mqtt wiringESP
EXTRA_LIBDIR	= $(ESP_ROOT)/local/lib

# compiler flags using during compilation of source files
ifeq ($(FLAVOR),debug)
    DEFINES	+= -DDEBUG_ON
    CFLAGS	+= -g
endif
DEFINES		+= -D__ets__ -DICACHE_FLASH -DSPI_FLASH_SIZE_MAP=$(SIZE_MAP)
CFLAGS		+= -Os -std=c99 -Wpointer-arith -Wundef -Werror -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals $(DEFINES)
CXXFLAGS	= $(subst -std=c99,-std=c++11,$(CFLAGS)) -fno-rtti -fno-exceptions

# linker flags used to generate the main object file
LDFLAGS		= -nostdlib -u call_user_start -Wl,-static -Wl,--gc-sections
#-Wl,--no-check-sections

# we create two different files for uploading into the flash
# these are the names and options to generate them
FW_ADDR_1	= 0x00000
FW_ADDR_2	= 0x10000
FW_ADDR_OTA	= 0x01000
FW_ADDR_OTA2	= 0x81000
ifeq ($(APP), 2)
    FW_ADDR_OTA	= $(FW_ADDR_OTA2)
endif
FW_ADDR_CA_CERT	= 0x77000
# 512KB Flash
ifeq ($(FLASH_SIZE), 512)
    FW_ADDR_ESP_INIT	= 0x7C000
    FW_ADDR_ESP_BLANK	= 0x7E000
    # blank should be flashed to 0xfe000 also ...
endif
# 4096KB Flash
ifeq ($(FLASH_SIZE), 4096)
    FW_ADDR_ESP_INIT	= 0x3FC000
    FW_ADDR_ESP_BLANK	= 0x3FE000
endif

FW_FILE_1	:= $(addprefix $(FW_BASE)/,$(FW_ADDR_1).bin)
FW_FILE_2	:= $(addprefix $(FW_BASE)/,$(FW_ADDR_2).bin)
FW_FILE_ESP_INIT	:= $(SDK_BASE)/bin/esp_init_data_default_v05.bin
FW_FILE_ESP_BLANK	:= $(SDK_BASE)/bin/blank.bin
FW_FILE_ESP_BOOT	:= $(SDK_BASE)/bin/boot_v1.7.bin
FW_FILE_OTA	:= $(FW_BASE)/user$(APP).$(FLASH_SIZE).$(BOOT).$(SIZE_MAP).bin
FW_FILE_OTA2	:= $(FW_BASE)/user2.$(FLASH_SIZE).$(BOOT).$(SIZE_MAP).bin
FW_FILE_CA_CERT	:= bin/esp_ca_cert.bin


####
#### no user configurable options below here
####
app_version	:= $(shell grep "^\#define APP_VERSION" include/user_config.h | sed "s/\#define APP_VERSION "//)
system_id	?= $(shell grep "^\#define HOMA_SYSTEM_ID" include/user_config.h | sed 's/.*"\(.*\)"/\1/')

# set tools to use as compiler, librarian and linker
CC			:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
CXX			:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-g++
AR			:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-ar
LD			:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
OBJCOPY		:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-objcopy

# various paths from the SDK used in this project
SDK_LIBDIR	:= lib
SDK_LDDIR	:= ld
SDK_INCDIR	:= include

SRC_DIR		:= $(MODULES)
BUILD_DIR	:= $(addprefix $(BUILD_BASE)/,$(MODULES))

SDK_LIBDIR	:= $(addprefix $(SDK_BASE)/,$(SDK_LIBDIR))
SDK_INCDIR	:= $(addprefix -I$(SDK_BASE)/,$(SDK_INCDIR))

SRC			:= $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c) $(wildcard $(sdir)/*.cpp))
OBJ			:= $(patsubst %.c,$(BUILD_BASE)/%.o,$(SRC))
OBJ			:= $(patsubst %.cpp,$(BUILD_BASE)/%.o,$(OBJ))
DEP			:= $(patsubst %.c,$(BUILD_BASE)/%.d,$(SRC))
DEP			:= $(patsubst %.cpp,$(BUILD_BASE)/%.d,$(DEP))
LIBS		:= $(addprefix -l,$(LIBS))
APP_AR		:= $(addprefix $(BUILD_BASE)/,$(TARGET)_app.a)
TARGET_OUT	:= $(addprefix $(BUILD_BASE)/,$(TARGET).out)

INCDIR		:= $(addprefix -I,$(SRC_DIR))
EXTRA_INCDIR	:= $(addprefix -I,$(EXTRA_INCDIR))
MODULE_INCDIR	:= $(addsuffix /include,$(INCDIR))

EXTRA_LIBDIR	:= $(addprefix -L,$(EXTRA_LIBDIR))

# linker script used by the linker
LD_SCRIPT	= eagle.app.v6.ld
ifneq ($(APP),0)
    ifneq ($(findstring $(SIZE_MAP),  6  8  9),)
        LD_SCRIPT = eagle.app.v6.$(BOOT).2048.ld
        # FW_FILE_ESP_BOOT at least v1.4 and +
    else ifeq ($(SIZE_MAP), 5)
        LD_SCRIPT = eagle.app.v6.$(BOOT).2048.ld
        # FW_FILE_ESP_BOOT at least v1.4 and +
    else ifeq ($(SIZE_MAP), 4)
        LD_SCRIPT = eagle.app.v6.$(BOOT).1024.app$(APP).ld
        # FW_FILE_ESP_BOOT at least v1.2 and +
    else ifeq ($(SIZE_MAP), 3)
        LD_SCRIPT = eagle.app.v6.$(BOOT).1024.app$(APP).ld
    else ifeq ($(SIZE_MAP), 2)
        LD_SCRIPT = eagle.app.v6.$(BOOT).1024.app$(APP).ld
    else ifeq ($(SIZE_MAP), 0)
        LD_SCRIPT = eagle.app.v6.$(BOOT).512.app$(APP).ld
    endif
endif
LD_SCRIPT	:= $(addprefix -T$(SDK_BASE)/$(SDK_LDDIR)/,$(LD_SCRIPT))

ifeq ("$(VERBOSE)", "1")
    Q :=
    #vecho := @true
    vecho := @echo
else
    Q := @
    vecho := @echo
endif

vpath %.c $(SRC_DIR)
vpath %.cpp $(SRC_DIR)

define compile-objects
$1/%.d: %.c
	$(vecho) "GEN $$@"
	$(Q) $(CC) $(INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CFLAGS) -MM $$< | sed 's;\(.*\)\.o[ :]*;$$(dir $$@)\1.o $$@ : ;g' > $$@
$1/%.d: %.cpp
	$(vecho) "GEN $$@"
	$(Q) $(CXX) $(INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CXXFLAGS) -MM $$< | sed 's;\(.*\)\.o[ :]*;$$(dir $$@)\1.o $$@ : ;g' > $$@
$1/%.o: %.c
	$(vecho) "CC $$<"
	$(Q) $(CC) $(INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CFLAGS) -c $$< -o $$@
$1/%.o: %.cpp
	$(vecho) "C+ $$<"
	$(Q) $(CXX) $(INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CXXFLAGS) -c $$< -o $$@
endef

.PHONY: all checkdirs flash clean connect publish

ifeq ($(APP), 0)
all: checkdirs $(DEP) $(TARGET_OUT) $(FW_FILE_1) $(FW_FILE_2)
else
all: checkdirs $(DEP) $(TARGET_OUT) $(FW_FILE_OTA)
endif

checkdirs: $(BUILD_DIR) $(FW_BASE)

$(BUILD_DIR):
	$(info CREATE $@)
	$(Q) mkdir -p $@

$(FW_BASE):
	$(info CREATE $@)
	$(Q) mkdir -p $@

$(FW_BASE)/%.bin: $(TARGET_OUT) | $(FW_BASE)
	$(info FW $(FW_BASE)/)
    ifeq ($(APP), 0)
	  $(Q) $(ESPTOOL) elf2image -o $(FW_BASE)/ -ff 40m -fs 4MB $(TARGET_OUT)
    else ifeq ($(APP), 9999)
#	  # does not work, app cores after boot loader
	  $(Q) $(ESPTOOL) elf2image -o $(FW_FILE_OTA) -ff 40m -fs 4MB --version=2 $(TARGET_OUT)
    else
	  $(Q) $(OBJCOPY) --only-section .text -O binary $< eagle.app.v6.text.bin
	  $(Q) $(OBJCOPY) --only-section .data -O binary $< eagle.app.v6.data.bin
	  $(Q) $(OBJCOPY) --only-section .rodata -O binary $< eagle.app.v6.rodata.bin
	  $(Q) $(OBJCOPY) --only-section .irom0.text -O binary $< eagle.app.v6.irom0text.bin
      ifeq ($(BOOT), old)
	    $(Q) env COMPILE=$(COMPILE) $(GEN_APPBIN) $< 1 $(SPI_MODE) $(SPI_SPEED) $(SIZE_MAP) $(APP)
      else
	    $(Q) env COMPILE=$(COMPILE) $(GEN_APPBIN) $< 2 $(SPI_MODE) $(SPI_SPEED) $(SIZE_MAP) $(APP)
      endif
	  $(Q) mv eagle.app.flash.bin $(FW_FILE_OTA)
	  $(Q) rm eagle.app.v6.*
	  $(vecho) "FW $(FW_FILE_OTA) generated. Flash size $(FLASH_SIZE)KB."
	  $(vecho) "$(notdir $(FW_FILE_ESP_BOOT))---------->0x00000"
	  $(vecho) "$(notdir $(FW_FILE_OTA))--->$(FW_ADDR_OTA)"
	  scp $(FW_FILE_OTA) $(UPDATE_SERVER):$(UPDATE_PATH)
      ifeq ($(APP), 1)
	    @rm $(TARGET_OUT)
	    $(MAKE) APP=2
      else ifeq ($(APP), 2)
#	    # user1...bin is now older than $(TARGET_OUT), fix that otherwise make will always redo this rule
	    $(Q) touch $(FW_BASE)/*.bin
      endif
    endif

$(TARGET_OUT): $(APP_AR)
	$(info LD $@)
	$(Q) $(LD) -L$(SDK_LIBDIR) $(EXTRA_LIBDIR) $(LD_SCRIPT) $(LDFLAGS) -Wl,--start-group $(LIBS) $(APP_AR) -Wl,--end-group -o $@

$(APP_AR): $(OBJ)
	$(info AR $@)
	$(Q) $(AR) cru $@ $^

ifeq ($(APP), 0)
flash: $(FW_FILE_1) $(FW_FILE_2)
	# write esp_init_data_default and clear default system parameter values
	$(ESPTOOL) --port $(ESPPORT) write_flash -fm dio $(FW_ADDR_1) $(FW_FILE_1) $(FW_ADDR_2) $(FW_FILE_2) $(FW_ADDR_ESP_INIT) $(FW_FILE_ESP_INIT) $(FW_ADDR_ESP_BLANK) $(FW_FILE_ESP_BLANK)
	# clear user parameter also
	#$(ESPTOOL) --port $(ESPPORT) write_flash -fm dio $(FW_ADDR_1) $(FW_FILE_1) $(FW_ADDR_2) $(FW_FILE_2) 0x3D000 $(FW_FILE_ESP_BLANK)
	# only write compiled code
	#$(ESPTOOL) --port $(ESPPORT) write_flash -fm dio $(FW_ADDR_1) $(FW_FILE_1) $(FW_ADDR_2) $(FW_FILE_2)
else ifeq ($(APP), 1)
flash: $(FW_FILE_OTA)
	# write esp_init_data_default and clear default system parameter values
	$(ESPTOOL) --port $(ESPPORT) write_flash -fm dio 0x00000 $(FW_FILE_ESP_BOOT) $(FW_ADDR_OTA) $(FW_FILE_OTA) $(FW_ADDR_OTA2) $(FW_FILE_OTA2) $(FW_ADDR_ESP_INIT) $(FW_FILE_ESP_INIT) $(FW_ADDR_ESP_BLANK) $(FW_FILE_ESP_BLANK) $(FW_ADDR_CA_CERT) $(FW_FILE_CA_CERT)
	# only write compiled code
	#$(ESPTOOL) --port $(ESPPORT) write_flash -fm dio $(FW_ADDR_OTA) $(FW_FILE_OTA) $(FW_ADDR_OTA2) $(FW_FILE_OTA2)
	# only write ca cert file
	#$(ESPTOOL) --port $(ESPPORT) write_flash -fm dio $(FW_ADDR_CA_CERT) $(FW_FILE_CA_CERT)
endif

connect:
	picocom -b 74880 --omap crcrlf $(ESPPORT)

publish: all
ifeq ($(system_id),)
	$(Q) echo usage: make publish system_id=\<HOMA_SYSTEM_ID\>
else
	$(MQTT_PUB) -h $(MQTT_HOST) -u "$(MQTT_USER)" -P "$(MQTT_PASS)" -q 2 -t "/sys/$(system_id)/server_version" -m "$(app_version)"
endif

clean:
	$(Q) rm -rf $(FW_BASE) $(BUILD_BASE)

# include deps only if exist (after first run)
ifeq ($(wildcard $(DEP)),$(DEP))
ifneq ($(filter clean,$(MAKECMDGOALS)),clean)
include $(DEP)
endif
endif

$(foreach bdir,$(BUILD_DIR),$(eval $(call compile-objects,$(bdir))))
