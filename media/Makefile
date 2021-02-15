all: \
    button-home.svg \
    button-power.svg \
    pen.svg \
    scroll-x.svg \
    scroll-y.svg \
    setup.gif \
    tap.svg \

.PHONY: all

%.pdf: %.tex
	$(eval BUILDDIR := $(shell mktemp -d "build-XXXXXXXXX"))
	xelatex -output-directory=$(BUILDDIR) $<
	mv $(BUILDDIR)/$@ $@
	rm -rf $(BUILDDIR)

%.svg: %.pdf
	inkscape $< --export-plain-svg --export-filename $@

%.gif: %.pdf
	$(eval BUILDDIR := $(shell mktemp -d "build-XXXXXXXXX"))
	pdftoppm $< $(BUILDDIR)/frame -rx 600 -ry 600 -png
	convert -delay 50 -loop 0 $(BUILDDIR)/frame-*.png $@
	rm -rf $(BUILDDIR)
