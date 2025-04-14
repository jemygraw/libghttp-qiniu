INCLUDE_PATH=-Ighttp-qiniu
FORM_SOURCE_FILES=\
	ghttp-qiniu/qiniu_utils.c\
	ghttp-qiniu/form-upload.c\
	examples/form-upload.c

CHUNK_USING_HASH_AS_KEY_SOURCE_FILES=\
	ghttp-qiniu/qiniu_utils.c\
	ghttp-qiniu/chunk-upload.c\
	examples/chunk-upload-using-hash-as-key.c

CHUNK_FULL_PARAMS_SOURCE_FILES=\
	ghttp-qiniu/qiniu_utils.c\
	ghttp-qiniu/chunk-upload.c\
	examples/chunk-upload-full-params.c


form: $(FORM_SOURCE_FILES)
	gcc -std=c99 -g $^ -o form-upload $(INCLUDE_PATH) -lghttp -lcjson

chunk_using_hash_as_key: $(CHUNK_USING_HASH_AS_KEY_SOURCE_FILES)
	gcc -std=c99 -g $^ -o chunk-upload-using-hash-as-key $(INCLUDE_PATH) -lghttp -lcjson

chunk_full_params: $(CHUNK_FULL_PARAMS_SOURCE_FILES)
	gcc -std=c99 -g $^ -o chunk-upload-full-params $(INCLUDE_PATH) -lghttp -lcjson