$pdf_mode = 1;
$pdflatex = 'pdflatex -interaction=nonstopmode -file-line-error %O %S';
@default_files = ('Kaushik Vada - Homework 4.tex');
$do_cd = 1;
$latexmk::illegal_in_texname = "\x00\t\f\n\r\$%\\\x7F"; # allow ~ in workspace path
