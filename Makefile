CC = gcc
SRC = main.c
OUT = cfire
OPT_LEVEL = -O2
aggresive: OPT_LEVEL = -O3
aggresive: build

build:
	$(CC) $(SRC) -o $(OUT) $(OPT_LEVEL)
all:
	$(CC) $(SRC) -o $(OUT) $(OPT_LEVEL)

clean:
	rm -f $(OUT)

