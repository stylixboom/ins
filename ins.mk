##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Release
ProjectName            :=ins
ConfigurationName      :=Release
WorkspacePath          := "${HOME}/webstylix/code"
ProjectPath            := "${HOME}/webstylix/code/lib/ins"
IntermediateDirectory  :=./Release
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=Siriwat Kasamwattanarote
Date                   :=10/04/15
CodeLitePath           :=""
LinkerName             :=g++
SharedObjectLinkerName :=g++ -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(IntermediateDirectory)/lib$(ProjectName).a
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="ins.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch)${HOME}/local/include 
IncludePCH             := 
RcIncludePath          := 
Libs                   := 
ArLibs                 :=  
LibPath                := $(LibraryPathSwitch). $(LibraryPathSwitch)${HOME}/local/lib 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := ar rcu
CXX      := g++
CC       := gcc
CXXFLAGS :=  -O3 -std=c++11 $(Preprocessors)
CFLAGS   :=   $(Preprocessors)
ASFLAGS  := 
AS       := as


##
## User defined environment variables
##
Objects0=$(IntermediateDirectory)/bow.cpp$(ObjectSuffix) $(IntermediateDirectory)/compat.cpp$(ObjectSuffix) $(IntermediateDirectory)/core.cpp$(ObjectSuffix) $(IntermediateDirectory)/dataset_info.cpp$(ObjectSuffix) $(IntermediateDirectory)/gvp.cpp$(ObjectSuffix) $(IntermediateDirectory)/homography.cpp$(ObjectSuffix) $(IntermediateDirectory)/ins_param.cpp$(ObjectSuffix) $(IntermediateDirectory)/invert_index.cpp$(ObjectSuffix) $(IntermediateDirectory)/kp_dumper.cpp$(ObjectSuffix) $(IntermediateDirectory)/qb.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/qe.cpp$(ObjectSuffix) $(IntermediateDirectory)/quantizer.cpp$(ObjectSuffix) $(IntermediateDirectory)/utils.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_mask.cpp$(ObjectSuffix) $(IntermediateDirectory)/core_dataset.cpp$(ObjectSuffix) $(IntermediateDirectory)/math_vector.cpp$(ObjectSuffix) $(IntermediateDirectory)/retr_pooling.cpp$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(IntermediateDirectory) $(OutputFile)

$(OutputFile): $(Objects)
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(AR) $(ArchiveOutputSwitch)$(OutputFile) @$(ObjectsFileList) $(ArLibs)
	@$(MakeDirCommand) "${HOME}/webstylix/code/.build-release"
	@echo rebuilt > "${HOME}/webstylix/code/.build-release/ins"

MakeIntermediateDirs:
	@test -d ./Release || $(MakeDirCommand) ./Release


./Release:
	@test -d ./Release || $(MakeDirCommand) ./Release

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/bow.cpp$(ObjectSuffix): bow.cpp $(IntermediateDirectory)/bow.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "bow.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bow.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bow.cpp$(DependSuffix): bow.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bow.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bow.cpp$(DependSuffix) -MM "bow.cpp"

$(IntermediateDirectory)/bow.cpp$(PreprocessSuffix): bow.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bow.cpp$(PreprocessSuffix) "bow.cpp"

$(IntermediateDirectory)/compat.cpp$(ObjectSuffix): compat.cpp $(IntermediateDirectory)/compat.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "compat.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/compat.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/compat.cpp$(DependSuffix): compat.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/compat.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/compat.cpp$(DependSuffix) -MM "compat.cpp"

$(IntermediateDirectory)/compat.cpp$(PreprocessSuffix): compat.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/compat.cpp$(PreprocessSuffix) "compat.cpp"

$(IntermediateDirectory)/core.cpp$(ObjectSuffix): core.cpp $(IntermediateDirectory)/core.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "core.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core.cpp$(DependSuffix): core.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/core.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/core.cpp$(DependSuffix) -MM "core.cpp"

$(IntermediateDirectory)/core.cpp$(PreprocessSuffix): core.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core.cpp$(PreprocessSuffix) "core.cpp"

$(IntermediateDirectory)/dataset_info.cpp$(ObjectSuffix): dataset_info.cpp $(IntermediateDirectory)/dataset_info.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "dataset_info.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/dataset_info.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/dataset_info.cpp$(DependSuffix): dataset_info.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/dataset_info.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/dataset_info.cpp$(DependSuffix) -MM "dataset_info.cpp"

$(IntermediateDirectory)/dataset_info.cpp$(PreprocessSuffix): dataset_info.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/dataset_info.cpp$(PreprocessSuffix) "dataset_info.cpp"

$(IntermediateDirectory)/gvp.cpp$(ObjectSuffix): gvp.cpp $(IntermediateDirectory)/gvp.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "gvp.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/gvp.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/gvp.cpp$(DependSuffix): gvp.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/gvp.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/gvp.cpp$(DependSuffix) -MM "gvp.cpp"

$(IntermediateDirectory)/gvp.cpp$(PreprocessSuffix): gvp.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/gvp.cpp$(PreprocessSuffix) "gvp.cpp"

$(IntermediateDirectory)/homography.cpp$(ObjectSuffix): homography.cpp $(IntermediateDirectory)/homography.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "homography.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/homography.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/homography.cpp$(DependSuffix): homography.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/homography.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/homography.cpp$(DependSuffix) -MM "homography.cpp"

$(IntermediateDirectory)/homography.cpp$(PreprocessSuffix): homography.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/homography.cpp$(PreprocessSuffix) "homography.cpp"

$(IntermediateDirectory)/ins_param.cpp$(ObjectSuffix): ins_param.cpp $(IntermediateDirectory)/ins_param.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "ins_param.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/ins_param.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/ins_param.cpp$(DependSuffix): ins_param.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/ins_param.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/ins_param.cpp$(DependSuffix) -MM "ins_param.cpp"

$(IntermediateDirectory)/ins_param.cpp$(PreprocessSuffix): ins_param.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/ins_param.cpp$(PreprocessSuffix) "ins_param.cpp"

$(IntermediateDirectory)/invert_index.cpp$(ObjectSuffix): invert_index.cpp $(IntermediateDirectory)/invert_index.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "invert_index.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/invert_index.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/invert_index.cpp$(DependSuffix): invert_index.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/invert_index.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/invert_index.cpp$(DependSuffix) -MM "invert_index.cpp"

$(IntermediateDirectory)/invert_index.cpp$(PreprocessSuffix): invert_index.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/invert_index.cpp$(PreprocessSuffix) "invert_index.cpp"

$(IntermediateDirectory)/kp_dumper.cpp$(ObjectSuffix): kp_dumper.cpp $(IntermediateDirectory)/kp_dumper.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "kp_dumper.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/kp_dumper.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/kp_dumper.cpp$(DependSuffix): kp_dumper.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/kp_dumper.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/kp_dumper.cpp$(DependSuffix) -MM "kp_dumper.cpp"

$(IntermediateDirectory)/kp_dumper.cpp$(PreprocessSuffix): kp_dumper.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/kp_dumper.cpp$(PreprocessSuffix) "kp_dumper.cpp"

$(IntermediateDirectory)/qb.cpp$(ObjectSuffix): qb.cpp $(IntermediateDirectory)/qb.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "qb.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/qb.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/qb.cpp$(DependSuffix): qb.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/qb.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/qb.cpp$(DependSuffix) -MM "qb.cpp"

$(IntermediateDirectory)/qb.cpp$(PreprocessSuffix): qb.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/qb.cpp$(PreprocessSuffix) "qb.cpp"

$(IntermediateDirectory)/qe.cpp$(ObjectSuffix): qe.cpp $(IntermediateDirectory)/qe.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "qe.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/qe.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/qe.cpp$(DependSuffix): qe.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/qe.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/qe.cpp$(DependSuffix) -MM "qe.cpp"

$(IntermediateDirectory)/qe.cpp$(PreprocessSuffix): qe.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/qe.cpp$(PreprocessSuffix) "qe.cpp"

$(IntermediateDirectory)/quantizer.cpp$(ObjectSuffix): quantizer.cpp $(IntermediateDirectory)/quantizer.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "quantizer.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/quantizer.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/quantizer.cpp$(DependSuffix): quantizer.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/quantizer.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/quantizer.cpp$(DependSuffix) -MM "quantizer.cpp"

$(IntermediateDirectory)/quantizer.cpp$(PreprocessSuffix): quantizer.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/quantizer.cpp$(PreprocessSuffix) "quantizer.cpp"

$(IntermediateDirectory)/utils.cpp$(ObjectSuffix): utils.cpp $(IntermediateDirectory)/utils.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "utils.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/utils.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/utils.cpp$(DependSuffix): utils.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/utils.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/utils.cpp$(DependSuffix) -MM "utils.cpp"

$(IntermediateDirectory)/utils.cpp$(PreprocessSuffix): utils.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/utils.cpp$(PreprocessSuffix) "utils.cpp"

$(IntermediateDirectory)/core_mask.cpp$(ObjectSuffix): core/mask.cpp $(IntermediateDirectory)/core_mask.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "core/mask.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_mask.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_mask.cpp$(DependSuffix): core/mask.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/core_mask.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/core_mask.cpp$(DependSuffix) -MM "core/mask.cpp"

$(IntermediateDirectory)/core_mask.cpp$(PreprocessSuffix): core/mask.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_mask.cpp$(PreprocessSuffix) "core/mask.cpp"

$(IntermediateDirectory)/core_dataset.cpp$(ObjectSuffix): core/dataset.cpp $(IntermediateDirectory)/core_dataset.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "core/dataset.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/core_dataset.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/core_dataset.cpp$(DependSuffix): core/dataset.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/core_dataset.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/core_dataset.cpp$(DependSuffix) -MM "core/dataset.cpp"

$(IntermediateDirectory)/core_dataset.cpp$(PreprocessSuffix): core/dataset.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/core_dataset.cpp$(PreprocessSuffix) "core/dataset.cpp"

$(IntermediateDirectory)/math_vector.cpp$(ObjectSuffix): math/vector.cpp $(IntermediateDirectory)/math_vector.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "math/vector.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/math_vector.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/math_vector.cpp$(DependSuffix): math/vector.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/math_vector.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/math_vector.cpp$(DependSuffix) -MM "math/vector.cpp"

$(IntermediateDirectory)/math_vector.cpp$(PreprocessSuffix): math/vector.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/math_vector.cpp$(PreprocessSuffix) "math/vector.cpp"

$(IntermediateDirectory)/retr_pooling.cpp$(ObjectSuffix): retr/pooling.cpp $(IntermediateDirectory)/retr_pooling.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "retr/pooling.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/retr_pooling.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/retr_pooling.cpp$(DependSuffix): retr/pooling.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/retr_pooling.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/retr_pooling.cpp$(DependSuffix) -MM "retr/pooling.cpp"

$(IntermediateDirectory)/retr_pooling.cpp$(PreprocessSuffix): retr/pooling.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/retr_pooling.cpp$(PreprocessSuffix) "retr/pooling.cpp"


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Release/


