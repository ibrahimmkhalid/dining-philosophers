install: main helper

main: main.c common.h
	gcc main.c -o main -lpthread -lrt

helper: helper.c common.h
	gcc helper.c -o helper

uninstall:
	rm helper -f
	rm main -f
