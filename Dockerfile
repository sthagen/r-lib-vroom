FROM rocker/verse:3.5.2

RUN R -e 'devtools::install_dev_deps()'
