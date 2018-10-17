#
# Main Makefile. This is basically the same as a component makefile.
#
COMPONENT_ADD_LDFLAGS=-lstdc++ -l$(COMPONENT_NAME)
COMPONENT_SRCDIRS := esp321 PN532_SPI PN532 NDEF
COMPONENT_ADD_INCLUDEDIRS := esp321 PN532_SPI PN532 NDEF
