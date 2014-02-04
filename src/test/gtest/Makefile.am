
dir_gtest = gtest

gtestdir = 
gtest_LIBRARIES = libgtest.a libgtest_main.a

libgtest_a_SOURCES = 
libgtest_main_a_SOURCES = 

if MUST_COMPILE_GTEST
libgtest.a: ${gtest_src}/src/gtest-all.cc
	$(CXX) $(gtest_CPPFLAGS) -I$(gtest_src) -pthread -c $<
	$(AR) cru $@ gtest-all.o

libgtest_main.a: ${gtest_src}/src/gtest_main.cc
	$(CXX) $(gtest_CPPFLAGS) -pthread -c $<
	$(AR) cru $@ gtest_main.o
endif # MUST_COMPILE_GTEST
