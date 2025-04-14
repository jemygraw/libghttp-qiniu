INCLUDE_PATH=-Ighttp_qiniu
FORM_FULL_PARAMS_SOURCE_FILES=\
	ghttp_qiniu/qiniu_utils.c\
	ghttp_qiniu/form_upload.c\
	examples/form_upload_full_params.c

CHUNK_USING_HASH_AS_KEY_SOURCE_FILES=\
	ghttp_qiniu/qiniu_utils.c\
	ghttp_qiniu/chunk_upload.c\
	examples/chunk_upload_using_hash_as_key.c

CHUNK_FULL_PARAMS_SOURCE_FILES=\
	ghttp_qiniu/qiniu_utils.c\
	ghttp_qiniu/chunk_upload.c\
	examples/chunk_upload_full_params.c

CHUNK_RUN_IN_THREAD_SOURCE_FILES=\
	ghttp_qiniu/qiniu_utils.c\
	ghttp_qiniu/chunk_upload.c\
	examples/chunk_upload_run_in_thread.c


all: prepare form_full_params \
	chunk_full_params \
	chunk_using_hash_as_key \
	chunk_run_in_thread

prepare:
	$(shell mkdir -p bin/)

clean:
	$(shell rm -rf bin/)

form_full_params: $(FORM_FULL_PARAMS_SOURCE_FILES)
	gcc -std=c99 -g $^ -o bin/form_upload_full_params $(INCLUDE_PATH) -lghttp -lcjson

chunk_using_hash_as_key: $(CHUNK_USING_HASH_AS_KEY_SOURCE_FILES)
	gcc -std=c99 -g $^ -o bin/chunk_upload_using_hash_as_key $(INCLUDE_PATH) -lghttp -lcjson

chunk_full_params: $(CHUNK_FULL_PARAMS_SOURCE_FILES)
	gcc -std=c99 -g $^ -o bin/chunk_upload_full_params $(INCLUDE_PATH) -lghttp -lcjson

chunk_run_in_thread: $(CHUNK_RUN_IN_THREAD_SOURCE_FILES)
	gcc -std=c99 -g $^ -o bin/chunk_run_in_thread $(INCLUDE_PATH) -lghttp -lcjson