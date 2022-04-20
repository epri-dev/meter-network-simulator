FROM fedora:36

LABEL maintainer="Ed Beroset <beroset@ieee.org>"

WORKDIR /tmp/
ENV TZ=America/New_York
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
RUN dnf install -y gcc-c++ git cmake gnuplot sqlite-devel gsl-devel \
    doxygen dia python3-sphinx texlive-collection-fontsrecommended \
    texlive-collection-latexextra
RUN git clone https://gitlab.com/nsnam/ns-3-dev.git
WORKDIR /tmp/ns-3-dev/
RUN ./ns3 configure --build-profile=debug --enable-examples --enable-tests
RUN ./ns3 docs all
