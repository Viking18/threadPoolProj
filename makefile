dept = main.cpp myThreadPool.cpp
h = myThreadPool.h
t = main

$t : $(dept) $h
	g++ $(dept) -o $t --std=c++11
