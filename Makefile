all:
	clang -stdlib=libc++ -std=c++14 -lc++ -lzmq -lsodium -orcv rcv.cc curve.cc socket.cc
	clang -stdlib=libc++ -std=c++14 -lc++ -lzmq -lsodium -osnd snd.cc curve.cc socket.cc


