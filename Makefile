INCLUDE_PATH=-Ighttp-qiniu
SOURCE_FILES=\
	ghttp-qiniu/ghttp-qiniu.c\
	main.c

all: $(SOURCE_FILES)
	gcc -std=c99 -g $^ -o upload $(INCLUDE_PATH) -lghttp
