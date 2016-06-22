all:
	clang -stdlib=libc++ -std=c++14 -lc++ -lzmq -orcv rcv.cc curve.cc socket.cc
	clang -stdlib=libc++ -std=c++14 -lc++ -lzmq -osnd snd.cc curve.cc socket.cc


