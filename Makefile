# Makefile

CXX=g++-mp-4.7
CXX_FLAGS=-O3 -std=c++11

all: author_topic cpt

author_topic: 
	${CXX} ${CXX_FLAGS} ./src/author_topic.cc -o author_topic

cpt: 
	${CXX} ${CXX_FLAGS} ./src/cpt.cc -o cpt

clean: 
	rm author_topic cpt
