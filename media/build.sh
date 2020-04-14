latexmk -xelatex -output-directory=build \
    home power scroll-x scroll-y setup tap

for file in build/*.pdf
do
    inkscape $file --export-plain-svg ${file/.pdf/.svg}
done

mv build/*.svg .
rm -rf build
