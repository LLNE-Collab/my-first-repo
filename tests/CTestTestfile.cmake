# CMake generated Testfile for 
# Source directory: /home/llne/breakout_project/tests
# Build directory: /home/llne/breakout_project/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(LogicTests "/home/llne/breakout_project/tests/test_logic")
set_tests_properties(LogicTests PROPERTIES  _BACKTRACE_TRIPLES "/home/llne/breakout_project/tests/CMakeLists.txt;10;add_test;/home/llne/breakout_project/tests/CMakeLists.txt;0;")
subdirs("../_deps/googletest-build")
