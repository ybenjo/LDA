# Makefile

CXX=g++-mp-4.7
CXX_FLAGS=-O3 -Wall -std=c++11

author_topic: 
	${CXX} ${CXX_FLAGS} ./src/author_topic.cc -o author_topic


clean: 
	rm author_topic
