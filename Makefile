test:
	g++ lzss.cpp -o lzss
	./lzss 4162.in 4162.out
	./lzss 18726.in 18726.out
	./lzss 22356.in 22356.out