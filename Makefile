TEST_DIR:=test
TEST_RES_DIR:=$(TEST_DIR)/results
TEST_SRCS = $(wildcard $(TEST_DIR)/*.lf)
TEST_SCRIPTS=$(TEST_DIR)/scripts

TEST_RESULTS = $(patsubst $(TEST_DIR)/%.lf,$(TEST_RES_DIR)/%_res.txt,$(TEST_SRCS))

.PHONY: test
test: $(TEST_RESULTS)
	@echo FINISHED

$(TEST_RES_DIR)/%_res.txt: $(TEST_DIR)/%.lf
	@echo Executing $^
	mkdir -p $(TEST_RES_DIR)
	bash $(TEST_SCRIPTS)/test_prolog.sh $@
	lfc -c $^ 
	sleep 2
	bash $(TEST_SCRIPTS)/test_epilogue.sh 
	bash $(TEST_SCRIPTS)/test_parse.sh $@

.PHONY: clean
clean:
	rm -r src-gen
	rm test/results/*
