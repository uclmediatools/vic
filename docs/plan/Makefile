# $Id$

TEXFILES=$(wildcard *.tex)
BIBFILE=report.bib
TARGET=report
FIGDIR=fig

RM	:= rm -f

# make pdf by default
#all: ${TARGET}.ps
${TARGET}.pdf:  ${TARGET}.ps
	@ps2pdf ${TARGET}.ps

# # it doesn't really need the .dvi, but this way all the refs are right
#%.pdf : %.dvi
#	pdflatex $*
         

# ${TARGET}.bbl: ../bib/music.bib
#
# # in case we don't already have a .aux file listing citations
# # this should probably be a separate makefile target/dependency instead
# # of doing it every time... but *shrug*
# #       latex ${TARGET}.tex
#
# # get the citations out of the bibliography
# #       bibtex ${TARGET}
#
# # do it again in case there are out-of-order cross-references
# #       @latex ${TARGET}.tex
#
# #${TARGET}.dvi: ${TARGET}.bbl ${TARGET}.tex
# #@latex ${TARGET}.tex
#
${TARGET}.dvi: $(TEXFILES)
	@latex ${TARGET}
	@bibtex ${TARGET}
	@latex ${TARGET}
	@latex ${TARGET}
#prosper: 
#	#${TARGET}.tex 
#	# clean:
#	@latex ${TARGET}.tex
#	@bibtex ${TARGET}
#	@latex ${TARGET}.tex
#	@dvips -o ${TARGET}.ps ${TARGET}
#	@ps2pdf ${TARGET}.ps ${TARGET}.pdf

# shortcut, so we can say "make ps"
ps: ${TARGET}.ps

${TARGET}.ps: ${TARGET}.dvi
	@dvips -t a4 -o ${TARGET}.ps ${TARGET}

clean:
	@echo "Start cleaning..."
	$(RM) ${TARGET}.aux ${TARGET}.toc ${TARGET}.lo[a,f,g,t]
	$(RM) ${TARGET}.bbl ${TARGET}.blg ${TARGET}.brf
	$(RM) ${TARGET}.dvi ${TARGET}.ps ${TARGET}.pdf
	$(RM) *.*~


distclean: clean
	$(RM) $(FIGDIR)/*

wordcount:
	@echo Approximate word count: \ 
	`grep -v '^\\\\' $(TEXFILES) | grep -v '^%' | wc -w`
                                        
PHONY : ps all clean distclean
