packages <- scan("top_100_CRAN_packages.txt", character())
install.packages(packages, INSTALL_opts = "--byte-compile")