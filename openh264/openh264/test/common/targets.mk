# This file is autogenerated, do not edit it directly, edit build/mktargets.py
# instead. To regenerate files, run build/mktargets.sh.

COMMON_UNITTEST_SRCDIR=test/common
COMMON_UNITTEST_CPP_SRCS=\
	$(COMMON_UNITTEST_SRCDIR)/CWelsListTest.cpp\
	$(COMMON_UNITTEST_SRCDIR)/ExpandPicture.cpp\
	$(COMMON_UNITTEST_SRCDIR)/WelsTaskListTest.cpp\
	$(COMMON_UNITTEST_SRCDIR)/WelsThreadPoolTest.cpp\

COMMON_UNITTEST_OBJS += $(COMMON_UNITTEST_CPP_SRCS:.cpp=.$(OBJ))

OBJS += $(COMMON_UNITTEST_OBJS)

$(COMMON_UNITTEST_SRCDIR)/%.$(OBJ): $(COMMON_UNITTEST_SRCDIR)/%.cpp
	$(QUIET_CXX)$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(COMMON_UNITTEST_CFLAGS) $(COMMON_UNITTEST_INCLUDES) -c $(CXX_O) $<

