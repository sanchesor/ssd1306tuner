##################################
# stm32 (stm32f103c8t6) minimal example Makefile 
##################################

.PHONY: all clean install

# directories 

SRC_DIR = src
BIN_DIR = bin

LIB_BASE_DIR = lib\STM32F10x_StdPeriph_Driver

LIB_SRC_DIR = $(LIB_BASE_DIR)\src
LIB_INC_DIR = $(LIB_BASE_DIR)\inc
LIB_BIN_DIR = $(LIB_BASE_DIR)\bin

CMSIS_SRC_DIR = lib\CMSIS

INCLUDES = -I$(CMSIS_SRC_DIR) -I$(LIB_INC_DIR)
SYSTEM_LIBS = -L"C:\Program Files\SysGCC\arm-eabi\arm-eabi\lib" -L"C:\Program Files\SysGCC\arm-eabi\lib\gcc\arm-eabi\5.2.0"


# tool
CC = arm-eabi-gcc

# release configuration
CFLAGS  = -c -fno-common -Wall -O3 -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -fmessage-length=0 -ffunction-sections -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER 
LFLAGS = -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -TLinkerScript.ld -Wl,--gc-sections -lm


# targets
all: $(BIN_DIR)\main.elf

	
# common

COMMON_OBJS = \
$(BIN_DIR)\startup_stm32f10x_md.o \
$(BIN_DIR)\system_stm32f10x.o 

$(BIN_DIR):
	if not exist $(BIN_DIR) mkdir $(BIN_DIR)

$(BIN_DIR)\startup_stm32f10x_md.o: $(CMSIS_SRC_DIR)\startup_stm32f10x_md.S | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(BIN_DIR)\startup_stm32f10x_md.o $(CMSIS_SRC_DIR)\startup_stm32f10x_md.S
	
$(BIN_DIR)\system_stm32f10x.o: $(CMSIS_SRC_DIR)\system_stm32f10x.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(BIN_DIR)\system_stm32f10x.o $(CMSIS_SRC_DIR)\system_stm32f10x.c
	
	
# libs

_TMP_SLASH_LIB_SRCS = $(wildcard $(LIB_SRC_DIR)/*.c)
_TMP_SLASH_LIB_OBJS = $(patsubst $(LIB_SRC_DIR)/%.c,$(LIB_BIN_DIR)/%.o,$(_TMP_SLASH_LIB_SRCS))

LIB_SRCS = $(subst /,\,$(_TMP_SLASH_LIB_SRCS))
LIB_OBJS = $(subst /,\,$(_TMP_SLASH_LIB_OBJS))


$(LIB_BIN_DIR):
	if not exist $(LIB_BIN_DIR) mkdir $(LIB_BIN_DIR)
	
$(LIB_OBJS): $(LIB_BIN_DIR)\\%.o: $(LIB_SRC_DIR)\\%.c | $(LIB_BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $<

# src

USER_OBJS = \
$(BIN_DIR)\libssd1306.o \
$(BIN_DIR)\movingrect.o

	
$(BIN_DIR)\movingrect.o: $(SRC_DIR)\movingrect.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(BIN_DIR)\movingrect.o $(SRC_DIR)\movingrect.c

$(BIN_DIR)\libssd1306.o: $(SRC_DIR)\libssd1306.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(BIN_DIR)\libssd1306.o $(SRC_DIR)\libssd1306.c
	

$(BIN_DIR)\main.elf: $(BIN_DIR)\main.o $(COMMON_OBJS) $(LIB_OBJS) $(USER_OBJS)| $(BIN_DIR)
	$(CC) $(LFLAGS) -o $(BIN_DIR)\main.elf $(BIN_DIR)\main.o $(COMMON_OBJS) $(LIB_OBJS) $(USER_OBJS)


$(BIN_DIR)\main.o: $(SRC_DIR)\main.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(BIN_DIR)\main.o $(SRC_DIR)\main.c 
	

	
clean:
	if exist $(BIN_DIR)\*.o del $(BIN_DIR)\*.o 
	if exist $(BIN_DIR)\*.elf del $(BIN_DIR)\*.elf	
	if exist $(LIB_BIN_DIR)\*.o del $(LIB_BIN_DIR)\*.o 
	if exist $(BIN_DIR) rmdir $(BIN_DIR) /s /q
	if exist $(LIB_BIN_DIR) rmdir $(LIB_BIN_DIR) /s /q
 


IMAGE = $(BIN_DIR)\\main.elf
OPENOCD_DIR = f:\download\openocd-0.10.0
#OPENOCD_DIR = C:\Users\sanchesor\Downloads\openocd-0.10.0
OPENOCD_PROGRAM = "$(OPENOCD_DIR)\bin\openocd.exe"
OPENOCD_SCRIPTS = -s "$(OPENOCD_DIR)\scripts\interface" -s "$(OPENOCD_DIR)\scripts\target"
install: 
	$(OPENOCD_PROGRAM) $(OPENOCD_SCRIPTS) -f "stlink-v2.cfg" -f "stm32f1x.cfg" -c init -c "reset halt" -c "flash write_image erase $(IMAGE)" -c "verify_image $(IMAGE)" -c "reset" -c shutdown
	 
	 
	 
