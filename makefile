mmu: main.cpp process.cpp pte.cpp frame.cpp pager.cpp
	g++ -std=c++17 main.cpp process.cpp pte.cpp frame.cpp pager.cpp -o mmu
clean:
	rm -f mmu *~

