INCLUDE_PATH=-Ighttp-qiniu
FORM_SOURCE_FILES=\
	ghttp-qiniu/qiniu_utils.c\
	ghttp-qiniu/form-upload.c\
	examples/form-upload.c

CHUNK_SOURCE_FILES=\
	ghttp-qiniu/qiniu_utils.c\
	ghttp-qiniu/chunk-upload.c\
	examples/chunk-upload.c

form: $(FORM_SOURCE_FILES)
	gcc -std=c99 -g $^ -o form_upload $(INCLUDE_PATH) -lghttp -lcjson
chunk: $(CHUNK_SOURCE_FILES)
	gcc -std=c99 -g $^ -o chunk_upload $(INCLUDE_PATH) -lghttp -lcjson