#makefile for mmdaq
# M.Byszewski, CERN

UNAME := $(shell uname -s)
SHELL = /bin/sh

EXEC=mmdaq

DEPDIR = .deps
df = $(DEPDIR)/$(*F)
MKDIR_P = mkdir -p

#CC=g++ -O3
CC=g++
CFLAGSBASE=-c -g -O3 -Wall
#-Wextra
LFLAGSBASE=-lpthread -lstdc++

SRCDAQ=main.cpp Dict.cpp CMMDaq.cpp CThread.cpp CThreadRoot.cpp CMutex.cpp \
	CPublisher.cpp CLogger.cpp CRootWriter.cpp CEventDecoder.cpp CReceiver.cpp \
	CApvPedestalsRoot.cpp CApvRawPedRoot.cpp CApvRawTree.cpp CConfiguration.cpp  \
   CMMEvent.cpp CApvEvent.cpp CEvent.cpp CUDPData.cpp \
	CUserInterface.cpp CDetElement.cpp CDetFec.cpp CDetChip.cpp CDetChamber.cpp \
   CDetReadout.cpp CGuiTabContents.cpp CGuiDetConfig.cpp CGuiUdpRawData.cpp CDetReadout.cpp

ROOTHDRS = CApvRawPedRoot.h CApvPedestalsRoot.h CUserInterface.h CGuiTabContents.h CGuiDetConfig.h

SVNVER = svn_version.c

SRC=$(SRCDAQ)

#HDR=Ntp.h CUserInterface.h CListener.h
OBJ=$(SRC:.cpp=.o)
HDR=$(SRC:.cpp=.h)


#BOOSt configuration
BOOSTINC=-I/usr/include/boost
BOOSTLIB=-L/usr/lib/x86_64-linux-gnu -lboost_program_options -lboost_thread -lboost_system -lboost_filesystem -lboost_date_time


#for this to work root must be set up already (source thisroot.sh)
ROOTINC=-I`root-config --incdir`
ROOTCFLAGS=`root-config --cflags --auxcflags`
ROOTLIB=`root-config --glibs`
ROOTLDFLAGS=`root-config --ldflags`

################################################################################
## do not change anything below
################################################################################


CFLAGS= $(CFLAGSBASE) $(BOOSTINC) $(ROOTINC) $(ROOTCFLAGS)
LDFLAGS_0= $(LFLAGSBASE) $(BOOSTLIB)  $(ROOTLDFLAGS) $(ROOTLIB) -L/usr/lib/gcc/x86_64-linux-gnu/9


ifeq ($(UNAME),Darwin)
# set Mac os X Specifics
LDFLAGS=$(LDFLAGS_0)
endif
ifeq ($(UNAME),Linux)
# set Linux Specifics
LDFLAGS=$(LDFLAGS_0) -lrt
endif


# ‘$^’ is a list of all the prerequisites of the rule, including the names of the directories in which they were found
# '$@' is the target.
# ‘$<’ is just the first prereq. Often the prereqs. include header files, which you do not want to mention in the recipe.
# from 4.5.4 Writing Recipes with Directory Search
# http://www.gnu.org/software/make/manual/make.html
################################################################################
.PHONY : clean directories svn_version.c

all: svn_version.c directories $(EXEC) 

directories: $(DEPDIR)

$(DEPDIR):
	@$(MKDIR_P) $(DEPDIR)

$(EXEC): svn_version.o $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -MD -o $@ $<
	@cp $*.d $(df).P; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	-e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $(df).P; \
	rm -f $*.d

-include $(SRC:%.cpp=$(DEPDIR)/%.P)


Dict.cpp: $(ROOTHDRS) LinkDef.h
	@echo "* Generating root dictionary"
	rootcint -f $@ -c $^

clean:
	@echo "* Clean"
	@rm -f $(OBJ)
	@rm -f $(DEPDIR)/*.P
	@rm -f Dict.cpp
	@rm -f svn_version.a svn_version.o

##
## on every build, record the working copy revision string
##
svn_version.c:
	echo 'const char* svn_version(void) { \' > svn_version.inc
	echo ' const char* SVN_Version = "\' >> svn_version.inc
	svnversion -n .  >> svn_version.inc
	echo '"; return SVN_Version;  }'   >> svn_version.inc
	echo ' '   >> svn_version.inc
