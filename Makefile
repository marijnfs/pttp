all:
	clang -stdlib=libc++ -std=c++14 -lc++ -lzmq -orcv rcv.cc socket.cc
	clang -stdlib=libc++ -std=c++14 -lc++ -lzmq -osnd snd.cc socket.cc


