# $Id$

TEXFILES=$(wildcard *.tex)
BIBFILE=reference.bib
TARGET=proposal
FIGSDIR=figs

RM	:= rm -f

${TARGET}.pdf:	${TARGET}.ps
	@ps2pdf ${TARGET}.ps

${TARGET}.ps:	${TARGET}.dvi
	@dvips -t a4 -o ${TARGET}.ps ${TARGET}

${TARGET}.dvi:	$(TEXFILES)
	@latex ${TARGET}
	@bibtex ${TARGET}
	@latex ${TARGET}
	@latex ${TARGET}

clean:
	@echo "Start cleaning..."
	$(RM) ${TARGET}.dvi ${TARGET}.ps ${TARGET}.pdf
	$(RM) ${TARGET}.aux ${TARGET}.log
	$(RM) ${TARGET}.bbl ${TARGET}.blg ${TARGET}.brf
	$(RM) *.*~

distclean: clean
	@echo "Start cleaning figures..."
	$(RM) $(FIGSDIR)/*

wc:
	@echo Approximate word count: \
		`grep -v '^\\\\' $(TEXFILES)|grep -v '^%'|wc -w`

