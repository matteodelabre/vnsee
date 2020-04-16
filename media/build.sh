latexmk -xelatex -output-directory=build \
    home power scroll-x scroll-y setup tap

for file in build/{home,power,scroll-x,scroll-y,tap}.pdf
do
    inkscape $file --export-plain-svg ${file/.pdf/.svg}
done
mv build/*.svg .

pdftoppm build/setup.pdf build/setup -rx 600 -ry 600 -png
convert -delay 50 -loop 0 build/setup-*.png setup.gif
