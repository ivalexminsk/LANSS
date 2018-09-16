################################
##        Program config
################################
PROJECT_NAME_CLIENT = LANSS_lab1_client
PROJECT_NAME_SERVER = LANSS_lab1_server
BUILD_DIR 		= build
SOURCE_DIR 		= .
INC_DIR 		= . ../bitsery/include/
OUT_DIR 		= out

################################
##        Source config
################################
SOURCE          = 
SOURCE_CLIENT	=
SOURCE_SERVER	=

SOURCE_CLIENT   += main_client.cpp
SOURCE_SERVER   += main.cpp
SOURCE_CLIENT   += TcpClient.cpp
SOURCE_SERVER   += TcpServ.cpp
SOURCE          += sock_thread_handler.cpp
SOURCE          += file_manage.cpp
SOURCE          += serializer.cpp

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
ALL_SRC_CLIENT  = $(SOURCE) $(SOURCE_CLIENT)
ALL_SRC_SERVER  = $(SOURCE) $(SOURCE_SERVER)
OBJECTS_CLIENT  = $(addprefix $(BUILD_DIR)/, $(ALL_SRC_CLIENT:.cpp=.o))
OBJECTS_SERVER  = $(addprefix $(BUILD_DIR)/, $(ALL_SRC_SERVER:.cpp=.o))
CFLAGS          += $(addprefix -I, $(INC_DIR))
PROJECT_CLIENT  = $(OUT_DIR)/$(PROJECT_NAME_CLIENT)
PROJECT_SERVER  = $(OUT_DIR)/$(PROJECT_NAME_SERVER)

################################
##        Make config
################################
.DEFAULT_GOAL := all

################################
##           Targets
################################
all: clean build

build: $(PROJECT_SERVER) $(PROJECT_CLIENT)

$(PROJECT_SERVER): $(OBJECTS_SERVER)
	@mkdir -p $(dir $@)
	@echo $(LDPRINT)
	@$(CXX) -o $@ $(OBJECTS_SERVER)

$(PROJECT_CLIENT): $(OBJECTS_CLIENT)
	@mkdir -p $(dir $@)
	@echo $(LDPRINT)
	@$(CXX) -o $@ $(OBJECTS_CLIENT)

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo $(CCPRINT)
	@$(CXX) -c $(CFLAGS) $< -o $@

clean:
	@rm -rf $(BUILD_DIR)
	@rm -rf $(OUT_DIR)

run_client:
	@$(PROJECT_CLIENT)

run_server:
	@$(PROJECT_SERVER)
