DSLite := /opt/ti/ccs/ccs_base/DebugServer/bin/DSLite

PROJECT := $(notdir $(CURDIR))
BUILD_DIR := Debug
CCXML := $(CURDIR)/target_config.ccxml
OUT := $(BUILD_DIR)/$(PROJECT).out

.PHONY: flash verify clean

flash:
	$(DSLite) flash \
		--config=$(CCXML) \
		--flash \
		--verify \
		--run \
		--verbose \
		$(OUT)

verify:
	$(DSLite) flash \
		--config=$(CCXML) \
		--flash \
		--verify \
		$(OUT)

clean:
	rm -f /tmp/dslite.log
