all:
	clang -stdlib=libc++ -std=c++14 -lc++ -lzmq -lsodium  -onode node.cc type.cc curve.cc socket.cc
	clang -stdlib=libc++ -std=c++14 -lc++ -lzmq -lsodium  -onodetest nodetest.cc type.cc curve.cc socket.cc 
	#clang -stdlib=libc++ -std=c++14 -lc++ -lzmq -lsodium -osnd snd.cc curve.cc socket.cc
	#clang -stdlib=libc++ -std=c++14 -lc++ -lzmq -lsodium -osnd snd.cc curve.cc socket.cc


