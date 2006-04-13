#
# Makefile for the common code library project documentation. 
# This probably requires GNU make.
#

MODULE=uclmmbase
SGML_TOPLEVEL=$(MODULE)-driver.sgml

CODE_DIR=../src
CODE_SRCS=$(CODE_DIR)/*.c $(CODE_DIR)/*.h

html: sgml $(SGML_TOPLEVEL)
	-cd html && gtkdoc-mkhtml $(MODULE) ../$(SGML_TOPLEVEL)

sgml: tmpl
	gtkdoc-mkdb --module=$(MODULE) --source-dir=$(CODE_DIR)

tmpl: scan
	gtkdoc-mktmpl --module=$(MODULE)

scan: $(CODE_SRCS)
	gtkdoc-scan --module=$(MODULE) --source-dir=$(CODE_DIR)

clean:
	-rm -f *.bak sgml/*.bak *unused.txt

distclean: clean
	-rm -f $(MODULE)-decl-list.txt $(MODULE)-decl.txt Makefile
	-rm -rf sgml/*

.PHONY: html sgml tmpl clean distclean
