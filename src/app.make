.PHONY: all
all:
	gcc \
	  `pkg-config --cflags gtk+-3.0` \
	  -g -O0 \
	  -o app config-file.c app.main.c debug.c \
	  `pkg-config --cflags --libs gtk+-3.0`

.PHONY: clean
clean:
	rm -f app
