Jack Martinez
N18522789
shell.c

Major defects:
	memory leak: tlist is never freed and malloced multiple times, therefore a memory leak occurs

Minor defects:
	function length: execute is too long

Notes:
	cd works but relies on the $HOME env variable being set to work.
	io redirection works, you can do:
		sort < unsortedfile > sortedfile 2> errorsfile
	but you can't chain multiple outputs:
		ls > test1 > test2 > test3
	or:
		ls > test1 >> test2
	
