################################
##        Program config
################################
PROJECT_NAME    = LANSS_lab1
BUILD_DIR 		= build
SOURCE_DIR 		= .
INC_DIR 		= .
OUT_DIR 		= out

################################
##        Source config
################################
SOURCE          = 
SOURCE          += main.cpp
SOURCE          += TcpServ.cpp
SOURCE          += sock_thread_handler.cpp

################################
##        Flags config
################################
CFLAGS          =
CFLAGS          += -std=c++11
CFLAGS          += -Wall -Wpedantic -Werror

################################
##        Print config
################################
LDPRINT         = "Link   $@"
CCPRINT         = "Build  $<"

################################
##        Auto config
################################
OBJECTS         = $(addprefix $(BUILD_DIR)/, $(SOURCE:.cpp=.o))
CFLAGS          += -I $(INC_DIR)
PROJECT			= $(OUT_DIR)/$(PROJECT_NAME)

################################
##        Make config
################################
.DEFAULT_GOAL := all

################################
##           Targets
################################
all: clean build

build: $(PROJECT)

$(PROJECT): $(OBJECTS)
	@mkdir -p $(dir $@)
	@echo $(LDPRINT)
	@$(CC) -o $@ $(OBJECTS)

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo $(CCPRINT)
	@$(CC) -c $(CFLAGS) $< -o $@

clean:
	@rm -rf $(BUILD_DIR)
	@rm -rf $(OUT_DIR)

run:
	@$(PROJECT)
