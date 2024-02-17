all:
	make -C src

check:
	make
	./tests/testsuite.sh

debug:
	make debug -C src

cover:
	make clean
	make cover -C src
	./tests/testsuite.sh
	gcovr --html --html-nested --output=report.html
	make coverclean

clean:
	rm -rf log.txt echo tmp.txt src/*/*.gc* src/*.gc* src/*/.deps logtest
	make clean -C src

coverclean:
	rm -rf log.txt echo tmp.txt src/*/*.gc* src/*.gc* src/*/.deps logtest
	make clean -C src

reportclean:
	rm report*
