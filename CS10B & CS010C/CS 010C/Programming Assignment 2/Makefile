all: run_test1 run_test2 run_test3 run_test4 run_test5 run_test6 run_test7 run_test8 run_test9 run_test10 run_test11 run_test12 run_test13 run_test14
program2: main.cpp Node.h Node.cpp BSTree.h BSTree.cpp
	$(CXX) -o program2 -std=c++11 main.cpp Node.cpp BSTree.cpp

test14: test14.cpp Node.h Node.cpp BSTree.h BSTree.cpp
	$(CXX) -o test14 -std=c++11 test14.cpp Node.cpp BSTree.cpp

test1: program2
	./program2 < input.1
run_test1: program2
	./program2 < input.1 > output.1
	diff output.1 expected.1
test2: program2
	./program2 < input.2
run_test2: program2
	./program2 < input.2 > output.2
	diff output.2 expected.2
test3: program2
	./program2 < input.3
run_test3: program2
	./program2 < input.3 > output.3
	diff output.3 expected.3
test4: program2
	./program2 < input.4
run_test4: program2
	./program2 < input.4 > output.4
	diff output.4 expected.4
test5: program2
	./program2 < input.5
run_test5: program2
	./program2 < input.5 > output.5
	diff output.5 expected.5
test6: program2
	./program2 < input.6
run_test6: program2
	./program2 < input.6 > output.6
	diff output.6 expected.6
test7: program2
	./program2 < input.7
run_test7: program2
	./program2 < input.7 > output.7
	diff output.7 expected.7
test8: program2
	./program2 < input.8
run_test8: program2
	./program2 < input.8 > output.8
	diff output.8 expected.8
test9: program2
	./program2 < input.9
run_test9: program2
	./program2 < input.9 > output.9
	diff output.9 expected.9
test10: program2
	./program2 < input.10
run_test10: program2
	./program2 < input.10 > output.10
	diff output.10 expected.10
test11: program2
	./program2 < input.11
run_test11: program2
	./program2 < input.11 > output.11
	diff output.11 expected.11
test12: program2
	./program2 < input.12
run_test12: program2
	./program2 < input.12 > output.12
	diff output.12 expected.12
test13: program2
	./program2 < input.13
run_test13: program2
	./program2 < input.13 > output.13
	diff output.13 expected.13
run_test14: program2 test14
	./test14
